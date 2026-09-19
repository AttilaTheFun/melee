#include "melee_refraction.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(unsigned char* p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int main(int argc,char** argv){
    unsigned char b[86]={0};word(b,86);word(b+4,32);word(b+8,1);word(b+12,1);
    word(b+48,0x3e4ccccd);word(b+52,0x40a00000);b[56]=3;
    word(b+64,28);word(b+68,24);memcpy(b+76,"lbRefData",10);
    if(argc==2){FILE* f=fopen(argv[1],"rb");assert(f&&fread(b,1,sizeof(b),f)==sizeof(b));fclose(f);}else assert(argc==1);
    MeleeArchive a;assert(melee_archive_open(&a,b,sizeof(b)));float pairs[6]={0};unsigned n=0;
    assert(melee_refraction_decode(&a,pairs,6,&n)&&n==3&&pairs[4]==.2f&&pairs[5]==5);
    float saved[6];memcpy(saved,pairs,sizeof(saved));
    assert(!melee_refraction_decode(&a,pairs,5,&n));
    word(b+48,0x7fc00000);assert(!melee_refraction_decode(&a,pairs,6,&n));
    assert(!memcmp(saved,pairs,sizeof(saved)));
    memset(b,0xa5,sizeof(b));assert(pairs[5]==5);
    puts("Refraction pairs: BE conversion, offset-zero pointer, capacity, NaN and owned storage passed");
}
