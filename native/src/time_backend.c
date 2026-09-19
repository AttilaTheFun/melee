/* Native timebase in GameCube ticks since 2000-01-01 UTC. Anchor wall time
 * once, then use monotonic elapsed time so clock corrections cannot reverse
 * game timers. This does not schedule frames or emulate OS alarms. */
#include <dolphin/os.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

static pthread_once_t time_once = PTHREAD_ONCE_INIT;
static struct timespec monotonic_origin;
static OSTime tick_origin;

static struct timespec read_clock(clockid_t clock)
{
    struct timespec result;
    if (clock_gettime(clock, &result) != 0) {
        OSPanic(__FILE__, __LINE__, "Unable to read native clock");
    }
    return result;
}

static void initialize_time(void)
{
    monotonic_origin = read_clock(CLOCK_MONOTONIC);
    struct timespec wall = read_clock(CLOCK_REALTIME);
    __int128 ticks = ((__int128) wall.tv_sec - 946684800) * OS_TIMER_CLOCK +
                     (__int128) wall.tv_nsec * OS_TIMER_CLOCK / 1000000000;
    if (ticks < INT64_MIN || ticks > INT64_MAX) {
        OSPanic(__FILE__, __LINE__, "Native clock is outside GameCube tick range");
    }
    tick_origin = (OSTime) ticks;
}

OSTime OSGetTime(void)
{
    pthread_once(&time_once, initialize_time);
    struct timespec now = read_clock(CLOCK_MONOTONIC);
    __int128 elapsed_ns = ((__int128) now.tv_sec - monotonic_origin.tv_sec) *
                          1000000000 + now.tv_nsec - monotonic_origin.tv_nsec;
    __int128 ticks = tick_origin + elapsed_ns * OS_TIMER_CLOCK / 1000000000;
    if (ticks < INT64_MIN || ticks > INT64_MAX) {
        OSPanic(__FILE__, __LINE__, "Native timebase overflow");
    }
    return (OSTime) ticks;
}

OSTick OSGetTick(void)
{
    return (OSTick) OSGetTime();
}
