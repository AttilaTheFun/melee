/* A monotonic NTSC retrace clock for the original HSD loop. This supplies
 * interrupt ordering and latched VI state; GPU presentation is a consumer. */
#include "melee_vi.h"
#include <dolphin/vi.h>
#include <dolphin/os.h>
#include <pthread.h>
#include "melee_cond.h"
#include <time.h>
#include <stdlib.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
#ifdef __APPLE__
static pthread_cond_t wake = PTHREAD_COND_INITIALIZER;
#else
static pthread_cond_t wake;
__attribute__((constructor)) static void init_condition(void)
{
    melee_cond_init_monotonic(&wake);
}
#endif
static pthread_t worker;
static bool running, stopping;
static bool display_clock, display_pending;
static VIRetraceCallback pre_callback, post_callback;
static MeleeVIPresentation current = {NULL, true, 0};
static void* next_buffer;
static bool next_black = true;
static unsigned dirty, pending;
static void* flushed_buffer;
static bool flushed_black;
static uint32_t configured_mode;
static uint16_t configured_y;
static _Thread_local bool in_retrace;

static uint64_t now_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000 + t.tv_nsec;
}

static void retrace(void)
{
    BOOL enabled = OSDisableInterrupts();
    in_retrace = true;
    pthread_mutex_lock(&lock);
    uint32_t count = ++current.retrace;
    VIRetraceCallback pre = pre_callback;
    pthread_mutex_unlock(&lock);
    if (pre) pre(count);
    /* A pre-callback flush affects this retrace, as in the SDK handler. */
    pthread_mutex_lock(&lock);
    if (pending & 1) current.framebuffer = flushed_buffer;
    if (pending & 2) current.black = flushed_black;
    pending = 0;
    VIRetraceCallback post = post_callback;
    pthread_mutex_unlock(&lock);
    if (post) post(count);
    in_retrace = false;
    /* Wake only after both callbacks; waiters reacquire the interrupt gate. */
    pthread_mutex_lock(&lock);
    pthread_cond_broadcast(&wake);
    pthread_mutex_unlock(&lock);
    OSRestoreInterrupts(enabled);
}

static void* clock_main(void* unused)
{
    (void)unused;
    const uint64_t period = 16683333; /* 60000/1001 Hz; no wall-clock jumps. */
    uint64_t deadline = now_ns() + period;
    uint64_t last_display = 0;
    unsigned display_phase = 1000;
    for (;;) {
        pthread_mutex_lock(&lock);
        while (!stopping) {
            uint64_t now = now_ns();
            if (display_clock) {
                if (display_pending) {
                    display_pending = false;
                    /* Pace from 60 Hz display pulses, not their jittery
                     * worker wake times. Preserve NTSC's 1000/1001 ratio:
                     * one display repeat per 1001 ticks, rather than bursts
                     * of skipped ticks when a callback arrives slightly early.
                     * Ignore accidental pulse bursts; never run catch-up frames. */
                    if (now - last_display >= period / 2) {
                        last_display = now;
                        display_phase += 1000;
                        if (display_phase >= 1001) {
                            display_phase -= 1001;
                            break;
                        }
                    }
                }
                pthread_cond_wait(&wake, &lock);
                continue;
            }
            if (now >= deadline) break;
            uint64_t remaining = deadline - now;
            struct timespec relative = {(time_t)(remaining / 1000000000),
                                        (long)(remaining % 1000000000)};
            melee_cond_wait_relative(&wake, &lock, &relative);
        }
        bool stop = stopping;
        pthread_mutex_unlock(&lock);
        if (stop) break;
        retrace();
        deadline += period;
        /* Suspension or a long callback drops elapsed ticks, without a burst. */
        uint64_t now = now_ns();
        if (deadline <= now) deadline = now + period;
    }
    return NULL;
}

void melee_vi_set_display_clock(bool enabled)
{
    pthread_mutex_lock(&lock);
    display_clock = enabled;
    display_pending = false;
    pthread_cond_broadcast(&wake);
    pthread_mutex_unlock(&lock);
}
void melee_vi_display_tick(void)
{
    pthread_mutex_lock(&lock);
    if (display_clock) {
        display_pending = true;
        pthread_cond_broadcast(&wake);
    }
    pthread_mutex_unlock(&lock);
}

