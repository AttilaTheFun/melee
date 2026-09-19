#include "melee_character_part_anims.h"
#include <sysdolphin/baselib/aobj.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned nodes,tracks,unavailable;
static void walk(HSD_AnimJoint*j){if(!j)return;nodes++;if(j->aobjdesc){assert(j->aobjdesc->end_frame>=0);for(HSD_FObjDesc*f=j->aobjdesc->fobjdesc;f;f=f->next){assert(f->ad);tracks++;}}walk(j->child);walk(j->next);}
int main(int argc,char**argv){assert(argc>=3&&argc<=7);unsigned counts[5];for(int i=2;i<argc;i++)counts[i-2]=atoi(argv[i]);FILE*f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);MeleeArchive a;assert(melee_archive_open(&a,b,n));const char*name;u32 root;assert(melee_archive_public(&a,0,&name,&root));MeleeCharacterPartAnims*o=melee_character_part_anims_decode(&a,root,140,counts,argc-2);assert(o);ftData d={0};melee_character_part_anims_bind(o,&d);assert(!melee_character_part_anims_decode(&a,root,1,counts,argc-2));memset(b,0xa5,n);free(b);
 for(unsigned i=0;i<(unsigned)argc-2;i++){assert(d.x1C[i]&&d.x1C[i]->x0<140);for(unsigned j=0;j<d.x1C[i]->x2;j++)assert(d.x1C[i]->x4[j]<140);for(unsigned j=0;j<counts[i];j++){assert(d.x1C[i]->x8[j]);if(d.x1C[i]->x8[j]==(HSD_AnimJoint*)(intptr_t)-1)unavailable++;else walk(d.x1C[i]->x8[j]);}}
 printf("%s: %d part groups, %u owned animation joints, %u tracks, %u unavailable slots after source disposal passed\n",argv[1],argc-2,nodes,tracks,unavailable);melee_character_part_anims_free(o);
}
