/* Native alarm delivery. Queue mutations and callbacks hold the cooperative
 * interrupt gate, matching the original SDK's interrupt exclusion. The host
 * worker never fabricates a PowerPC OSContext: callbacks receive NULL. */
#include <dolphin/os.h>
#include "melee_alarm_backend.h"
#include <pthread.h>
#include <stdint.h>
#include <time.h>

static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static pthread_t worker;
static OSAlarm* head;
static OSAlarm* tail;
static int running;
static int stopping;

static void fail(char* message)
{
    OSPanic(__FILE__, __LINE__, "%s", message);
}

static void unlink_alarm(OSAlarm* alarm)
{
    if (alarm->prev) alarm->prev->next = alarm->next;
    else head = alarm->next;
    if (alarm->next) alarm->next->prev = alarm->prev;
    else tail = alarm->prev;
    alarm->prev = alarm->next = NULL;
    alarm->handler = NULL;
}

static void insert_alarm(OSAlarm* alarm, OSTime fire, OSAlarmHandler handler)
{
    if (alarm->handler) fail("Alarm is already scheduled");
    if (alarm->period > 0) {
        OSTime now = OSGetTime();
        __int128 next = alarm->start;
        if (next < now) {
            next += (((__int128) now - next) / alarm->period + 1) * alarm->period;
        }
        if (next > INT64_MAX) fail("Periodic alarm deadline overflow");
        fire = (OSTime) next;
    }
    alarm->fire = fire;
    alarm->handler = handler;
    OSAlarm* next = head;
    while (next && next->fire <= fire) next = next->next;
    alarm->next = next;
    alarm->prev = next ? next->prev : tail;
    if (alarm->prev) alarm->prev->next = alarm;
    else head = alarm;
    if (next) next->prev = alarm;
    else tail = alarm;
    pthread_cond_signal(&changed);
}

static void* run_alarms(void* unused)
{
    (void) unused;
    for (;;) {
        pthread_mutex_lock(&queue_mutex);
        while (!stopping) {
            if (!head) {
                pthread_cond_wait(&changed, &queue_mutex);
                continue;
            }
            __int128 delta = (__int128) head->fire - OSGetTime();
            if (delta <= 0) break;
            __int128 ns = (delta * 1000000000 + OS_TIMER_CLOCK - 1) / OS_TIMER_CLOCK;
            /* Bound each wait, including deadlines outside a host timespec. */
            if (ns > 1000000000) ns = 1000000000;
            struct timespec wait = {.tv_sec=(time_t)(ns / 1000000000),
                                    .tv_nsec=(long)(ns % 1000000000)};
            pthread_cond_timedwait_relative_np(&changed, &queue_mutex, &wait);
        }
        int stop = stopping;
        pthread_mutex_unlock(&queue_mutex);
        if (stop) return NULL;

        /* Never wait for the interrupt gate while holding the queue mutex. */
        BOOL enabled = OSDisableInterrupts();
        pthread_mutex_lock(&queue_mutex);
        OSAlarm* alarm = head;
        OSAlarmHandler handler = NULL;
        if (!stopping && alarm && alarm->fire <= OSGetTime()) {
            handler = alarm->handler;
            unlink_alarm(alarm);
            /* As on console, reinsert before callback so it can cancel itself. */
            if (alarm->period > 0) insert_alarm(alarm, 0, handler);
        }
        pthread_mutex_unlock(&queue_mutex);
        if (handler) handler(alarm, NULL);
        /* A one-shot callback may have freed its alarm. Do not touch it here. */
        OSRestoreInterrupts(enabled);
    }
}

void OSInitAlarm(void)
{
    BOOL enabled = OSDisableInterrupts();
    pthread_mutex_lock(&queue_mutex);
    if (!running) {
        stopping = 0;
        if (pthread_create(&worker, NULL, run_alarms, NULL)) {
            fail("Unable to start native alarm worker");
        }
        running = 1;
    }
    pthread_mutex_unlock(&queue_mutex);
    OSRestoreInterrupts(enabled);
}

