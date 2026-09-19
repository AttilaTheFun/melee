#include "melee_card_icons.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char* card_names[]={"MemCardBanner_01","MemCardBanner_02","MemCardBanner_03","MemCardIcon_01","MemCardIconData"};
static const char* snap_names[]={"MemSnapBanner_01","MemSnapIcon_01","MemSnapIconData"};
static void word(unsigned char* p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void check(const char* path,int snapshot)
{
    unsigned count=snapshot?2:4;const char** names=snapshot?snap_names:card_names;
    MeleeCardIcons* (*create)(const MeleeArchive*)=snapshot?melee_card_snapshot_icons_create:melee_card_icons_create;
    size_t n;unsigned char* bytes;
    if(path){FILE* f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long length=ftell(f);assert(length>0);rewind(f);
        n=length;bytes=malloc(n);assert(bytes&&fread(bytes,1,n,f)==n);fclose(f);
    }else{
        unsigned offsets[5];for(unsigned i=0;i<count;i++)offsets[i]=6144*i;
        unsigned root=offsets[count-1]+1536;offsets[count]=root;
        unsigned data_size=root+4*(count+1),relocs=count*4,publics=(count+1)*8;
        n=32+data_size+relocs+publics;for(unsigned i=0;i<=count;i++)n+=strlen(names[i])+1;
        bytes=calloc(1,n);assert(bytes);word(bytes,n);word(bytes+4,data_size);word(bytes+8,count);word(bytes+12,count+1);
        for(unsigned i=0;i<root;i++)bytes[32+i]=(i*17+3)&255;
        unsigned string_offset=0;
        for(unsigned i=0;i<=count;i++){
            if(i<count){word(bytes+32+root+4*i,offsets[i]);word(bytes+32+data_size+4*i,root+4*i);}
            word(bytes+32+data_size+relocs+8*i,offsets[i]);word(bytes+32+data_size+relocs+8*i+4,string_offset);
            strcpy((char*)bytes+32+data_size+relocs+publics+string_offset,names[i]);string_offset+=strlen(names[i])+1;
        }
    }
    MeleeArchive view;assert(melee_archive_open(&view,bytes,n));
    MeleeCardIcons* icons=create(&view);assert(icons);
    unsigned char expected[19968];size_t cursor=0;
    for(unsigned i=0;i<count;i++){
        uint32_t offset;assert(melee_archive_find(&view,names[i],&offset));
        size_t size=melee_card_icons_size(icons,i);assert(size==(i+1<count?6144:1536));
        memcpy(expected+cursor,bytes+32+offset,size);cursor+=size;
    }
    uint32_t root;assert(melee_archive_find(&view,names[count],&root));
    bytes[32+root+3]^=4;assert(!create(&view));bytes[32+root+3]^=4;
    bytes[32+root+4*count+3]=4;assert(!create(&view));
    memset(bytes,0xa5,n);free(bytes);cursor=0;
    for(unsigned i=0;i<count;i++){
        const void* data=melee_card_icons_data(icons,i);size_t size=melee_card_icons_size(icons,i);
        assert((uintptr_t)data>UINT32_MAX&&!memcmp(data,expected+cursor,size));cursor+=size;
    }
    assert(!melee_card_icons_data(icons,count)&&!melee_card_icons_size(icons,count));
    melee_card_icons_free(icons);
}
int main(int argc,char** argv)
{
    if(argc==1){check(NULL,0);check(NULL,1);}
    else if(argc==2)check(argv[1],0);
    else {assert(argc==3&&!strcmp(argv[1],"--snapshot"));check(argv[2],1);}
    puts("Card icons: owned banner/icon bytes, bounded indices and malformed relocation/sentinel rejection passed");
}
