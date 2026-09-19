#include "melee_card_backend.h"
#include <dolphin/os.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

static pthread_mutex_t events_lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t events_wake=PTHREAD_COND_INITIALIZER;
static unsigned events;static s32 last_result,last_channel;
static CARDFileInfo chained_file;
static unsigned char written[16384] __attribute__((aligned(32)));
static unsigned char read_back[16384] __attribute__((aligned(32)));
static int phase;
static void complete(s32 channel,s32 result){
    assert(OSDisableInterrupts()==false);OSRestoreInterrupts(false);
    pthread_mutex_lock(&events_lock);++events;last_result=result;last_channel=channel;pthread_cond_signal(&events_wake);pthread_mutex_unlock(&events_lock);
}
static void wait_event(unsigned expected,s32 result){
    struct timespec deadline;clock_gettime(CLOCK_REALTIME,&deadline);deadline.tv_sec+=10;
    pthread_mutex_lock(&events_lock);
    while(events<expected)assert(!pthread_cond_timedwait(&events_wake,&events_lock,&deadline));
    assert(events==expected&&last_result==result);pthread_mutex_unlock(&events_lock);
    // Synchronize with callback return as a GameCube caller would.
    BOOL enabled=OSDisableInterrupts();OSRestoreInterrupts(enabled);
}
static void chain(s32 channel,s32 result){
    assert(channel==0&&!result&&CARDGetResultCode(channel)==0);
    assert(OSDisableInterrupts()==false);OSRestoreInterrupts(false);
    switch(phase++){
    case 0:assert(!CARDCreateAsync(0,"Melee Save",sizeof(written),&chained_file,chain));break;
    case 1:assert(chained_file.chan==0&&chained_file.fileNo==0);assert(!CARDWriteAsync(&chained_file,written,sizeof(written),0,chain));break;
    case 2:assert(CARDGetXferredBytes(0)==sizeof(written));assert(chained_file.offset==8192&&chained_file.length==0);
        assert(CARDRead(&chained_file,read_back,512,0)==CARD_RESULT_FATAL_ERROR);
        assert(!CARDReadAsync(&chained_file,read_back,sizeof(read_back),0,chain));break;
    case 3:assert(!memcmp(written,read_back,sizeof(written)));complete(channel,result);break;
    default:abort();
    }
}
static void detached(s32 channel,s32 result){assert(result==CARD_RESULT_NOCARD&&!CARDProbe(channel));complete(channel,result);}
int main(void){
    char directory[]="/tmp/melee-card-api-XXXXXX";assert(mkdtemp(directory));
    char path[256],path_b[256],moved[256],lock_path[270];
    snprintf(path,sizeof(path),"%s/a.card",directory);snprintf(path_b,sizeof(path_b),"%s/b.card",directory);snprintf(moved,sizeof(moved),"%s-moved",directory);
    void* work;assert(!posix_memalign(&work,32,CARD_WORKAREA_SIZE));
    memset(written,0x69,sizeof(written));CARDInit();CARDInit();
    assert(!CARDProbe(0)&&!CARDProbe(1)&&CARDGetResultCode(0)==CARD_RESULT_NOCARD);
    assert(CARDGetResultCode(-1)==CARD_RESULT_FATAL_ERROR);
    s32 size=7,sector=8;
    assert(CARDProbeEx(0,&size,&sector)==CARD_RESULT_NOCARD&&size==7&&sector==8);
    assert(CARDMountAsync(0,work,detached,complete)==CARD_RESULT_NOCARD&&events==0);
    assert(!melee_card_insert(0,path,true));assert(CARDProbe(0));
    assert(!CARDProbeEx(0,&size,&sector)&&size==16&&sector==8192);
    CARDFileInfo file;assert(CARDOpen(0,"Melee Save",&file)==CARD_RESULT_NOCARD);
    BOOL enabled=OSDisableInterrupts();
    assert(!CARDMountAsync(0,work,detached,chain));
    assert(CARDGetResultCode(0)==CARD_RESULT_BUSY);
    assert(CARDMountAsync(0,work,NULL,complete)==CARD_RESULT_BUSY);
    assert(melee_card_eject(0)==CARD_RESULT_BUSY);
    OSRestoreInterrupts(enabled);wait_event(1,0);assert(phase==4);
    CARDStat stat;assert(!CARDGetStatus(0,chained_file.fileNo,&stat));assert(stat.length==sizeof(written)&&stat.time>0);
    stat.iconAddr=96;stat.commentAddr=32;stat.bannerFormat=2;stat.iconFormat=5;stat.iconSpeed=9;
    assert(!CARDSetStatusAsync(0,chained_file.fileNo,&stat,complete));wait_event(2,0);assert(stat.offsetData==8800);
    s32 bytes,slots;assert(!CARDFreeBlocks(0,&bytes,&slots)&&bytes==249*8192&&slots==126);
    assert(!CARDOpen(0,"Melee Save",&file));enabled=OSDisableInterrupts();
    assert(!CARDRead(&file,read_back,512,0));assert(file.offset==0&&file.length==-7680);assert(OSDisableInterrupts()==false);OSRestoreInterrupts(enabled);
    assert(!memcmp(written,read_back,512));
    assert(CARDReadAsync(&file,read_back,1,0,complete)==CARD_RESULT_FATAL_ERROR);
    assert(CARDReadAsync(&file,read_back,512,sizeof(written),complete)==CARD_RESULT_LIMIT);
    assert(CARDDeleteAsync(0,"missing",complete)==CARD_RESULT_NOFILE&&events==2);
    assert(!CARDRenameAsync(0,"Melee Save","Renamed",complete));wait_event(3,0);
    assert(!CARDClose(&file)&&file.chan==-1);assert(CARDRead(&file,read_back,512,0)==CARD_RESULT_FATAL_ERROR);
    assert(!CARDUnmount(0)&&CARDProbe(0));assert(CARDGetResultCode(0)==CARD_RESULT_NOCARD);
    assert(!CARDMount(0,work,detached));assert(!melee_card_eject(0));wait_event(4,CARD_RESULT_NOCARD);
    assert(!melee_card_insert(0,path,false));assert(!CARDMount(0,work,NULL));assert(!CARDOpen(0,"Renamed",&file));
    assert(!CARDRead(&file,read_back,sizeof(read_back),0)&&!memcmp(written,read_back,sizeof(written)));
    assert(!melee_card_insert(1,path_b,true));assert(!CARDMount(1,work,NULL));
    CARDFileInfo other;assert(!CARDCreate(1,"Other",8192,&other));
    enabled=OSDisableInterrupts();
    assert(!CARDReadAsync(&file,read_back,512,0,complete));
    assert(!CARDWriteAsync(&other,written,8192,0,complete));
    OSRestoreInterrupts(enabled);
    // Both channels complete independently; their order is intentionally free.
    struct timespec deadline;clock_gettime(CLOCK_REALTIME,&deadline);deadline.tv_sec+=10;
    pthread_mutex_lock(&events_lock);while(events<6)assert(!pthread_cond_timedwait(&events_wake,&events_lock,&deadline));assert(events==6&&last_result==0);pthread_mutex_unlock(&events_lock);
    enabled=OSDisableInterrupts();OSRestoreInterrupts(enabled);
    assert(!rename(directory,moved));memset(written,0xaa,sizeof(written));
    assert(!CARDWriteAsync(&file,written,8192,0,complete));wait_event(7,CARD_RESULT_IOERROR);
    assert(!CARDUnmount(0));assert(!CARDMountAsync(0,work,NULL,complete));wait_event(8,CARD_RESULT_IOERROR);
    assert(CARDOpen(0,"Renamed",&file)==CARD_RESULT_NOCARD);
    assert(!rename(moved,directory));assert(!CARDMount(0,work,NULL));assert(!CARDOpen(0,"Renamed",&file));
    assert(!CARDRead(&file,read_back,512,0)&&read_back[0]==0x69);
    int fd=open(path,O_RDWR);assert(fd>=0);unsigned char bad=0x27;assert(pwrite(fd,&bad,1,9000)==1);close(fd);
    assert(!CARDCheckAsync(0,complete));wait_event(9,CARD_RESULT_BROKEN);
    assert(!CARDFormat(0));assert(!CARDCheck(0));assert(CARDGetStatus(0,0,&stat)==CARD_RESULT_NOFILE);
    assert(!CARDDelete(1,"Other"));assert(!melee_card_shutdown());
    CARDInit();assert(!CARDProbe(0)&&!CARDProbe(1));assert(!melee_card_shutdown());
    free(work);unlink(path);unlink(path_b);
    snprintf(lock_path,sizeof(lock_path),"%s.lock",path);unlink(lock_path);
    snprintf(lock_path,sizeof(lock_path),"%s.lock",path_b);unlink(lock_path);assert(!rmdir(directory));
    puts("Native CARD SDK: absent slots, async chaining, sync waits, two slots, persistence, failure and format recovery passed");
}
