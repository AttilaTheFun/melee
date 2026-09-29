/* Exercise the actual optimized movie cleanup with an outstanding read. */
#include "../../src/melee/lb/lbmthp.c"
#include "melee_vi.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

static unsigned canceled, flushed, freed;
static char allocation;
void OSCancelAlarm(OSAlarm* alarm)
{
    assert(alarm == &MoviePlayer.alarm && !MoviePlayer.unk_110);
    ++canceled;
}
void HSD_VIWaitXFBFlush(void) { ++flushed; }
void HSD_Free(void* memory)
{
    assert(memory == &allocation && !MoviePlayer.unk_110);
    assert(canceled == flushed && canceled == freed + 1);
    ++freed;
}
static void* complete_read(void* unused)
{
    (void) unused;
    /* Wait until cleanup has stopped new reads, then emulate the DVD
     * completion under the same interrupt gate used by dvd_backend.c. */
    for (;;) {
        BOOL intr = OSDisableInterrupts();
        if (!MoviePlayer.unk_70) {
            MoviePlayer.unk_110 = 0;
            OSRestoreInterrupts(intr);
            return NULL;
        }
        OSRestoreInterrupts(intr);
        struct timespec delay = {0, 1000000};
        nanosleep(&delay, NULL);
    }
}
int main(void)
{
    melee_vi_init();
    for (unsigned nested = 0; nested < 2; ++nested) {
        MoviePlayer.power = MoviePlayer.unk_70 = MoviePlayer.unk_110 = 1;
        MoviePlayer.unk_140 = &allocation;
        BOOL intr = nested ? OSDisableInterrupts() : true;
        pthread_t worker;
        assert(pthread_create(&worker, NULL, complete_read, NULL) == 0);
        lbMthp_8001F800();
        assert(!MoviePlayer.power && canceled == nested + 1);
        OSRestoreInterrupts(intr);
        assert(pthread_join(worker, NULL) == 0);
        lbMthp_8001F800(); /* Repeated cleanup does not free twice. */
        assert(freed == nested + 1);
    }
    melee_vi_shutdown();
    puts("Movie cleanup drains pending reads, including with interrupts held.");
    return 0;
}
