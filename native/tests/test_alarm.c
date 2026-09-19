#include <dolphin/os.h>
#include "melee_alarm_backend.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static int count;
static u32 order[16];
static atomic_int total;
static int callback_entered;
static int callback_release;
static atomic_int cancel_started;
static atomic_int cancel_finished;

static void blocked_callback(OSAlarm* alarm, OSContext* context)
{
    (void) alarm;
    assert(context == NULL);
    pthread_mutex_lock(&mutex);
    callback_entered = 1;
    pthread_cond_broadcast(&changed);
    while (!callback_release) pthread_cond_wait(&changed, &mutex);
    pthread_mutex_unlock(&mutex);
}

static void* cancel_in_flight(void* value)
{
    atomic_store(&cancel_started, 1);
    OSCancelAlarm(value);
    atomic_store(&cancel_finished, 1);
    return NULL;
}


static void record(OSAlarm* alarm, OSContext* context)
{
    assert(context == NULL);
    BOOL enabled = OSDisableInterrupts();
    assert(!enabled); /* Callback already owns the interrupt gate. */
    OSRestoreInterrupts(enabled);
    pthread_mutex_lock(&mutex);
    assert(count < 16);
    order[count++] = alarm->tag;
    atomic_fetch_add(&total, 1);
    if (alarm->tag == 7 && count == 3) OSCancelAlarm(alarm);
    if (alarm->tag == 9 && count == 1) {
        OSCreateAlarm(alarm);
        OSSetAlarm(alarm, OSMillisecondsToTicks((OSTime)5), record);
    }
    pthread_cond_signal(&changed);
    pthread_mutex_unlock(&mutex);
}

static void wait_count(int wanted)
{
    struct timespec deadline;
    assert(clock_gettime(CLOCK_REALTIME, &deadline) == 0);
    deadline.tv_sec += 3;
    pthread_mutex_lock(&mutex);
    while (count < wanted) {
        int result = pthread_cond_timedwait(&changed, &mutex, &deadline);
        assert(result == 0); /* A missing callback must fail instead of hang. */
    }
    pthread_mutex_unlock(&mutex);
}

static void clear_count(void)
{
    pthread_mutex_lock(&mutex);
    count = 0;
    pthread_mutex_unlock(&mutex);
}

int main(void)
{
    OSAlarm first, second, cancelled, periodic, rearm;
    OSInitAlarm();
    OSInitAlarm();
    OSCreateAlarm(&first); first.tag = 1;
    OSCreateAlarm(&second); second.tag = 2;
    OSCreateAlarm(&cancelled); cancelled.tag = 3;
    BOOL enabled = OSDisableInterrupts();
    OSTime deadline = OSGetTime() + OSMillisecondsToTicks((OSTime)20);
    OSSetAbsAlarm(&first, deadline, record);
    OSSetAbsAlarm(&second, deadline, record);
    OSSetAbsAlarm(&cancelled, deadline, record);
    OSCancelAlarm(&cancelled);
    assert(OSCheckAlarmQueue());
    struct timespec delay = {.tv_nsec=40000000};
    assert(nanosleep(&delay, NULL) == 0);
    assert(atomic_load(&total) == 0);
    OSRestoreInterrupts(enabled);
    wait_count(2);
    enabled = OSDisableInterrupts();
    assert(order[0] == 1 && order[1] == 2 && count == 2);
    assert(!first.handler && !second.handler && !cancelled.handler);
    assert(OSCheckAlarmQueue());
    OSRestoreInterrupts(enabled);

    clear_count();
    OSCreateAlarm(&periodic); periodic.tag = 7;
    enabled = OSDisableInterrupts();
    OSTime before = OSGetTime();
    OSTime period = OSMillisecondsToTicks((OSTime)10);
    OSSetPeriodicAlarm(&periodic, 0, period, record);
    assert(periodic.fire > before && periodic.fire <= OSGetTime() + period);
    assert(periodic.fire % period == 0);
    OSRestoreInterrupts(enabled);
    wait_count(3);
    enabled = OSDisableInterrupts();
    assert(count == 3 && !periodic.handler && OSCheckAlarmQueue());
    OSRestoreInterrupts(enabled);

    clear_count();
    OSCreateAlarm(&rearm); rearm.tag = 9;
    OSSetAlarm(&rearm, OSMillisecondsToTicks((OSTime)5), record);
    wait_count(2);
    enabled = OSDisableInterrupts();
    assert(count == 2 && !rearm.handler);
    OSRestoreInterrupts(enabled);

    /* Cancellation on another thread cannot return while a callback runs. */
    OSCreateAlarm(&cancelled);
    OSSetAlarm(&cancelled, OSMillisecondsToTicks((OSTime)5), blocked_callback);
    struct timespec timeout;
    assert(clock_gettime(CLOCK_REALTIME, &timeout) == 0);
    timeout.tv_sec += 3;
    pthread_mutex_lock(&mutex);
    while (!callback_entered) {
        assert(pthread_cond_timedwait(&changed, &mutex, &timeout) == 0);
    }
    pthread_mutex_unlock(&mutex);
    pthread_t canceller;
    assert(pthread_create(&canceller, NULL, cancel_in_flight, &cancelled) == 0);
    for (int i = 0; i < 100 && !atomic_load(&cancel_started); ++i) {
        struct timespec brief = {.tv_nsec=1000000};
        nanosleep(&brief, NULL);
    }
    assert(atomic_load(&cancel_started));
    assert(!atomic_load(&cancel_finished));
    pthread_mutex_lock(&mutex);
    callback_release = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&mutex);
    assert(pthread_join(canceller, NULL) == 0);
    assert(atomic_load(&cancel_finished));

    /* Shutdown cancels future work, joins the worker, and permits a restart. */
    OSSetAlarm(&first, OSSecondsToTicks((OSTime)60), record);
    melee_native_alarm_shutdown();
    assert(!first.handler && OSCheckAlarmQueue());
    clear_count();
    OSSetAlarm(&first, OSMillisecondsToTicks((OSTime)5), record);
    wait_count(1);
    melee_native_alarm_shutdown();
    assert(OSCheckAlarmQueue());
    puts("Native alarms: ordered delivery, interrupt exclusion, cancellation, periodic alignment, callback rearming and shutdown/restart passed");
    return 0;
}
