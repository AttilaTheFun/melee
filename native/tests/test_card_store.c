#include "melee_card_store.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

static MeleeCardStore* concurrent;
static s32 concurrent_slot;
static void* writer(void* unused){
    (void)unused;unsigned char bytes[8192];
    for(int i=0;i<12;i++){memset(bytes,i,sizeof(bytes));assert(!melee_card_store_write(concurrent,concurrent_slot,bytes,sizeof(bytes),0));}
    return NULL;
}
static void* reader(void* unused){
    (void)unused;unsigned char bytes[512];
    for(int i=0;i<80;i++){assert(!melee_card_store_read(concurrent,concurrent_slot,bytes,sizeof(bytes),0));for(size_t j=1;j<sizeof(bytes);j++)assert(bytes[j]==bytes[0]);}
    return NULL;
}
int main(void){
    char directory[]="/tmp/melee-card-XXXXXX";assert(mkdtemp(directory));
    char path[256],lock_path[270],moved[256];
    snprintf(path,sizeof(path),"%s/card.bin",directory);snprintf(lock_path,sizeof(lock_path),"%s.lock",path);
    snprintf(moved,sizeof(moved),"%s-moved",directory);
    s32 result=999;assert(!melee_card_store_open(path,false,&result)&&result==CARD_RESULT_NOCARD);
    MeleeCardStore* s=melee_card_store_open(path,true,&result);assert(s&&!result);
    assert(!melee_card_store_open(path,true,&result)&&result==CARD_RESULT_BUSY);
    s32 bytes,slots,first=-1,second=-1;
    assert(!melee_card_store_free(s,&bytes,&slots)&&bytes==251*8192&&slots==127);
    assert(!melee_card_store_create(s,"../Melee Save",16384,1234,&first)&&first==0);
    assert(melee_card_store_create(s,"../Melee Save",8192,0,&second)==CARD_RESULT_EXIST&&second==-1);
    assert(melee_card_store_create(s,"123456789012345678901234567890123",8192,0,&second)==CARD_RESULT_NAMETOOLONG);
    assert(!melee_card_store_create(s,"12345678901234567890123456789012",8192,42,&second)&&second==1);
    unsigned char input[8192],output[8192];memset(input,0x5a,sizeof(input));
    assert(!melee_card_store_read(s,first,output,512,8192));for(int i=0;i<512;i++)assert(output[i]==0xff);
    assert(!melee_card_store_write(s,first,input,sizeof(input),0));
    assert(!melee_card_store_read(s,first,output,sizeof(output),0)&&!memcmp(input,output,sizeof(input)));
    assert(melee_card_store_read(s,first,output,512,16384)==CARD_RESULT_LIMIT);
    assert(melee_card_store_write(s,first,input,512,0)==CARD_RESULT_FATAL_ERROR);
    CARDStat stat;assert(!melee_card_store_stat(s,first,&stat));
    assert(stat.length==16384&&stat.time==1234&&!memcmp(stat.gameName,"GALE",4)&&!memcmp(stat.company,"01",2));
    assert(stat.offsetData==0&&stat.offsetBanner==UINT32_MAX&&stat.offsetIcon[0]==UINT32_MAX);
    stat.iconAddr=96;stat.commentAddr=32;stat.bannerFormat=2;stat.iconFormat=5;stat.iconSpeed=9;
    stat.length=1;memcpy(stat.fileName,"wrong",6);
    assert(!melee_card_store_set_stat(s,first,&stat,5678));
    assert(!melee_card_store_stat(s,first,&stat)&&stat.length==16384&&stat.time==5678&&!strcmp(stat.fileName,"../Melee Save"));
    assert(!rename(directory,moved));
    memset(input,0xee,sizeof(input));assert(melee_card_store_write(s,first,input,sizeof(input),0)==CARD_RESULT_IOERROR);
    assert(!melee_card_store_read(s,first,output,512,0)&&output[0]==0x5a);
    assert(!rename(moved,directory));
    melee_card_store_close(s);s=melee_card_store_open(path,false,&result);assert(s&&!result);
    assert(melee_card_store_find(s,"../Melee Save")==first);
    assert(!melee_card_store_stat(s,first,&stat)&&stat.iconAddr==96&&stat.commentAddr==32&&stat.bannerFormat==2&&stat.iconFormat==5&&stat.iconSpeed==9);
    assert(stat.offsetBanner==96&&stat.offsetIcon[0]==6240&&stat.offsetIcon[1]==7264&&stat.offsetIconTlut==8288&&stat.offsetData==8800);
    assert(!melee_card_store_read(s,first,output,512,0)&&output[0]==0x5a);
    assert(!melee_card_store_rename(s,first,"Melee Save"));assert(melee_card_store_find(s,"../Melee Save")==CARD_RESULT_NOFILE);
    concurrent=s;concurrent_slot=second;pthread_t threads[3];
    assert(!pthread_create(&threads[0],NULL,writer,NULL));
    assert(!pthread_create(&threads[1],NULL,reader,NULL));assert(!pthread_create(&threads[2],NULL,reader,NULL));
    for(int i=0;i<3;i++)assert(!pthread_join(threads[i],NULL));
    assert(!melee_card_store_free(s,&bytes,&slots));s32 large;
    assert(!melee_card_store_create(s,"full",(u32)bytes,0,&large));
    s32 unchanged=88;assert(melee_card_store_create(s,"overflow",8192,0,&unchanged)==CARD_RESULT_INSSPACE&&unchanged==88);
    assert(!melee_card_store_delete(s,large));assert(!melee_card_store_delete(s,first));
    assert(!melee_card_store_create(s,"reuse",8192,0,&first)&&first==0);
    melee_card_store_close(s);s=melee_card_store_open(path,false,&result);assert(s&&!result);
    assert(melee_card_store_find(s,"reuse")==0&&melee_card_store_find(s,"12345678901234567890123456789012")==1);
    assert(!melee_card_store_format(s));assert(!melee_card_store_free(s,&bytes,&slots)&&bytes==251*8192&&slots==127);
    assert(!melee_card_store_create(s,"corruption check",8192,0,&first));melee_card_store_close(s);
    int fd=open(path,O_RDWR);assert(fd>=0);unsigned char corrupt=0x11;assert(pwrite(fd,&corrupt,1,9000)==1);assert(!close(fd));
    assert(!melee_card_store_open(path,true,&result)&&result==CARD_RESULT_BROKEN);
    fd=open(path,O_RDONLY);assert(fd>=0);unsigned char check=0;assert(pread(fd,&check,1,9000)==1&&check==corrupt);close(fd);
    assert(!unlink(path));assert(!unlink(lock_path));assert(!rmdir(directory));
    puts("Native card storage: persistence, capacity, metadata, rollback, corruption and concurrent access passed");
}
