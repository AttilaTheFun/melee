#include "melee_dvd.h"
#include "melee_archive.h"
#include <sysdolphin/baselib/devcom.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/ar.h>
#include <dolphin/dvd.h>
#include <dolphin/os.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define PAYLOAD_SIZE 0x90020
static unsigned char image[PAYLOAD_SIZE + 0x1000];
static _Alignas(32) unsigned char output[PAYLOAD_SIZE];
static pthread_mutex_t done_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t done_condition = PTHREAD_COND_INITIALIZER;
static int done;
static struct { int stage, entry; u32 aram; } context;
static void put32(unsigned char* p, u32 v)
{ p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
static void callback(int request, HSD_DevComArg argument, void* buffer, bool canceled)
{
    (void)request;
    assert((void*)argument == &context && !canceled);
    switch (context.stage++) {
    case 0:
        assert(!memcmp(output, image + 0x1000, sizeof(output)));
        HSD_DevComRequest(context.entry, 0, context.aram, PAYLOAD_SIZE, 0x23, 1, callback, &context);
        break;
    case 1:
        memset(output, 0, sizeof(output));
        HSD_DevComRequest(0, context.aram, (uintptr_t)output, PAYLOAD_SIZE, 0x19, 0, callback, &context);
        break;
    case 2:
        assert(!memcmp(output, image + 0x1000, sizeof(output)));
        HSD_DevComRequest(0, (uintptr_t)output, context.aram + PAYLOAD_SIZE, PAYLOAD_SIZE, 0xB, 0, callback, &context);
        break;
    case 3:
        HSD_DevComRequest(0, context.aram + PAYLOAD_SIZE, context.aram, PAYLOAD_SIZE, 0x1B, 0, callback, &context);
        break;
    case 4:
        HSD_DevComRequest(0, context.aram, 0, 32, 0x1A, 0, callback, &context);
        break;
    case 5:
        assert(buffer && !memcmp(buffer, image + 0x1000, 32));
        HSD_DevComRequest(0, 0, context.aram, 0x4020, 3, 0, callback, &context);
        break;
    case 6:
        HSD_DevComRequest(0, context.aram, (uintptr_t)output, 0x4020, 0x19, 0, callback, &context);
        break;
    case 7:
        for (size_t i = 0; i < 0x4020; ++i) assert(output[i] == 0);
        HSD_DevComRequest(context.entry, 0, 0, 32, 0x22, 0, callback, &context);
        break;
    case 8:
        assert(buffer && !memcmp(buffer, image + 0x1000, 32));
        pthread_mutex_lock(&done_lock); done = 1; pthread_cond_signal(&done_condition); pthread_mutex_unlock(&done_lock);
        break;
    default: assert(0);
    }
}
static u32 real_length;
static void real_callback(int request, HSD_DevComArg argument, void* buffer, bool canceled)
{
    (void)request; (void)buffer;
    assert((void*)argument == &context && !canceled);
    MeleeArchive archive; uint32_t root;
    assert(melee_archive_open(&archive, output, real_length));
    assert(melee_archive_find(&archive, "PlyFox5K_Share_joint", &root));
    pthread_mutex_lock(&done_lock); done = 1; pthread_cond_signal(&done_condition); pthread_mutex_unlock(&done_lock);
}
int main(int argc, char** argv)
{
    static _Alignas(32) u8 heap[1048576]; u32 stack[4];
    assert(OSInitAlloc(heap, heap + sizeof(heap), 1));
    HSD_SetHeap(OSCreateHeap(heap, heap + sizeof(heap)));
    ARInit(stack, 4); ARQInit(); context.aram = ARAlloc(PAYLOAD_SIZE * 2);
    memcpy(image, "GALE01\0\2", 8); put32(image + 0x1c, 0xc2339f3d);
    put32(image + 0x424, 0x800); put32(image + 0x428, 34);
    put32(image + 0x800, 0x01000000); put32(image + 0x808, 2);
    put32(image + 0x810, 0x1000); put32(image + 0x814, PAYLOAD_SIZE);
    memcpy(image + 0x818, "asset.dat", 10);
    for (size_t i = 0; i < PAYLOAD_SIZE; ++i) image[0x1000 + i] = (unsigned char)(i * 7 + 3);
    char path[] = "/tmp/melee-devcom-XXXXXX"; int fd = mkstemp(path); assert(fd >= 0);
    assert(write(fd, image, sizeof(image)) == sizeof(image)); close(fd);
    assert(melee_dvd_mount(path)); context.entry = DVDConvertPathToEntrynum("asset.dat");
    assert(context.entry == 1 && (uintptr_t)&context > UINT32_MAX);
    HSD_DevComRequest(context.entry, 0, (uintptr_t)output, sizeof(output), 0x21, 0, callback, &context);
    struct timespec deadline; clock_gettime(CLOCK_REALTIME, &deadline); deadline.tv_sec += 20;
    pthread_mutex_lock(&done_lock);
    while (!done) assert(!pthread_cond_timedwait(&done_condition, &done_lock, &deadline));
    pthread_mutex_unlock(&done_lock);
    int level = OSDisableInterrupts();
    for (int i = 0; i < 4; ++i) assert(!HSD_DevComIsBusy(i));
    assert(context.stage == 9);
    /* Request pools must survive replacement and reuse of the scene heap. */
    OSDestroyHeap(HSD_GetHeap());
    memset(heap, 0xa5, sizeof(heap));
    HSD_SetHeap(OSCreateHeap(heap, heap + sizeof(heap)));
    context.stage = 0; done = 0;
    HSD_DevComRequest(context.entry, 0, (uintptr_t)output, sizeof(output), 0x21, 0, callback, &context);
    OSRestoreInterrupts(level);
    clock_gettime(CLOCK_REALTIME, &deadline); deadline.tv_sec += 20;
    pthread_mutex_lock(&done_lock);
    while (!done) assert(!pthread_cond_timedwait(&done_condition, &done_lock, &deadline));
    pthread_mutex_unlock(&done_lock);
    level = OSDisableInterrupts();
    for (int i = 0; i < 4; ++i) assert(!HSD_DevComIsBusy(i));
    assert(context.stage == 9 && melee_dvd_unmount());
    OSRestoreInterrupts(level);
    if (argc == 2) {
        assert(melee_dvd_mount(argv[1])); DVDFileInfo info;
        int entry = DVDConvertPathToEntrynum("PlFxNr.dat");
        assert(DVDFastOpen(entry, &info)); real_length = info.length; assert(DVDClose(&info));
        size_t size = ((size_t)real_length + 31) & ~(size_t)31;
        assert(size <= sizeof(output)); done = 0;
        HSD_DevComRequest(entry, 0, (uintptr_t)output, size, 0x21, 0, real_callback, &context);
        clock_gettime(CLOCK_REALTIME, &deadline); deadline.tv_sec += 20;
        pthread_mutex_lock(&done_lock);
        while (!done) assert(!pthread_cond_timedwait(&done_condition, &done_lock, &deadline));
        pthread_mutex_unlock(&done_lock);
        level = OSDisableInterrupts(); assert(melee_dvd_unmount()); OSRestoreInterrupts(level);
        puts("Original HSD queue loaded Fox's real costume archive from CISO");
    }
    level = OSDisableInterrupts(); ARQReset(); ARReset(); OSRestoreInterrupts(level); unlink(path);
    puts("Original HSD DevCom: all eight transfer modes, chunking and native callback pointers passed");
}
