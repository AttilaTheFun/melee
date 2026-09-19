#include "melee_sis_bank.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(unsigned char* p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int main(int argc,char** argv)
{
    assert(argc>=1&&argc<=3);size_t length;unsigned char* bytes;
    const char* symbol=argc==3?argv[2]:"SIS_MessageData";
    if(argc>=2){
        FILE* f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));
        long n=ftell(f);assert(n>32);rewind(f);length=n;bytes=malloc(length);assert(bytes);
        assert(fread(bytes,1,length,f)==length);fclose(f);
    }else{
        length=32+16+12+8+16;bytes=calloc(1,length);assert(bytes);
        word(bytes,length);word(bytes+4,16);word(bytes+8,3);word(bytes+12,1);
        word(bytes+32,16);word(bytes+36,15);word(bytes+40,12);
        bytes[44]=0x16;bytes[45]=0x20;bytes[46]=0x18;
        word(bytes+48,0);word(bytes+52,4);word(bytes+56,8);
        memcpy(bytes+68,"SIS_MessageData",16);
    }
    MeleeArchive a;assert(melee_archive_open(&a,bytes,length));
    MeleeSisBank* bank=melee_sis_bank_decode(&a,symbol);assert(bank);
    size_t count=melee_sis_bank_count(bank);assert(count==a.reloc_count);
    void** table=melee_sis_bank_table(bank);
    unsigned char* expected=malloc(a.data_size);assert(expected);memcpy(expected,bytes+32,a.data_size);
    uint32_t* offsets=malloc(count*sizeof(*offsets));assert(offsets);
    for(size_t i=0;i<count;i++)assert(melee_archive_u32(&a,4*i,&offsets[i]));
    word(bytes+32+8,4);assert(!melee_sis_bank_decode(&a,symbol));
    memset(bytes,0xa5,length);free(bytes);
    for(size_t i=0;i<count;i++){
        assert((uintptr_t)table[i]>UINT32_MAX);
        assert(!memcmp(table[i],expected+offsets[i],a.data_size-offsets[i]));
    }
    assert((unsigned char*)table[0]-(unsigned char*)table[1]==(ptrdiff_t)offsets[0]-(ptrdiff_t)offsets[1]);
    if(argc==1)assert(offsets[0]==a.data_size);
    free(offsets);free(expected);melee_sis_bank_free(bank);
    printf("Owned SIS bank: %zu full-width entries, packed bytes, end pointer and source lifetime passed\n",count);
}
