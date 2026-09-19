#include "melee_item_common.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(u8* p,u32 v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int main(int argc,char** argv){
    size_t size=413;u8* b=calloc(1,size);assert(b);word(b,size);word(b+4,352+4);word(b+8,1);word(b+12,1);
    word(b+32,4);for(unsigned i=0;i<352;i+=4)word(b+36+i,0x3f800000+i*8192);b[36+0x48]=0xab;
    word(b+388,0);memcpy(b+400,"itPublicData",13);
    /* Header (32), scalar pointer/data (356), relocation (4), public (8), string (13). */
    if(argc==2){free(b);FILE* f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);size=n;b=malloc(size);assert(b&&fread(b,1,size,f)==size);fclose(f);}else assert(argc==1);
    MeleeArchive a;assert(melee_archive_open(&a,b,size));u32 root,data;MeleeHostBool present;
    assert(melee_archive_find(&a,"itPublicData",&root)&&melee_archive_pointer(&a,root,&data,&present)&&present);
    ItemCommonData out;assert(melee_item_common_decode(&a,&out));
    for(unsigned i=0;i<352;i+=4){
        const u8* p=b+32+data+i;
        if(i==0x48||i==0xe4||i==0xec){assert(!memcmp((u8*)&out+i,p,4));continue;}
        u32 bits;memcpy(&bits,(u8*)&out+i,4);assert(bits==(((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3]));
    }
    ItemCommonData saved=out;word(b+32+data+0x15c,0x7fc00000);
    assert(!melee_item_common_decode(&a,&out)&&!memcmp(&out,&saved,sizeof(out)));
    a.data_size=data+351;assert(!melee_item_common_decode(&a,&out)&&!memcmp(&out,&saved,sizeof(out)));
    memset(b,0xa5,size);free(b);assert(!memcmp(&out,&saved,sizeof(out)));
    puts("Item common parameters: scalar bits, raw byte/padding fields, NaN/truncation rejection and output ownership passed");
}
