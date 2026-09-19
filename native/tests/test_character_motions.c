#include "melee_character_motions.h"
#include <melee/ft/ftdata.h>
#include <melee/pl/player.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
Gm_PKind Player_GetPlayerSlotType(s32 slot){(void)slot;return Gm_PKind_Human;}
void OSReport(char* s,...){(void)s;}
void HSD_Panic(char* s,u32 n,char* m){(void)s;(void)n;(void)m;abort();}
#undef __assert
void __assert(char* s,u32 n,char* m){(void)s;(void)n;(void)m;abort();}
static void* readfile(const char* name,size_t* size){FILE*f=fopen(name,"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);void*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);*size=n;return b;}
int main(int argc,char**argv){assert(argc==4);size_t n,an;u8*b=readfile(argv[1],&n),*aj=readfile(argv[2],&an);unsigned count=atoi(argv[3]);MeleeArchive a;assert(melee_archive_open(&a,b,n));const char*name;u32 root,table;MeleeHostBool p;assert(melee_archive_public(&a,0,&name,&root));assert(melee_archive_pointer(&a,root+12,&table,&p)&&p);
 MeleeCharacterMotions*o=melee_character_motions_decode(&a,root,count,aj,an);assert(o);unsigned active=0,scripts=0;
 for(unsigned i=0;i<count;i++){const MeleeCharacterMotion*e=melee_character_motions_entry(o,i);assert(e);u32 v;assert(melee_archive_u32(&a,table+24*i+4,&v)&&v==e->offset);assert(melee_archive_u32(&a,table+24*i+8,&v)&&v==e->size);assert(melee_archive_u32(&a,table+24*i+16,&v)&&v==e->flags);assert(melee_archive_pointer(&a,table+24*i,&v,&p));if(p)assert(!strcmp(e->name,(char*)b+32+v));else assert(!e->name);active+=e->size!=0;scripts+=e->script_offset!=UINT32_MAX;}
 assert(!melee_character_motions_entry(o,count));assert(!melee_character_motions_decode(&a,root,count,aj,1));
 memset(b,0xa5,n);free(b);memset(aj,0xa5,an);free(aj);
 for(unsigned i=0;i<count;i++){union CmdUnion* script=melee_character_motions_script(o,i);assert((script!=NULL)==(melee_character_motions_entry(o,i)->script_offset!=UINT32_MAX));assert(script==melee_character_motions_script(o,i));FigaTree*t=melee_character_motions_tree(o,i);if(melee_character_motions_entry(o,i)->size){assert(t);assert(t==melee_character_motions_tree(o,i));}else assert(!t);}
 struct Fighter_WaitAnimData* records=melee_character_motions_records(o);assert(records);
 Fighter source={0},destination={0};ftData data={0};data.xC=records;gFtDataList[Ft_Kind_Fox]=&data;
 source.kind=Ft_Kind_Fox;source.x24=records;source.x58C=count;ftData_80085A14(Ft_Kind_Fox);
 for(unsigned i=0;i<count;i++){
  const MeleeCharacterMotion* e=melee_character_motions_entry(o,i);struct ftData_80085FD4_ret* alias=ftData_80085FD4(&source,i);
  assert(alias->x8==e->size&&(u32)alias->x10_animCurrFlags==e->flags&&alias->xC==melee_character_motions_script(o,i));
  ftData_80085CD8(&destination,&source,i);assert(destination.x590==melee_character_motions_tree(o,i));assert(destination.x5A4==(void*)records[i].x14);
  assert(ftData_80085E50(&source,i)==destination.x590&&source.x5A8==destination.x5A4);
 }
 assert(!ftData_80085E50(&source,-1)&&!ftData_80085E50(&source,count));
 gFtDataList[Ft_Kind_Fox]=NULL;
 melee_character_motions_free(o);printf("Motion index: %u records, %u lazy native animations, %u script references, source disposal, stable cache and original motion-loader entry points passed\n",count,active,scripts);
}
