#include "melee_dvd.h"
#include "melee_archive.h"
#include <dolphin/dvd.h>
#include <dolphin/os.h>
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
extern size_t lbFile_8001634C(int fileno);
static uint8_t image[4096];
static _Alignas(32) uint8_t output[64], second[64];
static pthread_mutex_t done_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t done_condition = PTHREAD_COND_INITIALIZER;
static atomic_int calls;
static int done, expect_error;
static pthread_t main_thread;
static void put32(uint8_t* p, uint32_t v)
{ p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
static void completed(s32 result, DVDFileInfo* info)
{
    assert(!pthread_equal(main_thread, pthread_self()));
    BOOL previous = OSDisableInterrupts(); assert(!previous); OSRestoreInterrupts(previous);
    int n = atomic_fetch_add(&calls, 1) + 1;
    if (expect_error) {
        assert(result == DVD_RESULT_FATAL_ERROR && DVDGetFileInfoStatus(info) == DVD_STATE_FATAL_ERROR);
        assert(DVDGetTransferredSize(info) == 0);
    } else {
        assert(result == (n == 1 ? 64 : 32));
        assert(DVDGetFileInfoStatus(info) == DVD_STATE_END);
        assert(DVDGetTransferredSize(info) == result);
        if (n == 1) {
            assert(!memcmp(output, image + 0xc00, 64));
            assert(DVDReadAsyncPrio(info, second, 32, 4, completed, 0));
            return;
        }
        assert(!memcmp(second, image + 0xc04, 32));
    }
    assert(DVDClose(info));
    assert(melee_dvd_unmount());
    pthread_mutex_lock(&done_lock); done = 1; pthread_cond_signal(&done_condition); pthread_mutex_unlock(&done_lock);
}
static void wait_done(void)
{
    struct timespec deadline; clock_gettime(CLOCK_REALTIME, &deadline); deadline.tv_sec += 10;
    pthread_mutex_lock(&done_lock);
    while (!done) assert(!pthread_cond_timedwait(&done_condition, &done_lock, &deadline));
    pthread_mutex_unlock(&done_lock);
}
static int priority_count;
static void priority_completed(s32 result, DVDFileInfo* info)
{
    assert(result == 32);
    assert((uintptr_t)info->cb.userData == (priority_count == 0 ? 2 : 1));
    assert(DVDClose(info));
    if (++priority_count == 2) {
        assert(melee_dvd_unmount());
        pthread_mutex_lock(&done_lock); done = 1; pthread_cond_signal(&done_condition); pthread_mutex_unlock(&done_lock);
    }
}
int main(int argc, char** argv)
{
    main_thread = pthread_self();
    memcpy(image, "GALE01\0\2", 8); put32(image + 0x1c, 0xc2339f3d);
    put32(image + 0x424, 0x800); put32(image + 0x428, 34);
    put32(image + 0x800, 0x01000000); put32(image + 0x808, 2);
    put32(image + 0x810, 0xc00); put32(image + 0x814, 59); memcpy(image + 0x818, "asset.dat", 10);
    for (int i = 0; i < 64; ++i) image[0xc00+i] = i;
    char path[] = "/tmp/melee-dvd-XXXXXX"; int fd = mkstemp(path); assert(fd >= 0);
    assert(write(fd, image, sizeof(image)) == sizeof(image));
    assert(DVDGetDriveStatus() == DVD_STATE_NO_DISK && !DVDCheckDisk());
    assert(melee_dvd_mount(path));
    assert(DVDCheckDisk() && DVDGetCurrentDiskID()->gameVersion == 2);
    assert(DVDConvertPathToEntrynum("/ASSET.DAT") == 1);
    assert(lbFile_8001634C(1) == 59); /* original game function through native SDK */
    DVDFileInfo info;
    assert(!DVDFastOpen(0, &info)); assert(!DVDOpen("missing", &info));
    assert(DVDOpen("asset.dat", &info) && info.length == 59);
    assert(!melee_dvd_unmount());
    info.cb.userData = (void*)0x1234;
    assert(DVDReadPrio(&info, output, 64, 0, 2) == 64);
    assert(!memcmp(output, image + 0xc00, 64));
    assert(info.cb.userData == (void*)0x1234 && DVDGetCommandBlockStatus(&info.cb) == DVD_STATE_END);
    assert(DVDReadPrio(&info, output + 1, 32, 0, 2) == -1);
    assert(DVDReadPrio(&info, output, 64, 32, 2) == -1);
    assert(DVDReadPrio(&info, output, 32, -4, 2) == -1);
    BOOL enabled = OSDisableInterrupts();
    assert(DVDReadAsyncPrio(&info, output, 64, 0, completed, 2));
    assert(DVDGetFileInfoStatus(&info) == DVD_STATE_WAITING);
    assert(!atomic_load(&calls)); assert(!DVDClose(&info));
    assert(!DVDReadAsyncPrio(&info, second, 32, 0, completed, 2));
    assert(!melee_dvd_unmount()); OSRestoreInterrupts(enabled);
    wait_done(); assert(atomic_load(&calls) == 2);
    assert(melee_dvd_mount(path));
    DVDFileInfo low, high;
    assert(DVDOpen("asset.dat", &low) && DVDOpen("asset.dat", &high));
    low.cb.userData = (void*)1; high.cb.userData = (void*)2;
    done = 0; enabled = OSDisableInterrupts();
    assert(DVDReadAsyncPrio(&low, output, 32, 0, priority_completed, 3));
    assert(DVDReadAsyncPrio(&high, second, 32, 0, priority_completed, 0));
    OSRestoreInterrupts(enabled); wait_done(); assert(priority_count == 2);
    assert(melee_dvd_mount(path)); assert(DVDOpen("asset.dat", &info));
    expect_error = 1; done = 0; assert(!ftruncate(fd, 0));
    assert(DVDReadAsyncPrio(&info, output, 64, 0, completed, 2)); wait_done();
    close(fd); unlink(path);
    if (argc == 2) {
        assert(melee_dvd_mount(argv[1]));
        int entry = DVDConvertPathToEntrynum("PlFxNr.dat");
        assert(entry >= 0 && lbFile_8001634C(entry) > 0);
        assert(DVDFastOpen(entry, &info));
        size_t padded = ((size_t)info.length + 31) & ~(size_t)31;
        void* buffer;
        assert(!posix_memalign(&buffer, 32, padded));
        assert(DVDReadPrio(&info, buffer, padded, 0, 2) == (long)padded);
        MeleeArchive archive; uint32_t root;
        assert(melee_archive_open(&archive, buffer, info.length));
        assert(melee_archive_find(&archive, "PlyFox5K_Share_joint", &root));
        free(buffer); assert(DVDClose(&info)); assert(melee_dvd_unmount());
        puts("Original DVD interface loaded Fox's real costume archive from the supplied disc");
    }
    puts("Native DVD reads, deferred callbacks, chaining, I/O errors and original file-size query passed");
}
