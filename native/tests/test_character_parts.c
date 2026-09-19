#include "melee_character_parts.h"
#include <melee/ft/ftparts.h>
#include <sysdolphin/baselib/dobj.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void OSReport(char*s,...){(void)s;}
void HSD_Panic(char*s,u32 n,char*m){(void)s;(void)n;(void)m;abort();}
#undef __assert
void __assert(char*s,u32 n,char*m){(void)s;(void)n;(void)m;abort();}
static u32 ref(const MeleeArchive*a,u32 at){u32 v;MeleeHostBool p;assert(melee_archive_pointer(a,at,&v,&p)&&p);return v;}
int main(int argc,char**argv){assert(argc==3);unsigned costumes=atoi(argv[2]);FILE*f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);MeleeArchive a;assert(melee_archive_open(&a,b,n));const char*name;u32 root;assert(melee_archive_public(&a,0,&name,&root));MeleeCharacterParts*o=melee_character_parts_decode(&a,root,costumes,140);assert(o);ftData d={0};melee_character_parts_bind(o,&d);u32 at=ref(&a,root+8),vis=ref(&a,at+4),tex=ref(&a,at+12);assert(!memcmp(&d.x8->x10,b+32+at+16,5));unsigned total=0;
 for(unsigned c=0;c<costumes;c++){
  if(d.x8->x8.xC[c]){u32 indices=ref(&a,tex+c*4);for(unsigned i=0;i<d.x8->x8.x8;i++)assert(d.x8->x8.xC[c][i]==((b[32+indices+2*i]<<8)|b[32+indices+2*i+1]));}
  for(unsigned t=0;t<4;t++){FtPartsVisLookup*groups=d.x8->x0.vis_table[c][t];u32 lookup;MeleeHostBool p;assert(melee_archive_pointer(&a,vis+c*16+t*4,&lookup,&p)&&p==(groups!=NULL));if(!groups)continue;
   for(unsigned g=0;g<d.x8->x0.model_num;g++){u32 count;assert(melee_archive_u32(&a,lookup+g*8,&count)&&count==groups[g].x0);if(!count)continue;u32 choices=ref(&a,lookup+g*8+4);
    for(unsigned i=0;i<count;i++){u32 size;assert(melee_archive_u32(&a,choices+i*8,&size)&&size==groups[g].x4[i].x0);if(size)assert(!memcmp(groups[g].x4[i].x4,b+32+ref(&a,choices+i*8+4),size));total+=size;}
   }
  }
 }
 assert(!melee_character_parts_decode(&a,root,costumes,1));memset(b,0xa5,n);free(b);
 unsigned owned=0;for(unsigned c=0;c<costumes;c++)for(unsigned t=0;t<4;t++){FtPartsVisLookup*groups=d.x8->x0.vis_table[c][t];if(groups)for(unsigned g=0;g<d.x8->x0.model_num;g++)for(int i=0;i<groups[g].x0;i++){TempS*v=&groups[g].x4[i];for(int j=0;j<v->x0;j++){assert(v->x4[j]<124);owned++;}}}assert(owned==total);

 for(unsigned c=0;c<costumes;c++){
  HSD_DObj objects[2][124]={0};HSD_DObj* pointers[2][124];DObjList lists[2]={{124,pointers[0]},{124,pointers[1]}};u8 expected[2][124]={0};
  for(unsigned l=0;l<2;l++)for(unsigned i=0;i<124;i++){pointers[l][i]=&objects[l][i];objects[l][i].flags=0x80;}
  FtPartsVis state={0};ftParts_8007487C(&d.x8->x0,&state,c,&lists[0],&lists[1]);assert(state.model_num==d.x8->x0.model_num);
  for(unsigned t=0;t<4;t++){FtPartsVisLookup*group=d.x8->x0.vis_table[c][t];if(!group)group=d.x8->x0.vis_table[0][t];assert(state.xC[t]==group);if(group)for(unsigned g=0;g<state.model_num;g++)for(int i=0;i<group[g].x0;i++)for(int j=0;j<group[g].x4[i].x0;j++)expected[t==2][group[g].x4[i].x4[j]]=1;}
  for(unsigned l=0;l<2;l++)for(unsigned i=0;i<124;i++)assert(objects[l][i].flags==(0x80|expected[l][i]));
 }
 printf("%s: %u costumes, %u groups, %u owned drawable indexes texture/bone fields and original visibility initialization passed\n",argv[1],costumes,d.x8->x0.model_num,total);melee_character_parts_free(o);
}
