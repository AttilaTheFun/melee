#include <dolphin/os.h>
#include <melee/lb/lbtime.h>
#include <assert.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

static void date(int year, int month, int day, int wday)
{
    OSCalendarTime input = {.year=year, .mon=month-1, .mday=day,
                           .hour=12, .min=34, .sec=56};
    OSTime ticks = OSCalendarTimeToTicks(&input);
    OSCalendarTime output;
    OSTicksToCalendarTime(ticks, &output);
    assert(output.year==year && output.mon==month-1 && output.mday==day);
    assert(output.hour==12 && output.min==34 && output.sec==56);
    assert(output.wday==wday && output.msec==0 && output.usec==0);
}

static void expected_abort(int number)
{
    _exit(number == SIGABRT ? 86 : 90);
}

static void invalid_date(int kind)
{
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        assert(freopen("/dev/null", "w", stderr));
        signal(SIGABRT, expected_abort);
        OSCalendarTime calendar = {.year=2000000, .mon=0, .mday=1};
        if (kind == 0) OSTicksToCalendarTime(INT64_MIN, &calendar);
        else if (kind == 1) OSCalendarTimeToTicks(&calendar);
        else OSTicksToCalendarTime(0, NULL);
        _exit(91);
    }
    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 86);
}

int main(void)
{
    _Static_assert(sizeof(OSTick)==4 && sizeof(OSTime)==8, "game tick ABI");
    assert(OS_TIMER_CLOCK == 40500000);
    OSCalendarTime calendar;
    OSTicksToCalendarTime(0, &calendar);
    assert(calendar.year==2000 && calendar.mon==0 && calendar.mday==1);
    assert(calendar.wday==6 && calendar.yday==0 && calendar.sec==0);
    OSTicksToCalendarTime(-OSSecondsToTicks((OSTime)1), &calendar);
    assert(calendar.year==1999 && calendar.mon==11 && calendar.mday==31);
    assert(calendar.hour==23 && calendar.min==59 && calendar.sec==59);
    OSCalendarTime origin = {.year=0, .mon=0, .mday=1};
    OSTime origin_ticks = OSCalendarTimeToTicks(&origin);
    OSTicksToCalendarTime(origin_ticks, &calendar);
    assert(calendar.year==0 && calendar.mon==0 && calendar.mday==1);
    OSTicksToCalendarTime(INT64_MAX, &calendar);
    OSTime rounded = OSCalendarTimeToTicks(&calendar);
    assert(rounded <= INT64_MAX && INT64_MAX-rounded < 100);
    date(1900, 3, 1, 4); /* Century exception. */
    date(2000, 2, 29, 2);
    date(2024, 2, 29, 4);
    date(2100, 3, 1, 1);
    date(2400, 2, 29, 2);
    OSTicksToCalendarTime(OSMillisecondsToTicks((OSTime)123) +
                          OSMicrosecondsToTicks((OSTime)456), &calendar);
    assert(calendar.msec==123 && calendar.usec==456);
    OSCalendarTime normalized = {.year=2000, .mon=-1, .mday=32};
    assert(OSCalendarTimeToTicks(&normalized)==0);
    lbTime_8000B028(&calendar, 86400);
    assert(calendar.year==2000 && calendar.mon==0 && calendar.mday==2);

    time_t wall = time(NULL);
    OSTime before = OSGetTime();
    assert((OSTime)wall-946684800-1 <= OSTicksToSeconds(before));
    assert(OSTicksToSeconds(before) <= (OSTime)wall-946684800+1);
    for (int i=0; i<10000; ++i) {
        OSTime current = OSGetTime();
        assert(current >= before);
        before = current;
    }
    OSTick low = OSGetTick();
    OSTime after = OSGetTime();
    assert((u32)(low-(u32)before) <= (u64)(after-before));
    assert(lbTime_GetTimeInSeconds() >= OSTicksToSeconds(before));
    struct timespec delay = {.tv_nsec=10000000};
    assert(nanosleep(&delay, NULL)==0);
    assert(OSGetTime()-after >= OSMillisecondsToTicks((OSTime)9));
    for (int kind=0; kind<3; ++kind) invalid_date(kind);
    puts("Native time: calendar epoch/leap rules, original game date helpers, monotonic ticks and wall-time anchor passed");
    return 0;
}
