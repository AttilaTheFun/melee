/* Apple hosts have no GameCube HIO/USB development adapter. These entry points
 * implement the SDK's no-device/uninitialized paths, not host filesystem I/O.
 * Error values come from dolphin/mcc/{mcc,fio}.c and hio/hio.c. */
#include <dolphin/types.h>
#include <dolphin/mcc.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>

static atomic_uchar mcc_error;
static atomic_uchar fio_error;
static int not_initialized(void){atomic_store(&mcc_error,1);return 0;}
int MCCEnumDevices(MCC_CBEnumDevices callback){
    atomic_store(&mcc_error,callback?0:13);
    return callback!=NULL; /* Enumeration succeeds, but finds zero devices. */
}
int MCCInit(enum MCC_EXI channel,u8 timeout,MCC_CBSysEvent callback){
    (void)channel;(void)timeout;(void)callback;
    atomic_store(&mcc_error,4);return 0;
}
void MCCExit(void){(void)not_initialized();}
u8 MCCGetLastError(void){return atomic_load(&mcc_error);}
u8 MCCGetFreeBlocks(enum MCC_MODE mode){(void)mode;return (u8)not_initialized();}
int MCCGetConnectionStatus(enum MCC_CHANNEL channel,enum MCC_CONNECT* status){
    (void)channel;(void)status;return not_initialized();
}
int MCCOpen(enum MCC_CHANNEL channel,u8 blocks,MCC_CBEvent callback){
    (void)channel;(void)blocks;(void)callback;return not_initialized();
}
int MCCClose(enum MCC_CHANNEL channel){(void)channel;return not_initialized();}
int MCCNotify(enum MCC_CHANNEL channel,u32 notify){(void)channel;(void)notify;return not_initialized();}
int MCCStreamOpen(enum MCC_CHANNEL channel,u8 blocks){return MCCOpen(channel,blocks,NULL);}
int MCCRead(enum MCC_CHANNEL channel,u32 offset,void* data,long size,enum MCC_SYNC_STATE async){
    (void)channel;(void)offset;(void)data;(void)size;(void)async;return not_initialized();
}
int MCCWrite(enum MCC_CHANNEL channel,u32 offset,void* data,long size,enum MCC_SYNC_STATE async){
    (void)channel;(void)offset;(void)data;(void)size;(void)async;return not_initialized();
}
int FIOInit(enum MCC_EXI channel,enum MCC_CHANNEL id,u8 blocks){
    (void)id;(void)blocks;MCCInit(channel,10,NULL);atomic_store(&fio_error,0x87);return 0;
}
void FIOExit(void){MCCClose(MCC_CHANNEL_SYSTEM);atomic_store(&fio_error,0x87);}
int FIOQuery(void){return 0;} /* No adapter can appear; omit the SDK's 5s spin. */
u8 FIOGetLastError(void){return atomic_load(&fio_error);}
int FIOFopen(const char* name,u32 mode){
    if(!name||(mode&~0xe03u))atomic_store(&fio_error,0xb0);
    else{not_initialized();atomic_store(&fio_error,0x83);}
    return -1;
}
int FIOFclose(int handle){
    if(handle==0||handle==-1)atomic_store(&fio_error,0xb0);
    else{not_initialized();atomic_store(&fio_error,0x83);}
    return 0;
}
u32 FIOFwrite(int handle,void* data,u32 size){
    if(handle==0||handle==-1||!data){atomic_store(&fio_error,0xb0);return UINT32_MAX;}
    if(!size)return 0;
    not_initialized();atomic_store(&fio_error,0x83);return UINT32_MAX;
}
