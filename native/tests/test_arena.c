#include <dolphin/os.h>
#include <dolphin/os/OSAlloc.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    unsigned char* storage = malloc(65536 + 128);
    assert(storage && (uintptr_t) storage > UINT32_MAX);
    memset(storage, 0xA5, 65536 + 128);
    unsigned char* begin = storage + 33;
    unsigned char* end = storage + 65535;
    OSSetArenaLo(begin);
    OSSetArenaHi(end);
    for (u32 align = 1; align <= 1024; align *= 2) {
        uintptr_t lo = (uintptr_t) OSGetArenaLo();
        uintptr_t hi = (uintptr_t) OSGetArenaHi();
        uintptr_t low_block = (uintptr_t) OSAllocFromArenaLo(17, align);
        uintptr_t high_block = (uintptr_t) OSAllocFromArenaHi(19, align);
        assert(low_block == ((lo + align - 1) & ~(uintptr_t)(align - 1)));
        assert(high_block == (((hi & ~(uintptr_t)(align - 1)) - 19) & ~(uintptr_t)(align - 1)));
        assert(low_block + 17 <= (uintptr_t) OSGetArenaLo());
        assert(high_block == (uintptr_t) OSGetArenaHi());
        assert((uintptr_t) OSGetArenaLo() <= high_block);
        memset((void*) low_block, 0x12, 17);
        memset((void*) high_block, 0x34, 19);
    }
    void* lo = OSGetArenaLo();
    void* hi = OSGetArenaHi();
    assert(!OSAllocFromArenaLo(UINT32_MAX, 32));
    assert(!OSAllocFromArenaHi(UINT32_MAX, 32));
    assert(!OSAllocFromArenaLo(1, 0));
    assert(!OSAllocFromArenaHi(1, 3));
    assert(OSGetArenaLo() == lo && OSGetArenaHi() == hi);

    /* The remaining region must be usable by the real native heap backend. */
    void* heap_start = OSInitAlloc(lo, hi, 1);
    assert(heap_start);
    int heap = OSCreateHeap(heap_start, hi);
    assert(heap >= 0);
    void* payload = OSAllocFromHeap(heap, 4096);
    assert(payload && (uintptr_t) payload >= (uintptr_t) lo);
    assert((uintptr_t) payload + 4096 <= (uintptr_t) hi);
    memset(payload, 0x56, 4096);
    OSFreeToHeap(heap, payload);
    assert(OSCheckHeap(heap) >= 4096);
    OSDestroyHeap(heap);

    /* Alignment padding itself cannot consume space outside the bounds. */
    uintptr_t aligned = ((uintptr_t) begin + 31) & ~(uintptr_t)31;
    OSSetArenaLo((void*) aligned);
    OSSetArenaHi((void*) (aligned + 31));
    assert(!OSAllocFromArenaLo(31, 32));
    assert(!OSAllocFromArenaHi(1, 32));
    OSSetArenaHi((void*) (aligned + 32));
    assert(OSAllocFromArenaLo(32, 32) == (void*) aligned);
    assert(OSGetArenaLo() == OSGetArenaHi());
    assert(!OSAllocFromArenaHi(1, 1));
    for (size_t i = 0; i < 33; ++i) assert(storage[i] == 0xA5);
    for (size_t i = 65535; i < 65536 + 128; ++i) assert(storage[i] == 0xA5);
    free(storage);
    puts("Native OS arena: full-width two-ended allocation, alignment, exhaustion and heap handoff passed");
    return 0;
}
