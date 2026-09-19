#ifndef MELEE_NATIVE
#include "__os.h"
#else
#include <stdint.h>
#endif

#include <macros.h>
#include <dolphin/exi.h>
#include <dolphin/os.h>

// End of each month in standard year
static int YearDays[MONTH_MAX] = { 0,   31,  59,  90,  120, 151,
                                   181, 212, 243, 273, 304, 334 };
// End of each month in leap year
static int LeapYearDays[MONTH_MAX] = { 0,   31,  60,  91,  121, 152,
                                       182, 213, 244, 274, 305, 335 };

#ifndef MELEE_NATIVE
asm long long OSGetTime(void)
{
    // clang-format off
jump:
    nofralloc

    mftbu r3
    mftb r4

    // Check for possible carry from TBL to TBU
    mftbu r5
    cmpw r3, r5
    bne OSGetTime

    blr
    // clang-format on
}

asm unsigned long OSGetTick(void)
{
    // clang-format off
    nofralloc

    mftb r3
    blr
    // clang-format on
}

asm static void __SetTime(long long time)
{
    // clang-format off
    nofralloc
    li r5, 0
    mttbl r5
    mttbu r3
    mttbl r4
    blr
    // clang-format on
}

void __OSSetTime(long long time)
{
    int enabled;
    long long* timeAdjustAddr;

    timeAdjustAddr = (long long*) 0x800030D8;
    enabled = OSDisableInterrupts();

    *timeAdjustAddr += OSGetTime() - time;
    __SetTime(time);
    EXIProbeReset();
    OSRestoreInterrupts(enabled);
}

long long __OSGetSystemTime()
{
    int enabled;
    long long* timeAdjustAddr;
    long long result;

    timeAdjustAddr = (long long*) 0x800030D8;
    enabled = OSDisableInterrupts();

    result = OSGetTime() + *timeAdjustAddr;
    OSRestoreInterrupts(enabled);
    return result;
}

long long __OSTimeToSystemTime(s64 time)
{
    int enabled;
    long long* timeAdjustAddr;
    long long sysTime;
    u8 _[4];

    timeAdjustAddr = (long long*) 0x800030D8;
    enabled = OSDisableInterrupts();
    sysTime = *timeAdjustAddr + time;
    OSRestoreInterrupts(enabled);
    return sysTime;
}

asm void __OSSetTick(register unsigned long newTicks)
{
    // clang-format off
    nofralloc
    mttbl newTicks
    blr
    // clang-format on
}

#endif /* !MELEE_NATIVE: native timebase is provided by time_backend.c. */

static int IsLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int GetYearDays(int year, int mon)
{
    int* md = (IsLeapYear(year)) ? LeapYearDays : YearDays;

    return md[mon];
}

static int GetLeapDays(int year)
{
    ASSERTLINE(260, 0 <= year);

    if (year < 1) {
        return 0;
    }
    return (year + 3) / 4 - (year - 1) / 100 + (year - 1) / 400;
}

static void GetDates(int days, OSCalendarTime* td)
{
    int year;
    int n;
    int month;
    int* md;

    ASSERTLINE(285, 0 <= days);

    td->wday = (days + 6) % WEEK_DAY_MAX;

    for (year = days / YEAR_DAY_MAX;
         days < (n = year * YEAR_DAY_MAX + GetLeapDays(year)); year--)
    {
        ;
    }

    days -= n;
    td->year = year;
    td->yday = days;

    md = IsLeapYear(year) ? LeapYearDays : YearDays;
    for (month = MONTH_MAX; days < md[--month];) {
        ;
    }
    td->mon = month;
    td->mday = days - md[month] + 1;
}

