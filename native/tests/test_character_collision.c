#include "melee_character_collision.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static u32 ref(const MeleeArchive*a,u32 at){u32 v;MeleeHostBool p;assert(melee_archive_pointer(a,at,&v,&p)&&p);return v;}
static void compare(const MeleeArchive*a,u32 at,const void* value,unsigned n){for(unsigned i=0;i<n;i++){u32 disk,native;assert(melee_archive_u32(a,at+4*i,&disk));memcpy(&native,(u8*)value+4*i,4);assert(disk==native);}}
int main(int argc,char**argv){assert(argc>1);for(int arg=1;arg<argc;arg++){
 FILE*f=fopen(argv[arg],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
 MeleeArchive a;assert(melee_archive_open(&a,b,n));const char*name;u32 root;assert(melee_archive_public(&a,0,&name,&root)&&!strncmp(name,"ftData",6));
 MeleeCharacterCollision*o=melee_character_collision_decode(&a,root,140);assert(o);ftData data={0};melee_character_collision_bind(o,&data);
 u32 h=ref(&a,root+0x30),count;assert(melee_archive_u32(&a,h,&count)&&count==data.x30->count);compare(&a,ref(&a,h+4),data.x30->inits,count*10);
 compare(&a,ref(&a,root+0x34),data.x34,2);compare(&a,ref(&a,root+0x38),data.x38,10);compare(&a,ref(&a,root+0x3c),data.x3C,6);compare(&a,ref(&a,root+0x40),data.x40,12);
 assert(!melee_character_collision_decode(&a,root,1));u8 saved[4];memcpy(saved,b+32+h,4);memcpy(b+32+h,"\0\0\0\x10",4);assert(!melee_character_collision_decode(&a,root,140));memcpy(b+32+h,saved,4);
 u32 radius=ref(&a,h+4)+36;memcpy(saved,b+32+radius,4);memcpy(b+32+radius,"\x7f\xc0\0\0",4);assert(!melee_character_collision_decode(&a,root,140));memcpy(b+32+radius,saved,4);
 ftHurtboxInit expected[15];memcpy(expected,data.x30->inits,count*sizeof(*expected));memset(b,0xa5,n);free(b);assert(!memcmp(expected,data.x30->inits,count*sizeof(*expected)));assert(data.x34->scale>=0);
 melee_character_collision_free(o);printf("%s: %u hurtboxes, shield/bounds/camera/pickup data and owned lifetime passed\n",argv[arg],count);
}}
