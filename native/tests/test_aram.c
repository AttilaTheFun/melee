#include "melee_aram.h"
#include <dolphin/ar.h>
#include <dolphin/os.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
static _Alignas(32) unsigned char source[0x3000], output[0x3000], extra[32];
static ARQRequest low, high, readback;
static u32 base;
static pthread_mutex_t done_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t done_condition = PTHREAD_COND_INITIALIZER;
static int completed;
static void callback(ARQRequest* request)
{
    int level = OSDisableInterrupts(); assert(!level); OSRestoreInterrupts(level);
    pthread_mutex_lock(&done_lock);
    if (request == &high) { assert(completed == 0); ++completed; }
    else if (request == &low) {
        assert(completed == 1); ++completed;
        ARQPostRequest(&readback, 9, ARQ_TYPE_ARAM_TO_MRAM, ARQ_PRIORITY_HIGH,
                       base, (ARAddress)output, sizeof(output), callback);
    } else {
        assert(request == &readback && completed == 2);
        assert(!memcmp(source, output, sizeof(source)));
        ++completed; pthread_cond_signal(&done_condition);
    }
    pthread_mutex_unlock(&done_lock);
}
int main(void)
{
    u32 stack[4], length;
    unsigned char snapshot[321];
    memset(snapshot, 0xA7, sizeof(snapshot));
    assert(!melee_aram_read(0, snapshot, sizeof(snapshot)));
    assert(snapshot[0] == 0xA7);
    assert(!ARCheckInit()); assert(ARInit(stack, 4) == 0x4000);
    assert(ARGetSize() == 16 * 1024 * 1024 && ARCheckInit());
    base = ARAlloc(0x4000); assert(base == ARGetBaseAddress());
    assert(ARAlloc(32) == base + 0x4000);
    assert(ARFree(&length) == base + 0x4000 && length == 32);
    assert((uintptr_t)source > UINT32_MAX);
    for (size_t i = 0; i < sizeof(source); ++i) source[i] = (unsigned char)(i * 7);
    ARQInit(); ARQSetChunkSize(1023); assert(ARQGetChunkSize() == 1024);
    int level = OSDisableInterrupts();
    ARQPostRequest(&low, 7, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_LOW,
                   (ARAddress)source, base, sizeof(source), callback);
    ARQPostRequest(&high, 8, ARQ_TYPE_MRAM_TO_ARAM, ARQ_PRIORITY_HIGH,
                   (ARAddress)extra, base + 0x3000, sizeof(extra), callback);
    assert(ARGetDMAStatus() == 0x200 && completed == 0);
    /* The mixer may hold this same gate. Reading must not wait for the DMA
     * worker, which needs the gate before it can commit its pending copy. */
    assert(melee_aram_read(base, snapshot, sizeof(snapshot)));
    for (size_t i = 0; i < sizeof(snapshot); ++i) assert(snapshot[i] == 0);
    OSRestoreInterrupts(level);
    struct timespec deadline; clock_gettime(CLOCK_REALTIME, &deadline); deadline.tv_sec += 10;
    pthread_mutex_lock(&done_lock);
    while (completed != 3) assert(!pthread_cond_timedwait(&done_condition, &done_lock, &deadline));
    pthread_mutex_unlock(&done_lock);
    assert(ARGetDMAStatus() == 0);
    memset(snapshot, 0xA7, sizeof(snapshot));
    assert(melee_aram_read(base + 3, snapshot + 1, 317));
    assert(!memcmp(snapshot + 1, source + 3, 317));
    assert(snapshot[0] == 0xA7 && snapshot[318] == 0xA7);
    assert(melee_aram_read(ARGetSize(), NULL, 0));
    assert(melee_aram_read(ARGetSize() - 3, snapshot, 3));
    assert(snapshot[0] == 0 && snapshot[1] == 0 && snapshot[2] == 0);
    memset(snapshot, 0xA7, sizeof(snapshot));
    assert(!melee_aram_read(ARGetSize() - 3, snapshot, 4));
    assert(!melee_aram_read(UINT32_MAX, snapshot, 1));
    assert(!melee_aram_read(base, snapshot, SIZE_MAX));
    assert(!melee_aram_read(base, NULL, 1));
    for (size_t i = 0; i < sizeof(snapshot); ++i) assert(snapshot[i] == 0xA7);
    assert(ARFree(&length) == base && length == 0x4000);
    level = OSDisableInterrupts(); ARQReset(); ARReset(); OSRestoreInterrupts(level);
    assert(!ARCheckInit() && !ARGetSize());
    assert(!melee_aram_read(base, snapshot, sizeof(snapshot)));
    for (size_t i = 0; i < sizeof(snapshot); ++i) assert(snapshot[i] == 0xA7);
    puts("Original ARQ scheduling, chunking, priorities, native ARAM round trip and bounded mixer reads passed");
}
