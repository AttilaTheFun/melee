#include "melee_character_parts.h"
#include <stdlib.h>
#include <string.h>
struct MeleeCharacterParts {struct ftData_x8 data;unsigned costumes;};
static int ref(const MeleeArchive*a,u32 slot,u32* at,MeleeHostBool* p,size_t size)
{return melee_archive_pointer(a,slot,at,p)&&(!*p||(*at<=a->data_size&&size<=a->data_size-*at));}
void melee_character_parts_free(MeleeCharacterParts* o)
{
 if(!o)return;
 for(unsigned c=0;c<o->costumes;c++){
  if(o->data.x0.vis_table)for(unsigned t=0;t<4;t++){
   FtPartsVisLookup* groups=o->data.x0.vis_table[c][t];if(!groups)continue;
   for(unsigned g=0;g<o->data.x0.model_num;g++){if(groups[g].x4)for(int i=0;i<groups[g].x0;i++)free(groups[g].x4[i].x4);free(groups[g].x4);}free(groups);
  }
  if(o->data.x8.xC)free(o->data.x8.xC[c]);
 }
 free(o->data.x0.vis_table);free(o->data.x8.xC);free(o);
}
void melee_character_parts_bind(MeleeCharacterParts* o,ftData* d){if(o&&d)d->x8=&o->data;}
static MeleeCharacterParts* decode(const MeleeArchive*a,u32 at,unsigned costumes,unsigned joints,int visibility_only)
{
 if(!a||!costumes||costumes>32||!joints||joints>140)return NULL;
 MeleeCharacterParts* o=calloc(1,sizeof(*o));if(!o)return NULL;o->costumes=costumes;
 u32 vis,tex=0;MeleeHostBool present;
 if(at>a->data_size||(visibility_only?8u:21u)>a->data_size-at||!melee_archive_u32(a,at,&o->data.x0.model_num)||o->data.x0.model_num>11||
    !ref(a,at+4,&vis,&present,costumes*16)||!present)goto fail;
 if(!visibility_only){
  if(!melee_archive_u32(a,at+8,&o->data.x8.x8)||o->data.x8.x8>5||!ref(a,at+12,&tex,&present,costumes*4)||!present)goto fail;
  u8* ids=&o->data.x10;for(unsigned i=0;i<5;i++){ids[i]=a->bytes[32+at+16+i];if(ids[i]>=joints)goto fail;}
 }
 o->data.x0.vis_table=calloc(costumes,sizeof(*o->data.x0.vis_table));o->data.x8.xC=calloc(costumes,sizeof(*o->data.x8.xC));if(!o->data.x0.vis_table||!o->data.x8.xC)goto fail;
 for(unsigned c=0;c<costumes;c++){
  u32 indices=0;present=false;if(!visibility_only&&!ref(a,tex+c*4,&indices,&present,o->data.x8.x8*2))goto fail;
  if(present){o->data.x8.xC[c]=calloc(o->data.x8.x8?o->data.x8.x8:1,sizeof(u16));if(!o->data.x8.xC[c])goto fail;
   for(unsigned i=0;i<o->data.x8.x8;i++)o->data.x8.xC[c][i]=(a->bytes[32+indices+2*i]<<8)|a->bytes[32+indices+2*i+1];}
  for(unsigned t=0;t<4;t++){
   u32 lookup;if(!ref(a,vis+c*16+t*4,&lookup,&present,o->data.x0.model_num*8))goto fail;if(!present)continue;
   FtPartsVisLookup* groups=calloc(o->data.x0.model_num?o->data.x0.model_num:1,sizeof(*groups));if(!groups)goto fail;o->data.x0.vis_table[c][t]=groups;
   for(unsigned g=0;g<o->data.x0.model_num;g++){
    u32 count,choices;if(!melee_archive_u32(a,lookup+g*8,&count)||count>128||!ref(a,lookup+g*8+4,&choices,&present,count*8)||(count&&!present))goto fail;
    groups[g].x0=count;groups[g].x4=calloc(count?count:1,sizeof(TempS));if(!groups[g].x4)goto fail;
    for(unsigned i=0;i<count;i++){
     u32 size,list;if(!melee_archive_u32(a,choices+i*8,&size)||size>124||!ref(a,choices+i*8+4,&list,&present,size)||(size&&!present))goto fail;
     TempS* choice=&groups[g].x4[i];choice->x0=size;choice->x4=malloc(size?size:1);if(!choice->x4)goto fail;
     if(size)memcpy(choice->x4,a->bytes+32+list,size);for(unsigned j=0;j<size;j++)if(choice->x4[j]>=124)goto fail;
    }
   }
  }
 }
 return o;
fail:melee_character_parts_free(o);return NULL;
}

MeleeCharacterParts* melee_character_parts_decode(const MeleeArchive*a,u32 root,unsigned costumes,unsigned joints)
{
 u32 at;MeleeHostBool present;
 if(!a||!ref(a,root+8,&at,&present,21)||!present)return NULL;
 return decode(a,at,costumes,joints,0);
}
MeleeCharacterParts* melee_character_visibility_decode(const MeleeArchive*a,u32 at,unsigned costumes)
{return decode(a,at,costumes,140,1);}
FtPartsDesc* melee_character_visibility_descriptor(MeleeCharacterParts*o)
{return o?&o->data.x0:NULL;}
