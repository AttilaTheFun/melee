#include "melee_vi.h"
#include <dolphin/vi.h>
#include <dolphin/os.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>

static unsigned phase, calls;
static uint32_t pre_count;
static char buffers[3];
static pthread_t display_owner;
static void display_post(uint32_t count)
{
    assert(count > 0 && !pthread_equal(pthread_self(), display_owner));
}
static void pre(uint32_t count)
{
    assert(phase == 0);
    phase = 1;
    pre_count = count;
    assert(VIGetRetraceCount() == count);
    if (calls == 0) {
        assert(melee_vi_presentation().framebuffer == NULL);
        VISetNextFrameBuffer(&buffers[0]);
        VISetBlack(false);
        melee_vi_flush();
        /* Unflushed changes must not overwrite the published snapshot. */
        VISetNextFrameBuffer(&buffers[1]);
        VISetBlack(true);
    }
}
static void post(uint32_t count)
{
    assert(phase == 1 && count == pre_count);
    phase = 0;
    MeleeVIPresentation p = melee_vi_presentation();
    assert(p.retrace == count);
    if (calls == 0) {
        assert(p.framebuffer == &buffers[0] && !p.black);
        melee_vi_flush(); /* Takes effect at the next retrace. */
        assert(melee_vi_presentation().framebuffer == &buffers[0]);
    } else {
        assert(p.framebuffer == &buffers[1] && p.black);
    }
    ++calls;
}
static void* waiter(void* unused)
{
    (void)unused;
    for (int i=0;i<3;++i) {
        uint32_t before=VIGetRetraceCount();
        VIWaitForRetrace();
        assert(VIGetRetraceCount()!=before);
    }
    return NULL;
}
int main(void)
{
    melee_vi_shutdown();
    assert(VISetPreRetraceCallback(pre) == NULL);
    assert(VISetPostRetraceCallback(post) == NULL);
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    melee_vi_init(); melee_vi_init();
    for (unsigned i = 0; i < 5; ++i) {
        /* Match original callers that sleep with interrupts disabled. */
        BOOL enabled = OSDisableInterrupts();
        uint32_t before = VIGetRetraceCount();
        VIWaitForRetrace();
        assert(OSDisableInterrupts() == false);
        assert(VIGetRetraceCount() != before && phase == 0 && calls > 0);
        OSRestoreInterrupts(enabled);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    int64_t elapsed=(end.tv_sec-start.tv_sec)*1000000000LL+end.tv_nsec-start.tv_nsec;
    assert(elapsed >= 50000000); /* Five waits cannot be a busy-loop fake. */
    BOOL enabled = OSDisableInterrupts();
    assert(calls >= 5);
    assert(VISetPreRetraceCallback(NULL) == pre);
    assert(VISetPostRetraceCallback(NULL) == post);
    melee_vi_configure(2, 0);
    assert(VIGetNextField() == 0 && VIGetDTVStatus() == 1);
    melee_vi_configure(0, 1);
    assert(VIGetNextField() == ((VIGetRetraceCount() & 1) ^ 1));
    OSRestoreInterrupts(enabled);
    pthread_t waiters[4];
    for (unsigned i=0;i<4;++i) assert(!pthread_create(&waiters[i],NULL,waiter,NULL));
    for (unsigned i=0;i<4;++i) assert(!pthread_join(waiters[i],NULL));
    melee_vi_shutdown();
    assert(VIGetRetraceCount() == 0);
    assert(melee_vi_presentation().black);
    melee_vi_init(); VIWaitForRetrace(); melee_vi_shutdown();
    /* A display link controls the phase without running callbacks on its
     * thread. No pulse means no retrace; disabling restores autonomous waits. */
    display_owner = pthread_self();
    VISetPostRetraceCallback(display_post);
    melee_vi_set_display_clock(true);
    melee_vi_init();
    const struct timespec frame = {.tv_nsec=20000000};
    nanosleep(&frame,NULL);
    assert(VIGetRetraceCount()==0);
    for (unsigned i=0;i<5;++i) {
        for (unsigned pulse=0;pulse<100;++pulse) melee_vi_display_tick();
        unsigned ticks=0;
        const struct timespec poll = {.tv_nsec=1000000};
        while (VIGetRetraceCount()!=i+1 && ticks++<1000) nanosleep(&poll,NULL);
        assert(ticks<1000);
        nanosleep(&frame,NULL);
        assert(VIGetRetraceCount()==i+1);
    }
    melee_vi_set_display_clock(false);
    VIWaitForRetrace();
    assert(VIGetRetraceCount()>5);
    melee_vi_set_display_clock(true);
    melee_vi_shutdown(); /* A stopped display link cannot strand shutdown. */
    puts("Native VI: timed retrace, callback order, flush latching, nested interrupt wait, display pulses/coalescing, clock handoff and restart passed");
}
