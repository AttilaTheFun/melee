#include "melee_character_sounds.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static u32 ref(const MeleeArchive*a,u32 at){u32 v;MeleeHostBool p;assert(melee_archive_pointer(a,at,&v,&p)&&p);return v;}
int main(int argc,char**argv){assert(argc>1);for(int arg=1;arg<argc;arg++){
 FILE*f=fopen(argv[arg],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
 MeleeArchive a;assert(melee_archive_open(&a,b,n));const char*name;u32 root;assert(melee_archive_public(&a,0,&name,&root)&&!strncmp(name,"ftData",6));
 MeleeCharacterSounds*o=melee_character_sounds_decode(&a,root);assert(o);FtSFX*h=melee_character_sounds_header(o);u32 at=ref(&a,root+0x4c);
 unsigned offsets[]={4,8,12,16,20,24,36,40,44,48,52};int values[]={h->x4,h->x8,h->xC,h->x10,h->x14,h->x18,h->x24,h->x28,h->x2C,h->x30,h->x34};
 for(unsigned i=0;i<11;i++){u32 expected;assert(melee_archive_u32(&a,at+offsets[i],&expected));assert((u32)values[i]==expected);}
 unsigned slots[]={0,28,32};FtSFXArr* lists[]={h->smash,h->x1C,h->x20};s32 expected[3][256];unsigned counts[3],total=0;
 for(unsigned i=0;i<3;i++){u32 list=ref(&a,at+slots[i]),data=ref(&a,list+4);assert(melee_archive_u32(&a,list,&counts[i]));assert(lists[i]&&lists[i]->num==counts[i]);total+=counts[i];
  for(unsigned j=0;j<counts[i];j++){u32 id;assert(melee_archive_u32(&a,data+4*j,&id));memcpy(&expected[i][j],&id,4);assert(lists[i]->sfx_ids[j]==expected[i][j]);}
  u8 saved[4];memcpy(saved,b+32+list,4);memset(b+32+list,0,4);assert(!melee_character_sounds_decode(&a,root));memcpy(b+32+list,saved,4);
  MeleeArchive short_a=a;short_a.data_size=data+counts[i]*4-1;assert(!melee_character_sounds_decode(&short_a,root));
 }
 memset(b,0xa5,n);free(b);Fighter fighter={0};fighter.dmg.x190C=(void*)h->x1C;FtSFXArr* damage=fighter.dmg.x190C;assert(damage==lists[1]);
 for(unsigned i=0;i<3;i++)assert(!memcmp(expected[i],lists[i]->sfx_ids,counts[i]*4));melee_character_sounds_free(o);
 printf("%s: 11 sound fields, %u list IDs, native damage pointer and owned lifetime passed\n",argv[arg],total);
}}
