#include "melee_player_params.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(unsigned char* p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int main(int argc,char** argv){
    unsigned char b[453]={0};size_t size=sizeof(b);
    word(b,size);word(b+4,392);word(b+8,1);word(b+12,1);
    word(b+32,0x3f800000);word(b+36,0xfffffffe);word(b+32+0x180,0x40000000);
    memcpy(b+32+0xc0,"abcd",4);word(b+424,0x184);word(b+428,0x184);
    memcpy(b+436,"plLoadCommonData",17);
    if(argc==2){FILE* f=fopen(argv[1],"rb");assert(f);size=fread(b,1,sizeof(b),f);assert(fgetc(f)==EOF);fclose(f);}else assert(argc==1);
    MeleeArchive a;assert(melee_archive_open(&a,b,size));pl_804D6470_t out;
    assert(melee_player_params_decode(&a,&out));
    for(unsigned i=0;i<0x184;i+=4){
        if(i==0xc0){assert(!memcmp((unsigned char*)&out+i,b+32+i,4));continue;}
        unsigned expected=((unsigned)b[32+i]<<24)|((unsigned)b[33+i]<<16)|((unsigned)b[34+i]<<8)|b[35+i],actual;
        memcpy(&actual,(unsigned char*)&out+i,4);assert(actual==expected);
    }
    if(argc==1)assert(out.x0==1&&out.x4==-2&&out.x180==2);
    pl_804D6470_t saved=out;
    word(b+32,0x7fc00000);assert(!melee_player_params_decode(&a,&out));assert(!memcmp(&out,&saved,sizeof(out)));
    word(b+32,0);word(b+32+0x184,8);assert(!melee_player_params_decode(&a,&out));
    assert(!memcmp(&out,&saved,sizeof(out)));memset(b,0xa5,sizeof(b));assert(!memcmp(&out,&saved,sizeof(out)));
    puts("Player parameters: all scalar bits, raw gap, offset-zero relocation, bounds, NaN and output ownership passed");
}