void OSCreateAlarm(OSAlarm* alarm)
{
    if (!alarm) fail("Null alarm");
    BOOL enabled = OSDisableInterrupts();
    pthread_mutex_lock(&queue_mutex);
    for (OSAlarm* entry = head; entry; entry = entry->next) {
        if (entry == alarm) fail("Cannot recreate a scheduled alarm");
    }
    alarm->handler = NULL;
    alarm->prev = alarm->next = NULL;
    alarm->fire = alarm->period = alarm->start = 0;
    pthread_mutex_unlock(&queue_mutex);
    OSRestoreInterrupts(enabled);
}

void OSSetAbsAlarm(OSAlarm* alarm, long long time, OSAlarmHandler handler)
{
    if (!alarm || !handler) fail("Invalid alarm or handler");
    OSInitAlarm();
    BOOL enabled = OSDisableInterrupts();
    pthread_mutex_lock(&queue_mutex);
    alarm->period = 0;
    insert_alarm(alarm, time, handler);
    pthread_mutex_unlock(&queue_mutex);
    OSRestoreInterrupts(enabled);
}

void OSSetAlarm(OSAlarm* alarm, OSTime tick, OSAlarmHandler handler)
{
    __int128 deadline = (__int128) OSGetTime() + tick;
    if (tick <= 0 || deadline > INT64_MAX) fail("Invalid relative alarm deadline");
    OSSetAbsAlarm(alarm, (OSTime) deadline, handler);
}

void OSSetPeriodicAlarm(OSAlarm* alarm, OSTime start, OSTime period,
                        OSAlarmHandler handler)
{
    if (!alarm || !handler || period <= 0) fail("Invalid periodic alarm");
    OSInitAlarm();
    BOOL enabled = OSDisableInterrupts();
    pthread_mutex_lock(&queue_mutex);
    alarm->start = start;
    alarm->period = period;
    insert_alarm(alarm, 0, handler);
    pthread_mutex_unlock(&queue_mutex);
    OSRestoreInterrupts(enabled);
}

void OSCancelAlarm(OSAlarm* alarm)
{
    if (!alarm) fail("Null alarm");
    BOOL enabled = OSDisableInterrupts();
    pthread_mutex_lock(&queue_mutex);
    if (alarm->handler) unlink_alarm(alarm);
    pthread_cond_signal(&changed);
    pthread_mutex_unlock(&queue_mutex);
    OSRestoreInterrupts(enabled);
}

BOOL OSCheckAlarmQueue(void)
{
    BOOL enabled = OSDisableInterrupts();
    pthread_mutex_lock(&queue_mutex);
    OSAlarm* previous = NULL;
    BOOL valid = (!head == !tail);
    for (OSAlarm* alarm = head; valid && alarm; alarm = alarm->next) {
        valid = alarm->handler && alarm->prev == previous &&
                (!previous || previous->fire <= alarm->fire);
        previous = alarm;
    }
    valid = valid && previous == tail;
    pthread_mutex_unlock(&queue_mutex);
    OSRestoreInterrupts(enabled);
    return valid;
}

void melee_native_alarm_shutdown(void)
{
    BOOL enabled = OSDisableInterrupts();
    if (!enabled) fail("Alarm shutdown must run outside interrupt callbacks");
    pthread_mutex_lock(&queue_mutex);
    int join = running;
    stopping = 1;
    while (head) unlink_alarm(head);
    pthread_cond_signal(&changed);
    pthread_mutex_unlock(&queue_mutex);
    OSRestoreInterrupts(enabled);
    if (join) pthread_join(worker, NULL);
    pthread_mutex_lock(&queue_mutex);
    running = 0;
    pthread_mutex_unlock(&queue_mutex);
}
