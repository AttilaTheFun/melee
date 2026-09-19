#include "melee_dvd.h"
#include "melee_disc.h"
#include <dolphin/dvd.h>
#include <dolphin/os.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct OpenFile {
    DVDFileInfo* info;
    uint32_t entry;
    MeleeHostBool active;
    struct OpenFile* next;
} OpenFile;
typedef struct ReadJob {
    OpenFile* file;
    void* output;
    s32 length, offset, priority;
    DVDCallback callback;
    struct ReadJob* next;
} ReadJob;
static MeleeDisc* disc;
static DVDDiskID disk_id;
static uint32_t current_dir;
static OpenFile* files;
static ReadJob* queue;
static unsigned outstanding;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ready = PTHREAD_COND_INITIALIZER;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static int worker_started;
static OpenFile* find_file(DVDFileInfo* info)
{ for (OpenFile* f = files; f; f = f->next) if (f->info == info) return f; return NULL; }
static void begin(OpenFile* f, void* output, s32 length, s32 offset, s32 state)
{
    f->active = true;
    f->info->cb.command = DVD_COMMAND_READ;
    f->info->cb.state = state;
    f->info->cb.addr = output;
    f->info->cb.offset = f->info->startAddr + (u32)offset;
    f->info->cb.length = length;
    f->info->cb.currTransferSize = 0;
    f->info->cb.transferredSize = 0;
}
static s32 finish(OpenFile* f, MeleeHostBool success, s32 length)
{
    f->active = false;
    f->info->cb.state = success ? DVD_STATE_END : DVD_STATE_FATAL_ERROR;
    f->info->cb.transferredSize = success ? length : 0;
    f->info->cb.currTransferSize = 0;
    return success ? length : DVD_RESULT_FATAL_ERROR;
}
static MeleeHostBool valid_read(OpenFile* f, void* out, long length, long offset, long priority)
{
    return f && !f->active && out && !((uintptr_t)out & 31) && length >= 0 &&
        length <= INT32_MAX && !(length & 31) && offset >= 0 && !(offset & 3) &&
        (uint64_t)offset < f->info->length &&
        (uint64_t)offset + (uint64_t)length < (uint64_t)f->info->length + 32 &&
        priority >= 0 && priority <= 3;
}
static void* worker(void* unused)
{
    (void)unused;
    for (;;) {
        pthread_mutex_lock(&lock);
        while (!queue) pthread_cond_wait(&ready, &lock);
        pthread_mutex_unlock(&lock);
        int interrupts = OSDisableInterrupts();
        pthread_mutex_lock(&lock);
        ReadJob* job = queue; queue = job->next;
        MeleeDisc* source = disc;
        job->file->info->cb.state = DVD_STATE_BUSY;
        job->file->info->cb.currTransferSize = job->length;
        pthread_mutex_unlock(&lock);
        OSRestoreInterrupts(interrupts);
        MeleeHostBool success = melee_disc_read(source, job->file->entry, job->output, job->length, job->offset);
        interrupts = OSDisableInterrupts();
        pthread_mutex_lock(&lock);
        DVDFileInfo* info = job->file->info;
        s32 result = finish(job->file, success, job->length);
        --outstanding;
        pthread_mutex_unlock(&lock);
        /* No file or disc access after callback: it may close/unmount them. */
        if (job->callback) job->callback(result, info);
        free(job);
        OSRestoreInterrupts(interrupts);
    }
    return NULL;
}
static void start_worker(void)
{
    pthread_t thread;
    if (!pthread_create(&thread, NULL, worker, NULL)) {
        pthread_detach(thread); worker_started = 1;
    }
}
void DVDInit(void) { pthread_once(&once, start_worker); }
MeleeHostBool melee_dvd_mount(const char* path)
{
    MeleeDisc* fresh = melee_disc_open(path);
    if (!fresh) return false;
    DVDInit();
    int interrupts = OSDisableInterrupts();
    pthread_mutex_lock(&lock);
    MeleeHostBool ok = worker_started && !files && !outstanding;
    if (ok) {
        melee_disc_close(disc); disc = fresh; current_dir = 0;
        memcpy(&disk_id, melee_disc_id(disc), sizeof(disk_id));
    } else melee_disc_close(fresh);
    pthread_mutex_unlock(&lock); OSRestoreInterrupts(interrupts);
    return ok;
}
MeleeHostBool melee_dvd_unmount(void)
{
    int interrupts = OSDisableInterrupts(); pthread_mutex_lock(&lock);
    MeleeHostBool ok = !files && !outstanding;
    if (ok) { melee_disc_close(disc); disc = NULL; current_dir = 0; memset(&disk_id, 0, sizeof(disk_id)); }
    pthread_mutex_unlock(&lock); OSRestoreInterrupts(interrupts); return ok;
}
s32 DVDConvertPathToEntrynum(const char* path)
{
    pthread_mutex_lock(&lock); s32 result = melee_disc_find(disc, current_dir, path);
    pthread_mutex_unlock(&lock); return result;
}
BOOL DVDChangeDir(char* path)
{
    pthread_mutex_lock(&lock); s32 entry = melee_disc_find(disc, current_dir, path);
    const MeleeDiscEntry* e = entry < 0 ? NULL : melee_disc_entry(disc, entry);
    BOOL result = e && e->directory;
    if (result) current_dir = entry;
    pthread_mutex_unlock(&lock); return result;
}
BOOL DVDFastOpen(s32 entry, DVDFileInfo* info)
{
    if (!info || entry < 0) return FALSE;
    int interrupts = OSDisableInterrupts(); pthread_mutex_lock(&lock);
    const MeleeDiscEntry* e = melee_disc_entry(disc, entry);
    OpenFile* f = find_file(info);
    BOOL result = e && !e->directory && (!f || !f->active);
    if (result && !f) {
        f = calloc(1, sizeof(*f)); result = f != NULL;
        if (f) { f->next = files; files = f; }
    }
    if (result) {
        f->info = info; f->entry = entry;
        memset(info, 0, sizeof(*info)); info->startAddr = e->offset; info->length = e->length;
    }
    pthread_mutex_unlock(&lock); OSRestoreInterrupts(interrupts); return result;
}
BOOL DVDOpen(char* path, DVDFileInfo* info)
{
    int interrupts = OSDisableInterrupts();
    BOOL result = DVDFastOpen(DVDConvertPathToEntrynum(path), info);
    OSRestoreInterrupts(interrupts); return result;
}
BOOL DVDClose(DVDFileInfo* info)
{
    int interrupts = OSDisableInterrupts(); pthread_mutex_lock(&lock);
    OpenFile** p = &files;
    while (*p && (*p)->info != info) p = &(*p)->next;
    BOOL result = *p && !(*p)->active;
    if (result) { OpenFile* f = *p; *p = f->next; free(f); }
    pthread_mutex_unlock(&lock); OSRestoreInterrupts(interrupts); return result;
}
long DVDReadPrio(DVDFileInfo* info, void* output, long length, long offset, long priority)
{
    int interrupts = OSDisableInterrupts(); pthread_mutex_lock(&lock);
    OpenFile* f = find_file(info);
    if (!valid_read(f, output, length, offset, priority)) {
        pthread_mutex_unlock(&lock); OSRestoreInterrupts(interrupts); return DVD_RESULT_FATAL_ERROR;
    }
    begin(f, output, length, offset, DVD_STATE_BUSY); ++outstanding;
    MeleeDisc* source = disc; pthread_mutex_unlock(&lock); OSRestoreInterrupts(interrupts);
    MeleeHostBool success = melee_disc_read(source, f->entry, output, length, offset);
    interrupts = OSDisableInterrupts(); pthread_mutex_lock(&lock);
    s32 result = finish(f, success, length); --outstanding;
    pthread_mutex_unlock(&lock); OSRestoreInterrupts(interrupts); return result;
}
BOOL DVDReadAsyncPrio(DVDFileInfo* info, void* output, s32 length, s32 offset, DVDCallback callback, s32 priority)
{
    DVDInit();
    int interrupts = OSDisableInterrupts(); pthread_mutex_lock(&lock);
    OpenFile* f = find_file(info);
    ReadJob* job = NULL;
    if (worker_started && valid_read(f, output, length, offset, priority)) job = calloc(1, sizeof(*job));
    if (job) {
        *job = (ReadJob){.file = f, .output = output, .length = length, .offset = offset,
                        .priority = priority, .callback = callback};
        begin(f, output, length, offset, DVD_STATE_WAITING); info->callback = callback; ++outstanding;
        ReadJob** position = &queue;
        while (*position && (*position)->priority <= priority) position = &(*position)->next;
        job->next = *position; *position = job;
        pthread_cond_signal(&ready);
    }
    BOOL accepted = job != NULL;
    pthread_mutex_unlock(&lock); OSRestoreInterrupts(interrupts); return accepted;
}
long DVDGetFileInfoStatus(DVDFileInfo* info)
{
    pthread_mutex_lock(&lock); OpenFile* f = find_file(info);
    long result = f ? info->cb.state : DVD_STATE_FATAL_ERROR;
    pthread_mutex_unlock(&lock); return result;
}
s32 DVDGetTransferredSize(DVDFileInfo* info)
{
    pthread_mutex_lock(&lock); OpenFile* f = find_file(info);
    s32 result = f ? (s32)info->cb.transferredSize : DVD_RESULT_FATAL_ERROR;
    pthread_mutex_unlock(&lock); return result;
}
long DVDGetDriveStatus(void)
{
    pthread_mutex_lock(&lock); long result = !disc ? DVD_STATE_NO_DISK : outstanding ? DVD_STATE_BUSY : DVD_STATE_END;
    pthread_mutex_unlock(&lock); return result;
}
BOOL DVDCheckDisk(void) { pthread_mutex_lock(&lock); BOOL result = disc != NULL; pthread_mutex_unlock(&lock); return result; }
DVDDiskID* DVDGetCurrentDiskID(void) { return &disk_id; }

long DVDGetCommandBlockStatus(DVDCommandBlock* block)
{
    pthread_mutex_lock(&lock);
    long result = DVD_STATE_FATAL_ERROR;
    for (OpenFile* f = files; f; f = f->next)
        if (&f->info->cb == block) { result = block->state; break; }
    pthread_mutex_unlock(&lock); return result;
}
