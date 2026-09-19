#include "melee_audio_load_data.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void put(unsigned char* p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static unsigned word(const unsigned char* p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
int main(int argc,char** argv){
    assert(argc==1||argc==2);unsigned char* bytes;size_t size;
    if(argc==2){FILE* f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));size=ftell(f);rewind(f);
        bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
    }else{
        const char name[]="lbAudioLoadData";size=32+1456+124*4+8+sizeof(name);bytes=calloc(1,size);assert(bytes);
        put(bytes,size);put(bytes+4,1456);put(bytes+8,124);put(bytes+12,1);unsigned char* d=bytes+32;
        for(unsigned i=0;i<120;i++){put(d+8*i,i*1000);put(d+8*i+4,0x83d60);put(d+960+4*i,8*i);put(d+1456+4*i,960+4*i);}
        for(unsigned i=0;i<4;i++){put(d+1440+4*i,960+120*i);put(d+1456+480+4*i,1440+4*i);}
        put(d+1456+496,1440);memcpy(d+1456+496+8,name,sizeof(name));
    }
    MeleeArchive a;uint32_t root;assert(melee_archive_open(&a,bytes,size)&&melee_archive_find(&a,"lbAudioLoadData",&root));
    MeleeAudioLoadData* owner=melee_audio_load_data_decode(&a);assert(owner);
    unsigned first=word(bytes+32+root);int* expected=malloc(first);assert(expected);
    for(unsigned i=0;i<first/4;i++)expected[i]=(int32_t)word(bytes+32+4*i);
    unsigned offsets[4][30];
    for(unsigned g=0;g<4;g++)for(unsigned i=0;i<30;i++){
        unsigned table=word(bytes+32+root+4*g);offsets[g][i]=word(bytes+32+table+4*i)/4;
    }
    for(unsigned i=0;i<first/4;i++)if(expected[i]==0x83d60)put(bytes+32+4*i,0);
    assert(!melee_audio_load_data_decode(&a));
    memset(bytes,0xa5,size);free(bytes);
    for(unsigned g=0;g<4;g++)for(unsigned i=0;i<30;i++){
        int* p=melee_audio_load_data_group(owner,g)[i];assert((uintptr_t)p>UINT32_MAX);
        unsigned k=0;do{assert(offsets[g][i]+k<first/4&&p[k]==expected[offsets[g][i]+k]);}while(p[k++]!=0x83d60);
    }
    assert(!melee_audio_load_data_group(owner,4));
    melee_audio_load_data_free(owner);free(expected);
    puts("Audio load data: all 120 locale lists, owned ids, terminators and malformed-list rejection passed");
}