void OSTicksToCalendarTime(long long ticks, OSCalendarTime* td)
{
    int days;
    int secs;
    long long d;

#ifdef MELEE_NATIVE
    /* The original calendar algorithm starts at year zero. Reject invalid
     * input before signed subtraction or a negative month-table index. */
    if (td == NULL || ticks < -(s64) 0xEB1E1BF80ULL * OS_TIMER_CLOCK) {
        OSPanic(__FILE__, __LINE__, "Calendar ticks precede year zero or output is null");
    }
#endif
    d = ticks % OS_SEC_TO_TICKS(1);
    if (d < 0) {
        d += OS_SEC_TO_TICKS(1);
        ASSERTLINE(330, 0 <= d);
    }

    td->usec = OS_TICKS_TO_USEC(d) % USEC_MAX;
    td->msec = OS_TICKS_TO_MSEC(d) % MSEC_MAX;

    ASSERTLINE(334, 0 <= td->usec);
    ASSERTLINE(335, 0 <= td->msec);

    ticks -= d;

    ASSERTLINE(338, ticks % OSSecondsToTicks(1) == 0);
    ASSERTLINE(342, 0 <= OSTicksToSeconds(ticks) / 86400 + BIAS &&
                        OSTicksToSeconds(ticks) / 86400 + BIAS <= INT_MAX);

    days = (OS_TICKS_TO_SEC(ticks) / SECS_IN_DAY) + BIAS;
    secs = OS_TICKS_TO_SEC(ticks) % SECS_IN_DAY;
    if (secs < 0) {
        days -= 1;
        secs += SECS_IN_DAY;
        ASSERTLINE(349, 0 <= secs);
    }

    GetDates(days, td);
    td->hour = secs / 60 / 60;
    td->min = secs / 60 % 60;
    td->sec = secs % 60;
}

OSTime OSCalendarTimeToTicks(OSCalendarTime* td)
{
    long long secs;
    int ov_mon;
    int mon;
    int year;

#ifdef MELEE_NATIVE
    if (td == NULL) {
        OSPanic(__FILE__, __LINE__, "Calendar input is null");
    }
#endif
    ov_mon = td->mon / MONTH_MAX;
    mon = td->mon - (ov_mon * MONTH_MAX);

    if (mon < 0) {
        mon += MONTH_MAX;
        ov_mon--;
    }

    ASSERTLINE(0x182, (ov_mon <= 0 && 0 <= td->year + ov_mon) ||
                          (0 < ov_mon && td->year <= INT_MAX - ov_mon));

#ifdef MELEE_NATIVE
    s64 native_year = (s64) td->year + ov_mon;
    if (native_year < 0 || native_year > INT32_MAX - 3) {
        OSPanic(__FILE__, __LINE__, "Calendar year is outside native tick range");
    }
#endif
    year = td->year + ov_mon;

#ifdef MELEE_NATIVE
    secs = (s64) SECS_IN_YEAR * year +
           (s64) SECS_IN_DAY * ((s64) GetLeapDays(year) +
                                GetYearDays(year, mon) + td->mday - 1) +
           (s64) SECS_IN_HOUR * td->hour +
           (s64) SECS_IN_MIN * td->min + td->sec - (s64) 0xEB1E1BF80ULL;
#else
    // clang-format off
    secs = (s64)SECS_IN_YEAR * year +
              (s64)SECS_IN_DAY * (GetLeapDays(year) + GetYearDays(year, mon) + td->mday - 1) +
              (s64)SECS_IN_HOUR * td->hour +
              (s64)SECS_IN_MIN * td->min +
              td->sec -
              (s64)0xEB1E1BF80ULL;
    // clang-format on
#endif

#ifdef MELEE_NATIVE
    __int128 result = (__int128) secs * OS_TIMER_CLOCK +
                     (__int128) td->msec * (OS_TIMER_CLOCK / 1000) +
                     (__int128) td->usec * (OS_TIMER_CLOCK / 125000) / 8;
    if (result < INT64_MIN || result > INT64_MAX) {
        OSPanic(__FILE__, __LINE__, "Calendar conversion overflows native ticks");
    }
    return (OSTime) result;
#else
    return OS_SEC_TO_TICKS(secs) + OS_MSEC_TO_TICKS((s64) td->msec) +
           OS_USEC_TO_TICKS((s64) td->usec);
#endif
}
