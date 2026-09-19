#include "melee_character_aux.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static u32 ref(const MeleeArchive*a,u32 at){u32 v;MeleeHostBool p;assert(melee_archive_pointer(a,at,&v,&p)&&p);return v;}
static void word(const MeleeArchive*a,u32 at,void* data){u32 native,expected;assert(melee_archive_u32(a,at,&expected));memcpy(&native,data,4);assert(native==expected);}
int main(int argc,char**argv){assert(argc>1);for(int arg=1;arg<argc;arg++){
 FILE*f=fopen(argv[arg],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);MeleeArchive a;assert(melee_archive_open(&a,b,n));
 const char*name;u32 root;assert(melee_archive_public(&a,0,&name,&root)&&!strncmp(name,"ftData",6));MeleeCharacterAux*o=melee_character_aux_decode(&a,root);assert(o);ftData d={0};melee_character_aux_bind(o,&d);
 u32 at=ref(&a,root+0x44);for(unsigned i=0;i<12;i+=2){u16 value;memcpy(&value,(u8*)d.x44+i,2);assert(value==((u16)b[32+at+i]<<8|b[32+at+i+1]));}
 for(unsigned i=12;i<28;i+=4)word(&a,at+i,(u8*)d.x44+i);word(&a,ref(&a,root+0x50),&d.x50->x);word(&a,ref(&a,root+0x50)+4,&d.x50->y);
 int bones[5];for(unsigned i=0;i<5;i++){word(&a,ref(&a,root+0x54)+4*i,d.x54+i);bones[i]=d.x54[i];}
 at=ref(&a,root+0x58);for(unsigned i=0;i<28;i+=4){if(i==4||i==12||i==24)word(&a,at+i,(u8*)d.x58+i);else assert(!memcmp(b+32+at+i,(u8*)d.x58+i,4));}
 MeleeArchive short_a=a;short_a.data_size=at+27;assert(!melee_character_aux_decode(&short_a,root));u8 saved[4];memcpy(saved,b+32+at+4,4);memcpy(b+32+at+4,"\x7f\xc0\0\0",4);assert(!melee_character_aux_decode(&a,root));memcpy(b+32+at+4,saved,4);
 memset(b,0xa5,n);free(b);for(unsigned i=0;i<5;i++)assert(((int*)d.x54)[i]==bones[i]);melee_character_aux_free(o);printf("%s: ledge, position, five effect bones, limb fields and ownership passed\n",argv[arg]);
}}
