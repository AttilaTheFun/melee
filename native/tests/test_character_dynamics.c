#include "melee_character_dynamics.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static u32 ref(const MeleeArchive*a,u32 at){u32 v;MeleeHostBool p;assert(melee_archive_pointer(a,at,&v,&p)&&p);return v;}
int main(int argc,char**argv){assert(argc==4);unsigned counts[2]={atoi(argv[2]),atoi(argv[3])};FILE*f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);MeleeArchive a;assert(melee_archive_open(&a,b,n));const char*name;u32 root;assert(melee_archive_public(&a,0,&name,&root));
 MeleeCharacterDynamics*o=melee_character_dynamics_decode(&a,root,140,counts[0],counts[1]);assert(o);ftData d={0};melee_character_dynamics_bind(o,&d);u8(*pairs[2])[2]={d.x10,d.x18};unsigned slots=0;
 for(unsigned k=0;k<2;k++){u32 at=ref(&a,root+16+k*8);assert(!memcmp(pairs[k],b+32+at,counts[k]*2));for(unsigned i=0;i<counts[k];i++)if((unsigned)pairs[k][i][1]+1>slots)slots=pairs[k][i][1]+1;}
 u32 at=ref(&a,root+44),value;assert(melee_archive_u32(&a,at,&value)&&value==d.x2C->dynamicsNum);assert(melee_archive_u32(&a,at+8,&value)&&value==d.x2C->x4);
 unsigned controls=0;if(d.x2C->x10){u32 table=ref(&a,at+16);for(unsigned i=0;i<slots;i++)if(d.x2C->x10[i]){u32 row=ref(&a,table+i*4);for(int j=0;j<d.x2C->dynamicsNum;j++){u32 v;assert(melee_archive_u32(&a,row+j*4,&v)&&v==d.x2C->x10[i][j]);controls++;}}}

 u8 saved[4];memcpy(saved,b+32+at,4);memcpy(b+32+at,"\0\0\0\x0b",4);assert(!melee_character_dynamics_decode(&a,root,140,counts[0],counts[1]));memcpy(b+32+at,saved,4);
 memset(b,0xa5,n);free(b);for(int i=0;i<d.x2C->dynamicsNum;i++){BoneDynamicsDesc*bone=&d.x2C->ftDynamicBones->array[i];assert(bone->bone_id<140&&bone->dyn_desc.count<=320);if(bone->dyn_desc.count)assert(bone->dyn_desc.params);}
 printf("%s: %d dynamic sets, %d collisions, %u selector slots, %u integer chain controls passed\n",argv[1],d.x2C->dynamicsNum,d.x2C->x4,slots,controls);melee_character_dynamics_free(o);
}
