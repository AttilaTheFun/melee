#include "melee_character_wait.h"
#include <melee/ft/ftanim.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftdynamics.h>
#include <sysdolphin/baselib/gobj.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int roll,chosen;
s32 HSD_Randi(s32 max){assert(max==100);return roll;}
bool ftAnim_IsFramesRemaining(Fighter_GObj*g){(void)g;return false;}
void ftData_80085CD8(Fighter*a,Fighter*b,enum_t id){(void)a;(void)b;chosen=id;}
void ftCo_8009E7B4(Fighter*f,u8(*b)[2]){(void)f;(void)b;}
void ftAnim_8006EBA4(Fighter_GObj*g){(void)g;}
void ftAnim_8006EBE8(HSD_GObj*g,float a,float b,float c){(void)g;(void)a;(void)b;(void)c;abort();}
void OSReport(char*s,...){(void)s;}
void HSD_Panic(char*s,u32 n,char*m){(void)s;(void)n;(void)m;abort();}
#undef __assert
void __assert(char*s,u32 n,char*m){(void)s;(void)n;(void)m;abort();}
static u32 ref(const MeleeArchive*a,u32 at){u32 v;MeleeHostBool p;assert(melee_archive_pointer(a,at,&v,&p)&&p);return v;}
int main(int argc,char**argv){assert(argc>1);for(int arg=1;arg<argc;arg++){
 FILE*f=fopen(argv[arg],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);MeleeArchive a;assert(melee_archive_open(&a,b,n));
 const char*name;u32 root;assert(melee_archive_public(&a,0,&name,&root)&&!strncmp(name,"ftData",6));MeleeCharacterWait*o=melee_character_wait_decode(&a,root,400);assert(o);ftData d={0};melee_character_wait_bind(o,&d);
 WaitStruct* tables[]={d.x24,d.x28};int expected[2][100];unsigned count=0;
 for(unsigned k=0;k<2;k++){if(!tables[k])continue;count++;u32 at=ref(&a,root+0x24+k*4);unsigned cursor=0;
  for(unsigned i=0;tables[k][i].u.i.x!=-1;i++){u32 id,w;assert(melee_archive_u32(&a,at+8*i,&id)&&melee_archive_u32(&a,at+8*i+4,&w));assert(id==tables[k][i].u.i.x&&w==tables[k][i].u.i.y);for(unsigned j=0;j<w;j++){assert(cursor<100);expected[k][cursor++]=id;}}
  assert(cursor==100);
 }
 if(count){assert(!melee_character_wait_decode(&a,root,1));u32 at=ref(&a,root+0x24+(tables[0]?0:4));u8 saved[4];memcpy(saved,b+32+at+4,4);memset(b+32+at+4,0,4);assert(!melee_character_wait_decode(&a,root,400));memcpy(b+32+at+4,saved,4);}
 memset(b,0xa5,n);free(b);Fighter fighter={0};HSD_GObj g={0};g.user_data=&fighter;struct Fighter_WaitAnimData anims[400]={{0}};u8 blends[400][2]={{0}};fighter.x24=anims;fighter.x28=blends;
 for(unsigned k=0;k<2;k++)if(tables[k])for(roll=0;roll<100;roll++){fighter.anim_id=2;chosen=-1;ftCo_8008A7A8(&g,tables[k]);assert(chosen==expected[k][roll]&&fighter.anim_id==chosen);}
 melee_character_wait_free(o);printf("%s: %u tables, %u original probability selections and source disposal passed\n",argv[arg],count,count*100);
}}
