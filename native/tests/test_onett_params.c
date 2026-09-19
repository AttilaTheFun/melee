#include "melee_onett_params.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(u8* p,u32 v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int main(int argc,char** argv){
    size_t size=159;u8* b=calloc(1,size);assert(b);word(b,size);word(b+4,104);word(b+12,1);
    for(unsigned i=0;i<26;i++)word(b+32+4*i,0x3f800000+i*0x10000);
    memcpy(b+144,"yakumono_param",15);
    if(argc==2){free(b);FILE* f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);size=n;b=malloc(size);assert(b&&fread(b,1,size,f)==size);fclose(f);}else assert(argc==1);
    MeleeArchive a;assert(melee_archive_open(&a,b,size));u32 root;assert(melee_archive_find(&a,"yakumono_param",&root));
    struct grOnett_StageParam p;assert(melee_onett_params_decode(&a,&p));
    for(unsigned i=0;i<26;i++){
        const u8* d=b+32+root+4*i;u32 actual;memcpy(&actual,(u8*)&p+4*i,4);
        assert(actual==(((u32)d[0]<<24)|((u32)d[1]<<16)|((u32)d[2]<<8)|d[3]));
    }
    struct grOnett_StageParam saved=p;
    u8 old[4];memcpy(old,b+32+root+100,4);word(b+32+root+100,0x7fc00000);
    assert(!melee_onett_params_decode(&a,&p)&&!memcmp(&p,&saved,sizeof(p)));memcpy(b+32+root+100,old,4);
    a.data_size=root+103;assert(!melee_onett_params_decode(&a,&p)&&!memcmp(&p,&saved,sizeof(p)));
    memset(b,0xa5,size);free(b);assert(!memcmp(&p,&saved,sizeof(p)));
    puts("Onett hazard parameters: all 26 float fields, NaN/truncation rejection and output ownership passed");
}
