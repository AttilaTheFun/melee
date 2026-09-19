/* Include the original module to exercise its private HSD_OSInit memory stage
 * without substituting a renderer or pretending the whole engine booted. */
#include "../../src/sysdolphin/baselib/initialize.c"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    OSInit();
    uintptr_t start = (uintptr_t) OSGetArenaLo();
    uintptr_t end = (uintptr_t) OSGetArenaHi();
    assert(start > UINT32_MAX && (start & 31) == 0 && (end & 31) == 0);
    assert(end - start == OSGetPhysicalMemSize());
    assert(OSGetConsoleSimulatedMemSize() == end - start);
    assert(OSRoundUp32B(start + 1) == start + 32);
    assert(OSRoundDown32B(start + 31) == start);
    GXRenderModeObj mode = {.fbWidth=640, .xfbHeight=480};
    assert(HSD_AllocateXFB(4, &mode) == NULL);
    assert(HSD_AllocateXFB(-1, &mode) == NULL);
    void** buffers = HSD_AllocateXFB(2, &mode);
    assert(buffers && buffers[0] == (void*)start);
    assert((uintptr_t) buffers[1] == start + 640*480*2);
    assert(!buffers[2]);
    memset(buffers[0], 0xA5, 640*480*2);
    memset(buffers[1], 0x5A, 640*480*2);
    GXFifoObj* fifo = HSD_AllocateFifo(HSD_DEFAULT_FIFO_SIZE);
    assert((uintptr_t)fifo == start + 2*640*480*2);
    HSD_GXSetFifoObj(fifo);
    uintptr_t after_reservations = (uintptr_t) OSGetArenaLo();
    OSInit(); /* Idempotence must not reclaim already reserved ranges. */
    assert((uintptr_t)OSGetArenaLo() == after_reservations);
    HSD_OSInit();
    assert(OSGetArenaLo() == OSGetArenaHi());
    assert(HSD_GetHeap() >= 0 && HSD_GetHeap() != HSD_Synth_804D6018);
    assert(__OSCurrHeap == HSD_GetHeap());
    void* lo;
    void* hi;
    HSD_GetNextArena(&lo, &hi);
    assert((uintptr_t)lo == after_reservations + HSD_DEFAULT_AUDIO_SIZE);
    assert((uintptr_t)hi == end);
    assert(memReport.system == 0);
    assert(memReport.xfb == 2*640*480*2);
    assert(memReport.gxfifo == HSD_DEFAULT_FIFO_SIZE);
    assert(memReport.heap == end - (uintptr_t)lo);
    void* audio = OSAllocFromHeap(HSD_Synth_804D6018, 256);
    void* game = OSAllocFromHeap(HSD_GetHeap(), 4096);
    assert(audio && game && (uintptr_t)audio < (uintptr_t)lo);
    assert((uintptr_t)game >= (uintptr_t)lo && (uintptr_t)game + 4096 <= end);
    memset(audio, 1, 256); memset(game, 2, 4096);
    void* extra_fifo = HSD_AllocateFifo(1024);
    assert(extra_fifo && (uintptr_t)extra_fifo >= (uintptr_t)lo);
    OSFreeToHeap(HSD_GetHeap(), extra_fifo);
    OSFreeToHeap(HSD_GetHeap(), game);
    OSFreeToHeap(HSD_Synth_804D6018, audio);
    assert(OSCheckHeap(HSD_GetHeap()) > 0);
    assert(OSCheckHeap(HSD_Synth_804D6018) > 0);
    puts("Native startup: original HSD framebuffer/FIFO reservations, audio/game heap setup and full-width arena accounting passed");
    return 0;
}