void melee_vi_init(void)
{
    pthread_mutex_lock(&lock);
    if (!running) {
        stopping = false;
        if (pthread_create(&worker, NULL, clock_main, NULL) != 0) abort();
        running = true;
    }
    pthread_mutex_unlock(&lock);
}

void melee_vi_shutdown(void)
{
    if (in_retrace) abort();
    /* Joining while holding the interrupt gate can strand the clock in its
     * callback entry. Lifecycle calls belong to the host owner. */
    BOOL caller_enabled = OSDisableInterrupts();
    if (!caller_enabled) abort();
    OSRestoreInterrupts(caller_enabled);
    pthread_mutex_lock(&lock);
    bool join = running;
    stopping = true;
    pthread_cond_broadcast(&wake);
    pthread_mutex_unlock(&lock);
    if (join) pthread_join(worker, NULL);
    BOOL enabled = OSDisableInterrupts();
    pthread_mutex_lock(&lock);
    running = false;
    display_clock = display_pending = false;
    pre_callback = post_callback = NULL;
    current = (MeleeVIPresentation){NULL, true, 0};
    next_buffer = flushed_buffer = NULL;
    next_black = true;
    dirty = pending = 0;
    configured_mode = configured_y = 0;
    pthread_mutex_unlock(&lock);
    OSRestoreInterrupts(enabled);
}

void melee_vi_configure(uint32_t mode, uint16_t y)
{
    BOOL enabled = OSDisableInterrupts();
    configured_mode = mode;
    configured_y = y;
    OSRestoreInterrupts(enabled);
}

void VISetNextFrameBuffer(void* buffer)
{
    BOOL enabled = OSDisableInterrupts();
    pthread_mutex_lock(&lock);
    next_buffer = buffer; dirty |= 1;
    pthread_mutex_unlock(&lock);
    OSRestoreInterrupts(enabled);
}
void VISetBlack(BOOL black)
{
    BOOL enabled = OSDisableInterrupts();
    pthread_mutex_lock(&lock);
    next_black = black != 0; dirty |= 2;
    pthread_mutex_unlock(&lock);
    OSRestoreInterrupts(enabled);
}
void melee_vi_flush(void)
{
    BOOL enabled = OSDisableInterrupts();
    pthread_mutex_lock(&lock);
    if (dirty & 1) flushed_buffer = next_buffer;
    if (dirty & 2) flushed_black = next_black;
    pending |= dirty;
    dirty = 0;
    pthread_mutex_unlock(&lock);
    OSRestoreInterrupts(enabled);
}
MeleeVIPresentation melee_vi_presentation(void)
{
    BOOL enabled = OSDisableInterrupts();
    pthread_mutex_lock(&lock);
    MeleeVIPresentation result = current;
    pthread_mutex_unlock(&lock);
    OSRestoreInterrupts(enabled);
    return result;
}
u32 VIGetRetraceCount(void) { return melee_vi_presentation().retrace; }
u32 VIGetDTVStatus(void) { return 1; } /* Native display supports progressive output. */
u32 VIGetNextField(void)
{
    BOOL enabled = OSDisableInterrupts();
    u32 field = (configured_mode & 3) == 0 ? (current.retrace & 1) : 0;
    field ^= configured_y & 1;
    OSRestoreInterrupts(enabled);
    return field;
}
VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback callback)
{
    BOOL enabled = OSDisableInterrupts();
    VIRetraceCallback old = pre_callback; pre_callback = callback;
    OSRestoreInterrupts(enabled); return old;
}
VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback callback)
{
    BOOL enabled = OSDisableInterrupts();
    VIRetraceCallback old = post_callback; post_callback = callback;
    OSRestoreInterrupts(enabled); return old;
}
void VIWaitForRetrace(void)
{
    if (in_retrace) abort();
    melee_vi_init();
    BOOL enabled = OSDisableInterrupts();
    pthread_mutex_lock(&lock);
    uint32_t count = current.retrace;
    /* Sleeping yields the gate even when the caller entered with it held. */
    OSRestoreInterrupts(true);
    while (count == current.retrace && !stopping)
        pthread_cond_wait(&wake, &lock);
    pthread_mutex_unlock(&lock);
    OSDisableInterrupts();
    OSRestoreInterrupts(enabled);
}
