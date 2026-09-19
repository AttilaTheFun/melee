/* Owned native save storage. Encode fields explicitly; never persist host
 * pointers/padding. A mutation commits in memory only after atomic replacement. */
#include "melee_card_store.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>

#define CAPACITY (251u*8192u)
#define HEADER 20u
#define DIRECTORY (127u*64u)
typedef struct Entry { CARDStat stat; unsigned char* data; } Entry;
struct MeleeCardStore { pthread_mutex_t lock; int lock_fd; char* path; Entry entries[127]; };
static const unsigned char magic[8]={'M','L','C','A','R','D',0,1};
static u32 be32(const unsigned char* p){return (u32)p[0]<<24|(u32)p[1]<<16|(u32)p[2]<<8|p[3];}
static u16 be16(const unsigned char* p){return (u16)((u16)p[0]<<8|p[1]);}
static void put32(unsigned char* p,u32 v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void put16(unsigned char* p,u16 v){p[0]=v>>8;p[1]=v;}
static u32 crc32(const unsigned char* bytes,size_t length){
    u32 crc=~0u;
    for(size_t i=0;i<length;i++){crc^=bytes[i];for(int b=0;b<8;b++)crc=(crc>>1)^((0u-(crc&1))&0xedb88320u);}
    return ~crc;
}
static s32 name_key(const char* name,char key[32]){
    if(!name)return CARD_RESULT_FATAL_ERROR;
    size_t n=strnlen(name,33);
    if(n>32)return CARD_RESULT_NAMETOOLONG;
    if(!n)return CARD_RESULT_FATAL_ERROR;
    memset(key,0,32);memcpy(key,name,n);return CARD_RESULT_READY;
}
static s32 find(Entry entries[127],const char key[32]){
    for(int i=0;i<127;i++)if(entries[i].data&&!memcmp(entries[i].stat.fileName,key,32))return i;
    return CARD_RESULT_NOFILE;
}
static s32 valid(MeleeCardStore* store,s32 slot){
    if(!store||slot<0||slot>=127)return CARD_RESULT_FATAL_ERROR;
    return store->entries[slot].data?CARD_RESULT_READY:CARD_RESULT_NOFILE;
}
static u32 used(Entry entries[127]){u32 n=0;for(int i=0;i<127;i++)n+=entries[i].stat.length;return n;}
static void clear_entries(Entry entries[127]){for(int i=0;i<127;i++)free(entries[i].data);memset(entries,0,sizeof(Entry)*127);}
static bool transfer(int fd,void* bytes,size_t size,bool writing){
    unsigned char* p=bytes;
    while(size){ssize_t n=writing?write(fd,p,size):read(fd,p,size);if(n<0&&errno==EINTR)continue;if(n<=0)return false;p+=n;size-=(size_t)n;}
    return true;
}
static bool persist(MeleeCardStore* store,Entry entries[127],bool create){
    size_t size=HEADER+DIRECTORY+used(entries);
    unsigned char* blob=calloc(1,size);if(!blob)return false;
    memcpy(blob,magic,8);put32(blob+8,CAPACITY);put32(blob+12,127);
    size_t offset=HEADER+DIRECTORY;
    for(int i=0;i<127;i++)if(entries[i].data){
        CARDStat* s=&entries[i].stat;unsigned char* d=blob+HEADER+i*64;
        memcpy(d,s->gameName,4);memcpy(d+4,s->company,2);d[7]=s->bannerFormat;
        memcpy(d+8,s->fileName,32);put32(d+40,s->time);put32(d+44,s->iconAddr);
        put16(d+48,s->iconFormat);put16(d+50,s->iconSpeed);put16(d+56,s->length/8192);put32(d+60,s->commentAddr);
        memcpy(blob+offset,entries[i].data,s->length);offset+=s->length;
    }
    put32(blob+16,crc32(blob+HEADER,size-HEADER));
    size_t path_size=strlen(store->path)+sizeof(".XXXXXX");char* temporary=malloc(path_size);
    bool ok=temporary!=NULL;int fd=-1;
    if(ok){snprintf(temporary,path_size,"%s.XXXXXX",store->path);fd=mkstemp(temporary);ok=fd>=0;}
    if(ok)ok=transfer(fd,blob,size,true)&&fsync(fd)==0;
    if(fd>=0&&close(fd)!=0)ok=false;
    if(ok)ok=(create?link(temporary,store->path):rename(temporary,store->path))==0;
    if(fd>=0)unlink(temporary);
    free(temporary);free(blob);return ok;
}
static bool parse(MeleeCardStore* store,const unsigned char* blob,size_t size){
    if(size<HEADER+DIRECTORY||memcmp(blob,magic,8)||be32(blob+8)!=CAPACITY||be32(blob+12)!=127||
       be32(blob+16)!=crc32(blob+HEADER,size-HEADER))return false;
    size_t offset=HEADER+DIRECTORY;u32 total=0;
    for(int i=0;i<127;i++){
        const unsigned char* d=blob+HEADER+i*64;
        if(!d[8]){for(int j=0;j<64;j++)if(d[j])return false;continue;}
        char key[32];memcpy(key,d+8,32);
        const char* end=memchr(key,0,32);
        if(end)for(const char* p=end;p<key+32;p++)if(*p)return false;
        if(find(store->entries,key)>=0)return false;
        u32 length=(u32)be16(d+56)*8192u;
        if(!length||length>CAPACITY-total||length>size-offset)return false;
        if(memcmp(d,"GALE01",6)||d[6]||d[52]||d[53]||d[54]||d[55]||d[58]||d[59])return false;
        CARDStat* s=&store->entries[i].stat;memcpy(s->fileName,key,32);
        memcpy(s->gameName,d,4);memcpy(s->company,d+4,2);s->bannerFormat=d[7];s->length=length;
        s->time=be32(d+40);s->iconAddr=be32(d+44);s->iconFormat=be16(d+48);s->iconSpeed=be16(d+50);s->commentAddr=be32(d+60);
        if((s->iconAddr!=UINT32_MAX&&s->iconAddr>=512)||
           (s->commentAddr!=UINT32_MAX&&s->commentAddr%8192>8192-64))return false;
        store->entries[i].data=malloc(length);if(!store->entries[i].data)return false;
        memcpy(store->entries[i].data,blob+offset,length);offset+=length;total+=length;
    }
    return offset==size;
}
MeleeCardStore* melee_card_store_open(const char* path,bool create,s32* error){
    if(error)*error=CARD_RESULT_FATAL_ERROR;
    if(!path||!*path)return NULL;
    MeleeCardStore* store=calloc(1,sizeof(*store));if(!store)return NULL;
    store->lock_fd=-1;
    store->path=strdup(path);
    if(!store->path||pthread_mutex_init(&store->lock,NULL)){free(store->path);free(store);return NULL;}
    size_t lock_size=strlen(path)+sizeof(".lock");char* lock_path=malloc(lock_size);
    if(lock_path){snprintf(lock_path,lock_size,"%s.lock",path);store->lock_fd=open(lock_path,O_RDWR|O_CREAT|O_NOFOLLOW,0600);free(lock_path);}
    if(store->lock_fd<0||flock(store->lock_fd,LOCK_EX|LOCK_NB)!=0){
        if(error)*error=store->lock_fd>=0&&(errno==EWOULDBLOCK||errno==EAGAIN)?CARD_RESULT_BUSY:CARD_RESULT_IOERROR;
        melee_card_store_close(store);return NULL;
    }
    int fd=open(path,O_RDONLY|O_NOFOLLOW);s32 result=CARD_RESULT_IOERROR;
    if(fd<0){
        if(errno==ENOENT)result=create?(persist(store,store->entries,true)?CARD_RESULT_READY:CARD_RESULT_IOERROR):CARD_RESULT_NOCARD;
    }else{
        struct stat info;unsigned char* blob=NULL;
        if(fstat(fd,&info)==0&&S_ISREG(info.st_mode)&&info.st_size>=HEADER+DIRECTORY&&info.st_size<=HEADER+DIRECTORY+CAPACITY){
            size_t size=(size_t)info.st_size;blob=malloc(size);
            if(blob&&transfer(fd,blob,size,false))result=parse(store,blob,size)?CARD_RESULT_READY:CARD_RESULT_BROKEN;
        }else result=CARD_RESULT_BROKEN;
        free(blob);if(close(fd)!=0)result=CARD_RESULT_IOERROR;
    }
    if(error)*error=result;
    if(result!=CARD_RESULT_READY){melee_card_store_close(store);return NULL;}
    return store;
}
void melee_card_store_close(MeleeCardStore* store){if(store){clear_entries(store->entries);pthread_mutex_destroy(&store->lock);if(store->lock_fd>=0)close(store->lock_fd);free(store->path);free(store);}}
s32 melee_card_store_find(MeleeCardStore* store,const char* name){
    char key[32];s32 result=name_key(name,key);if(!store)return CARD_RESULT_FATAL_ERROR;if(result<0)return result;
    pthread_mutex_lock(&store->lock);result=find(store->entries,key);pthread_mutex_unlock(&store->lock);return result;
}
s32 melee_card_store_create(MeleeCardStore* store,const char* name,u32 length,u32 time,s32* slot){
    char key[32];s32 result=name_key(name,key);if(!store||!slot||!length||length%8192)return CARD_RESULT_FATAL_ERROR;if(result<0)return result;
    pthread_mutex_lock(&store->lock);Entry candidate[127];memcpy(candidate,store->entries,sizeof(candidate));
    int index=-1;for(int i=0;i<127;i++)if(!candidate[i].data){index=i;break;}
    if(find(candidate,key)>=0)result=CARD_RESULT_EXIST;
    else if(index<0)result=CARD_RESULT_NOENT;
    else if(length>CAPACITY-used(candidate))result=CARD_RESULT_INSSPACE;
    else{
        Entry* e=&candidate[index];e->data=malloc(length);
        if(!e->data)result=CARD_RESULT_IOERROR;
        else{
            memset(e->data,0xff,length);memcpy(e->stat.fileName,key,32);memcpy(e->stat.gameName,"GALE",4);memcpy(e->stat.company,"01",2);
            e->stat.length=length;e->stat.time=time;e->stat.iconAddr=e->stat.commentAddr=UINT32_MAX;
            if(persist(store,candidate,false)){store->entries[index]=*e;*slot=index;result=CARD_RESULT_READY;}
            else{free(e->data);result=CARD_RESULT_IOERROR;}
        }
    }
    pthread_mutex_unlock(&store->lock);return result;
}
static s32 commit_entry(MeleeCardStore* store,int slot,Entry replacement){
    Entry candidate[127];memcpy(candidate,store->entries,sizeof(candidate));candidate[slot]=replacement;
    if(!persist(store,candidate,false))return CARD_RESULT_IOERROR;
    store->entries[slot]=replacement;return CARD_RESULT_READY;
}
s32 melee_card_store_delete(MeleeCardStore* store,s32 slot){
    if(!store)return CARD_RESULT_FATAL_ERROR;pthread_mutex_lock(&store->lock);s32 result=valid(store,slot);
    if(result==0){unsigned char* old=store->entries[slot].data;result=commit_entry(store,slot,(Entry){0});if(!result)free(old);}
    pthread_mutex_unlock(&store->lock);return result;
}
s32 melee_card_store_rename(MeleeCardStore* store,s32 slot,const char* name){
    char key[32];s32 result=name_key(name,key);if(!store)return CARD_RESULT_FATAL_ERROR;if(result<0)return result;
    pthread_mutex_lock(&store->lock);result=valid(store,slot);
    if(!result){if(find(store->entries,key)>=0)result=CARD_RESULT_EXIST;else{Entry e=store->entries[slot];memcpy(e.stat.fileName,key,32);result=commit_entry(store,slot,e);}}
    pthread_mutex_unlock(&store->lock);return result;
}
/* Same layout calculation as SDK CARDStat.c, including the no-icon sentinel. */
static void icon_offsets(CARDStat* out){
    u8 banner=out->bannerFormat;u16 formats=out->iconFormat;
    u32 offset=out->iconAddr;bool palette=false;
    if(offset==UINT32_MAX){out->bannerFormat=0;out->iconFormat=out->iconSpeed=0;offset=0;}
    out->offsetBanner=out->offsetBannerTlut=UINT32_MAX;
    if((banner&3)==1){out->offsetBanner=offset;offset+=96*32;out->offsetBannerTlut=offset;offset+=512;}
    else if((banner&3)==2){out->offsetBanner=offset;offset+=2*96*32;}
    for(int i=0;i<8;i++){
        unsigned format=(formats>>(i*2))&3;out->offsetIcon[i]=UINT32_MAX;
        if(format==1){out->offsetIcon[i]=offset;offset+=32*32;palette=true;}
        else if(format==2){out->offsetIcon[i]=offset;offset+=2*32*32;}
    }
    out->offsetIconTlut=palette?offset:UINT32_MAX;if(palette)offset+=512;
    out->offsetData=offset;
}
s32 melee_card_store_stat(MeleeCardStore* store,s32 slot,CARDStat* out){
    if(!store||!out)return CARD_RESULT_FATAL_ERROR;pthread_mutex_lock(&store->lock);s32 result=valid(store,slot);
    if(!result){*out=store->entries[slot].stat;icon_offsets(out);}pthread_mutex_unlock(&store->lock);return result;
}
s32 melee_card_store_set_stat(MeleeCardStore* store,s32 slot,const CARDStat* in,u32 time){
    if(!store||!in||(in->iconAddr!=UINT32_MAX&&in->iconAddr>=512)||
       (in->commentAddr!=UINT32_MAX&&in->commentAddr%8192>8128))return CARD_RESULT_FATAL_ERROR;
    pthread_mutex_lock(&store->lock);s32 result=valid(store,slot);
    if(!result){Entry e=store->entries[slot];e.stat.bannerFormat=in->bannerFormat;e.stat.iconAddr=in->iconAddr;
        e.stat.iconFormat=in->iconFormat;e.stat.iconSpeed=in->iconSpeed;e.stat.commentAddr=in->commentAddr;e.stat.time=time;
        if(e.stat.iconAddr==UINT32_MAX)e.stat.iconSpeed=(e.stat.iconSpeed&~3u)|1u;
        result=commit_entry(store,slot,e);}
    pthread_mutex_unlock(&store->lock);return result;
}
static s32 range(MeleeCardStore* store,s32 slot,const void* bytes,s32 length,s32 offset,u32 alignment){
    s32 result=valid(store,slot);if(result<0)return result;
    if(!bytes||length<=0||offset<0||(u32)length%alignment||(u32)offset%alignment)return CARD_RESULT_FATAL_ERROR;
    u32 size=store->entries[slot].stat.length;
    return (u32)offset>=size||(u32)length>size-(u32)offset?CARD_RESULT_LIMIT:CARD_RESULT_READY;
}
s32 melee_card_store_read(MeleeCardStore* store,s32 slot,void* bytes,s32 length,s32 offset){
    if(!store)return CARD_RESULT_FATAL_ERROR;pthread_mutex_lock(&store->lock);s32 result=range(store,slot,bytes,length,offset,512);
    if(!result)memcpy(bytes,store->entries[slot].data+offset,(size_t)length);pthread_mutex_unlock(&store->lock);return result;
}
static s32 write_file(MeleeCardStore* store,s32 slot,const void* bytes,s32 length,s32 offset,bool stamp,u32 time){
    if(!store)return CARD_RESULT_FATAL_ERROR;pthread_mutex_lock(&store->lock);s32 result=range(store,slot,bytes,length,offset,8192);
    if(!result){Entry e=store->entries[slot];unsigned char* old=e.data;e.data=malloc(e.stat.length);
        if(!e.data)result=CARD_RESULT_IOERROR;
        else{memcpy(e.data,old,e.stat.length);memcpy(e.data+offset,bytes,(size_t)length);if(stamp)e.stat.time=time;result=commit_entry(store,slot,e);if(result)free(e.data);else free(old);}}
    pthread_mutex_unlock(&store->lock);return result;
}
s32 melee_card_store_free(MeleeCardStore* store,s32* bytes,s32* slots){
    if(!store||!bytes||!slots)return CARD_RESULT_FATAL_ERROR;pthread_mutex_lock(&store->lock);
    *bytes=(s32)(CAPACITY-used(store->entries));*slots=0;for(int i=0;i<127;i++)*slots+=store->entries[i].data==NULL;
    pthread_mutex_unlock(&store->lock);return CARD_RESULT_READY;
}
s32 melee_card_store_format(MeleeCardStore* store){
    if(!store)return CARD_RESULT_FATAL_ERROR;pthread_mutex_lock(&store->lock);Entry empty[127]={0};
    s32 result=persist(store,empty,false)?CARD_RESULT_READY:CARD_RESULT_IOERROR;
    if(!result)clear_entries(store->entries);pthread_mutex_unlock(&store->lock);return result;
}

s32 melee_card_store_write(MeleeCardStore* store,s32 slot,const void* bytes,s32 length,s32 offset){return write_file(store,slot,bytes,length,offset,false,0);}
s32 melee_card_store_write_timed(MeleeCardStore* store,s32 slot,const void* bytes,s32 length,s32 offset,u32 time){return write_file(store,slot,bytes,length,offset,true,time);}
s32 melee_card_store_check(MeleeCardStore* store){
    if(!store)return CARD_RESULT_FATAL_ERROR;
    pthread_mutex_lock(&store->lock);s32 result=CARD_RESULT_IOERROR;
    int fd=open(store->path,O_RDONLY|O_NOFOLLOW);struct stat info;
    if(fd>=0){
        if(fstat(fd,&info)==0&&S_ISREG(info.st_mode)&&info.st_size>=HEADER+DIRECTORY&&info.st_size<=HEADER+DIRECTORY+CAPACITY){
            unsigned char* blob=malloc((size_t)info.st_size);MeleeCardStore* checked=calloc(1,sizeof(*checked));
            if(blob&&checked&&transfer(fd,blob,(size_t)info.st_size,false))result=parse(checked,blob,(size_t)info.st_size)?CARD_RESULT_READY:CARD_RESULT_BROKEN;
            if(checked)clear_entries(checked->entries);free(checked);free(blob);
        }else result=CARD_RESULT_BROKEN;
        if(close(fd)!=0)result=CARD_RESULT_IOERROR;
    }
    pthread_mutex_unlock(&store->lock);return result;
}
