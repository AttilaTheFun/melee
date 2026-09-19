/* Game-visible CARD API over owned native storage. Disk I/O runs without the
 * interrupt gate; completion and callback chaining use the gate like the SDK. */
#include "melee_card_backend.h"
#include <dolphin/os.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

typedef enum Operation {MOUNT,CHECK,CREATE,DELETE,RENAME,STAT,READ,WRITE,FORMAT} Operation;
typedef struct Sync {pthread_mutex_t lock;pthread_cond_t wake;bool done;s32 result;} Sync;
typedef struct Job {
    Operation op;s32 channel,slot,length,offset;u32 size;
    char name[33],new_name[33];void* bytes;CARDFileInfo* file;
    CARDStat stat;CARDStat* stat_out;CARDCallback callback,detach;Sync* sync;
} Job;
typedef struct Channel {
    MeleeCardStore* store;bool mounted,busy,pending,in_callback;
    s32 result,xferred;CARDCallback detach;Job job;pthread_t worker;
} Channel;
static Channel channels[2]={{.result=CARD_RESULT_NOCARD},{.result=CARD_RESULT_NOCARD}};
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wake=PTHREAD_COND_INITIALIZER;
static bool initialized,stopping;
static _Thread_local bool callback_context;
static bool channel_ok(s32 channel){return channel>=0&&channel<2;}
static u32 card_time(void){time_t now=time(NULL);return now<946684800?0:(u32)(now-946684800);}
static s32 copy_name(char out[33],const char* name){
    if(!name||!*name)return CARD_RESULT_FATAL_ERROR;size_t length=strnlen(name,33);
    if(length>32)return CARD_RESULT_NAMETOOLONG;memcpy(out,name,length);out[length]=0;return 0;
}
static s32 ready(s32 channel,bool mounting){
    if(!channel_ok(channel))return CARD_RESULT_FATAL_ERROR;
    Channel* c=&channels[channel];
    if(stopping||c->busy)return CARD_RESULT_BUSY;
    if(!c->store||(!mounting&&!c->mounted))return CARD_RESULT_NOCARD;
    return CARD_RESULT_READY;
}
static void file_info(CARDFileInfo* out,s32 channel,s32 slot){
    *out=(CARDFileInfo){.chan=channel,.fileNo=slot,.offset=0,.length=0,.iBlock=5};
}
static s32 execute(Job* j,MeleeCardStore* store){
    switch(j->op){
    case MOUNT:case CHECK:return melee_card_store_check(store);
    case CREATE:return melee_card_store_create(store,j->name,j->size,card_time(),&j->slot);
    case DELETE:return melee_card_store_delete(store,j->slot);
    case RENAME:return melee_card_store_rename(store,j->slot,j->new_name);
    case STAT:return melee_card_store_set_stat(store,j->slot,&j->stat,card_time());
    case READ:return melee_card_store_read(store,j->slot,j->bytes,j->length,j->offset);
    case WRITE:return melee_card_store_write_timed(store,j->slot,j->bytes,j->length,j->offset,card_time());
    case FORMAT:return melee_card_store_format(store);
    }
    return CARD_RESULT_FATAL_ERROR;
}
static void* worker(void* argument){
    s32 channel=(s32)(intptr_t)argument;Channel* c=&channels[channel];
    for(;;){
        pthread_mutex_lock(&lock);
        while(!c->pending&&!stopping)pthread_cond_wait(&wake,&lock);
        if(stopping){pthread_mutex_unlock(&lock);return NULL;}
        Job j=c->job;c->pending=false;MeleeCardStore* store=c->store;
        pthread_mutex_unlock(&lock);
        s32 result=execute(&j,store);
        if(getenv("MELEE_CARD_TRACE"))fprintf(stderr,"Native card operation=%d channel=%d slot=%d length=%d offset=%d result=%d\n",j.op,j.channel,j.slot,j.length,j.offset,result);
        BOOL enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);
        if(j.op==MOUNT){c->mounted=result!=CARD_RESULT_IOERROR&&result!=CARD_RESULT_NOCARD;c->detach=c->mounted?j.detach:NULL;}
        if(!result){
            if(j.op==CREATE)file_info(j.file,channel,j.slot);
            if(j.op==STAT)melee_card_store_stat(store,j.slot,j.stat_out);
            if(j.op==READ||j.op==WRITE){
                // Keep the SDK cursor at the start of the final sector chunk.
                s32 last=((j.offset+j.length-1)/8192)*8192;
                j.file->offset=last>j.offset?last:j.offset;
                j.file->length=(j.offset+j.length)-((j.offset+j.length-1)/8192+1)*8192;j.file->iBlock=(u16)(5+j.file->offset/8192);
                c->xferred=j.length;
            }
        }
        c->busy=false;c->result=result;c->in_callback=true;
        pthread_mutex_unlock(&lock);
        callback_context=true;if(j.callback)j.callback(channel,result);callback_context=false;
        pthread_mutex_lock(&lock);c->in_callback=false;pthread_mutex_unlock(&lock);
        if(j.sync){pthread_mutex_lock(&j.sync->lock);j.sync->result=result;j.sync->done=true;pthread_cond_signal(&j.sync->wake);pthread_mutex_unlock(&j.sync->lock);}
        OSRestoreInterrupts(enabled);
    }
}
void CARDInit(void){
    BOOL enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);
    if(!initialized){
        stopping=false;
        for(int i=0;i<2;i++)if(pthread_create(&channels[i].worker,NULL,worker,(void*)(intptr_t)i))abort();
        initialized=true;
    }
    pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);
}
static s32 preflight(Job* j){
    s32 result=ready(j->channel,j->op==MOUNT);if(result<0)return result;
    MeleeCardStore* store=channels[j->channel].store;CARDStat stat;
    if(j->op==CREATE){
        if(!j->file||!j->size||j->size%8192)return CARD_RESULT_FATAL_ERROR;
        if(melee_card_store_find(store,j->name)>=0)return CARD_RESULT_EXIST;
        s32 bytes,slots;melee_card_store_free(store,&bytes,&slots);
        if(!slots)return CARD_RESULT_NOENT;if(j->size>(u32)bytes)return CARD_RESULT_INSSPACE;
    }
    if(j->op==DELETE||j->op==RENAME){
        j->slot=melee_card_store_find(store,j->name);if(j->slot<0)return j->slot;
        if(j->op==RENAME&&melee_card_store_find(store,j->new_name)>=0)return CARD_RESULT_EXIST;
    }
    if(j->op==STAT||j->op==READ||j->op==WRITE){
        result=melee_card_store_stat(store,j->slot,&stat);if(result<0)return result;
    }
    if(j->op==READ||j->op==WRITE){
        s32 alignment=j->op==READ?512:8192;
        if(!j->bytes||j->length<=0||j->offset<0||j->length%alignment||j->offset%alignment)return CARD_RESULT_FATAL_ERROR;
        if((u32)j->offset>=stat.length||(u32)j->length>stat.length-(u32)j->offset)return CARD_RESULT_LIMIT;
    }
    return 0;
}
static s32 enqueue(Job j){
    CARDInit();BOOL enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);
    s32 result=preflight(&j);
    if(!result){Channel* c=&channels[j.channel];c->job=j;c->busy=c->pending=true;c->result=CARD_RESULT_BUSY;c->xferred=0;pthread_cond_broadcast(&wake);}
    pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return result;
}
static s32 synchronous(Job j){
    if(callback_context)return CARD_RESULT_FATAL_ERROR;
    Sync sync={.lock=PTHREAD_MUTEX_INITIALIZER,.wake=PTHREAD_COND_INITIALIZER};j.sync=&sync;
    s32 result=enqueue(j);
    if(!result){
        BOOL enabled=OSDisableInterrupts();OSRestoreInterrupts(true);
        pthread_mutex_lock(&sync.lock);while(!sync.done)pthread_cond_wait(&sync.wake,&sync.lock);result=sync.result;pthread_mutex_unlock(&sync.lock);
        OSDisableInterrupts();OSRestoreInterrupts(enabled);
    }
    pthread_cond_destroy(&sync.wake);pthread_mutex_destroy(&sync.lock);return result;
}
s32 melee_card_insert(s32 channel,const char* path,bool create){
    if(!channel_ok(channel)||callback_context)return CARD_RESULT_FATAL_ERROR;
    CARDInit();BOOL enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);Channel* c=&channels[channel];
    if(stopping||c->store||c->busy||c->in_callback){pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return CARD_RESULT_BUSY;}
    c->busy=true;c->result=CARD_RESULT_BUSY;pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);
    s32 result;MeleeCardStore* store=melee_card_store_open(path,create,&result);
    enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);c->store=store;c->busy=false;c->mounted=false;c->result=CARD_RESULT_NOCARD;c->xferred=0;
    pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return result;
}
s32 melee_card_eject(s32 channel){
    if(!channel_ok(channel)||callback_context)return CARD_RESULT_FATAL_ERROR;
    CARDInit();BOOL enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);Channel* c=&channels[channel];
    if(stopping||c->busy||c->in_callback){pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return CARD_RESULT_BUSY;}
    MeleeCardStore* store=c->store;CARDCallback detach=c->mounted?c->detach:NULL;
    c->store=NULL;c->mounted=false;c->detach=NULL;c->result=CARD_RESULT_NOCARD;
    pthread_mutex_unlock(&lock);melee_card_store_close(store);
    callback_context=true;if(detach)detach(channel,CARD_RESULT_NOCARD);callback_context=false;
    OSRestoreInterrupts(enabled);return 0;
}
s32 melee_card_shutdown(void){
    if(callback_context)return CARD_RESULT_FATAL_ERROR;
    BOOL enabled=OSDisableInterrupts();if(!enabled)return CARD_RESULT_FATAL_ERROR;
    pthread_mutex_lock(&lock);
    for(int i=0;i<2;i++)if(channels[i].busy||channels[i].in_callback){pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return CARD_RESULT_BUSY;}
    bool join=initialized;stopping=true;pthread_cond_broadcast(&wake);pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);
    if(join)for(int i=0;i<2;i++)pthread_join(channels[i].worker,NULL);
    enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);
    for(int i=0;i<2;i++){melee_card_store_close(channels[i].store);channels[i]=(Channel){.result=CARD_RESULT_NOCARD};}
    initialized=false;stopping=false;pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return 0;
}
int CARDProbe(s32 channel){
    if(!channel_ok(channel))return 0;CARDInit();BOOL enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);
    int present=channels[channel].store!=NULL;pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return present;
}
s32 CARDProbeEx(s32 channel,s32* size,s32* sector){
    if(!channel_ok(channel))return CARD_RESULT_FATAL_ERROR;
    if(!CARDProbe(channel))return CARD_RESULT_NOCARD;if(size)*size=16;if(sector)*sector=8192;return 0;
}
s32 CARDGetResultCode(s32 channel){
    if(!channel_ok(channel))return CARD_RESULT_FATAL_ERROR;CARDInit();BOOL enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);
    s32 result=channels[channel].result;pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return result;
}
s32 CARDGetXferredBytes(s32 channel){
    if(!channel_ok(channel))return CARD_RESULT_FATAL_ERROR;CARDInit();BOOL enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);
    s32 result=channels[channel].xferred;pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return result;
}
s32 CARDMountAsync(s32 channel,void* area,CARDCallback detach,CARDCallback callback){
    if(!area||((uintptr_t)area&31))return CARD_RESULT_FATAL_ERROR;
    return enqueue((Job){.op=MOUNT,.channel=channel,.detach=detach,.callback=callback});
}
s32 CARDMount(s32 channel,void* area,CARDCallback detach){
    if(!area||((uintptr_t)area&31))return CARD_RESULT_FATAL_ERROR;
    return synchronous((Job){.op=MOUNT,.channel=channel,.detach=detach});
}
s32 CARDUnmount(s32 channel){
    CARDInit();BOOL enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);s32 result=ready(channel,false);
    if(!result){channels[channel].mounted=false;channels[channel].detach=NULL;channels[channel].result=CARD_RESULT_NOCARD;}
    pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return result;
}
s32 CARDCheckAsync(s32 channel,CARDCallback callback){return enqueue((Job){.op=CHECK,.channel=channel,.callback=callback});}
s32 CARDCheck(s32 channel){return synchronous((Job){.op=CHECK,.channel=channel});}
s32 CARDFormatAsync(s32 channel,CARDCallback callback){return enqueue((Job){.op=FORMAT,.channel=channel,.callback=callback});}
s32 CARDFormat(s32 channel){return synchronous((Job){.op=FORMAT,.channel=channel});}
static s32 create_file(s32 channel,const char* name,u32 size,CARDFileInfo* out,CARDCallback cb,bool sync){
    Job j={.op=CREATE,.channel=channel,.size=size,.file=out,.callback=cb};s32 result=copy_name(j.name,name);if(result<0)return result;
    return sync?synchronous(j):enqueue(j);
}
s32 CARDCreateAsync(s32 c,const char* n,u32 size,CARDFileInfo* f,CARDCallback cb){return create_file(c,n,size,f,cb,false);}
s32 CARDCreate(s32 c,char* n,u32 size,CARDFileInfo* f){return create_file(c,n,size,f,NULL,true);}
static s32 named_job(Operation op,s32 channel,const char* name,const char* new_name,CARDCallback cb,bool sync){
    Job j={.op=op,.channel=channel,.callback=cb};s32 result=copy_name(j.name,name);if(result<0)return result;
    if(op==RENAME){result=copy_name(j.new_name,new_name);if(result<0)return result;}
    return sync?synchronous(j):enqueue(j);
}
s32 CARDDeleteAsync(s32 c,char* n,CARDCallback cb){return named_job(DELETE,c,n,NULL,cb,false);}
s32 CARDDelete(s32 c,char* n){return named_job(DELETE,c,n,NULL,NULL,true);}
s32 CARDRenameAsync(s32 c,const char* a,const char* b,CARDCallback cb){return named_job(RENAME,c,a,b,cb,false);}
s32 CARDRename(s32 c,char* a,char* b){return named_job(RENAME,c,a,b,NULL,true);}
static s32 transfer_file(Operation op,CARDFileInfo* file,void* bytes,s32 length,s32 offset,CARDCallback cb,bool sync){
    if(!file)return CARD_RESULT_FATAL_ERROR;
    Job j={.op=op,.channel=file->chan,.slot=file->fileNo,.file=file,.bytes=bytes,.length=length,.offset=offset,.callback=cb};
    return sync?synchronous(j):enqueue(j);
}
s32 CARDReadAsync(CARDFileInfo* f,void* b,s32 n,s32 o,CARDCallback cb){return transfer_file(READ,f,b,n,o,cb,false);}
s32 CARDRead(CARDFileInfo* f,void* b,s32 n,s32 o){return transfer_file(READ,f,b,n,o,NULL,true);}
s32 CARDWriteAsync(CARDFileInfo* f,void* b,s32 n,s32 o,CARDCallback cb){return transfer_file(WRITE,f,b,n,o,cb,false);}
s32 CARDWrite(CARDFileInfo* f,void* b,s32 n,s32 o){return transfer_file(WRITE,f,b,n,o,NULL,true);}
static s32 set_stat(s32 channel,s32 slot,CARDStat* stat,CARDCallback cb,bool sync){
    if(!stat||(stat->iconAddr!=UINT32_MAX&&stat->iconAddr>=512)||(stat->commentAddr!=UINT32_MAX&&stat->commentAddr%8192>8128))return CARD_RESULT_FATAL_ERROR;
    Job j={.op=STAT,.channel=channel,.slot=slot,.stat=*stat,.stat_out=stat,.callback=cb};return sync?synchronous(j):enqueue(j);
}
s32 CARDSetStatusAsync(s32 c,s32 s,CARDStat* stat,CARDCallback cb){return set_stat(c,s,stat,cb,false);}
s32 CARDSetStatus(s32 c,s32 s,CARDStat* stat){return set_stat(c,s,stat,NULL,true);}
s32 CARDGetStatus(s32 channel,s32 slot,CARDStat* stat){
    if(!stat)return CARD_RESULT_FATAL_ERROR;CARDInit();BOOL enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);s32 result=ready(channel,false);
    if(!result){result=melee_card_store_stat(channels[channel].store,slot,stat);channels[channel].result=result;}
    pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return result;
}
static s32 open_file(s32 channel,s32 slot,char* name,CARDFileInfo* out){
    if(!out)return CARD_RESULT_FATAL_ERROR;out->chan=-1;CARDInit();BOOL enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);s32 result=ready(channel,false);
    if(!result){
        if(name)slot=melee_card_store_find(channels[channel].store,name);
        CARDStat stat;result=slot<0?slot:melee_card_store_stat(channels[channel].store,slot,&stat);
        if(!result)file_info(out,channel,slot);channels[channel].result=result;
    }
    pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return result;
}
s32 CARDOpen(s32 channel,char* name,CARDFileInfo* out){if(!name)return CARD_RESULT_FATAL_ERROR;return open_file(channel,-1,name,out);}
s32 CARDFastOpen(s32 channel,s32 slot,CARDFileInfo* out){if(slot<0||slot>=127)return CARD_RESULT_FATAL_ERROR;return open_file(channel,slot,NULL,out);}
s32 CARDClose(CARDFileInfo* file){
    if(!file||file->fileNo<0||file->fileNo>=127)return CARD_RESULT_FATAL_ERROR;CARDInit();BOOL enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);s32 result=ready(file->chan,false);
    if(!result){channels[file->chan].result=0;file->chan=-1;}
    pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return result;
}
s32 CARDFreeBlocks(s32 channel,s32* bytes,s32* slots){
    if(!bytes||!slots)return CARD_RESULT_FATAL_ERROR;CARDInit();BOOL enabled=OSDisableInterrupts();pthread_mutex_lock(&lock);s32 result=ready(channel,false);
    if(!result){result=melee_card_store_free(channels[channel].store,bytes,slots);channels[channel].result=result;}
    pthread_mutex_unlock(&lock);OSRestoreInterrupts(enabled);return result;
}
