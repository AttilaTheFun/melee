#include "melee_trophy_data.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char* names[]={"tyInitModelTbl","tyInitModelDTbl","tyNoGetUsTbl","tyExpDifferentTbl","tyModelSortTbl","tyDisplayModelTbl","tyDisplayModelUsTbl"};
static const unsigned strides[]={36,36,2,2,12,16,16};
static void word(unsigned char* p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int main(int argc,char** argv){
    assert(argc==1||argc==2);unsigned char* bytes;size_t size;
    if(argc==2){FILE* f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>32);rewind(f);size=n;bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);}
    else{
        size=32+120+56;for(unsigned t=0;t<7;t++)size+=strlen(names[t])+1;
        bytes=calloc(1,size);assert(bytes);word(bytes,size);word(bytes+4,120);word(bytes+12,7);
        unsigned offset=0,name=0;
        for(unsigned t=0;t<7;t++){
            word(bytes+152+8*t,offset);word(bytes+156+8*t,name);
            strcpy((char*)bytes+208+name,names[t]);name+=strlen(names[t])+1;
            if(t>=2&&t<=4){bytes[32+offset]=255;bytes[33+offset]=255;}
            else word(bytes+32+offset,UINT32_MAX);
            offset+=strides[t];
        }
    }
    MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
    MeleeTrophyData* d=melee_trophy_data_decode(&a);assert(d);
    for(unsigned t=0;t<7;t++){
        uint32_t offset;assert(melee_archive_find(&a,names[t],&offset));
        const unsigned char* in=bytes+32+offset;const unsigned char* out=melee_trophy_data_table(d,t);
        size_t n=melee_trophy_data_count(d,t)*strides[t];assert((uintptr_t)out>UINT32_MAX);
        for(size_t j=0;j<n;){
            unsigned col=j%strides[t];
            if(t>=2&&t<=4){uint16_t v;memcpy(&v,out+j,2);assert(v==((uint16_t)in[j]<<8|in[j+1]));j+=2;}
            else if((t<=1&&col==32)||(t>=5&&col==4)){assert(!memcmp(out+j,in+j,4));j+=4;}
            else{uint32_t v,expected;memcpy(&v,out+j,4);assert(melee_archive_u32(&a,offset+j,&expected)&&v==expected);j+=4;}
        }
    }
    unsigned char expected[36];memcpy(expected,melee_trophy_data_table(d,0),36);
    memset(bytes+32,0,36);word(bytes+32+8,0x7fc00000);assert(!melee_trophy_data_decode(&a));
    memset(bytes,0xa5,size);free(bytes);
    assert(!memcmp(expected,melee_trophy_data_table(d,0),36));
    melee_trophy_data_free(d);puts("Seven owned trophy tables: scalar byte order, packed fields, sentinels and source lifetime passed");
}
