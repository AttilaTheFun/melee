#include "melee_character_dynamics.h"
#include "melee_item_dynamics.h"
#include <stdlib.h>
#include <string.h>
struct MeleeCharacterDynamics {
    u8 (*pairs[2])[2];
    ItemDynamics* base;
    struct ftDynamics data;
    struct ArticleDynamicBones bones;
    struct ftData_x38 collision[2];
    unsigned slots;
};
void melee_character_dynamics_free(MeleeCharacterDynamics* o)
{
    if(!o)return;
    if(o->data.x10)for(unsigned i=0;i<o->slots;i++)free(o->data.x10[i]);
    free(o->data.x10);free(o->pairs[0]);free(o->pairs[1]);melee_item_dynamics_free(o->base);free(o);
}
void melee_character_dynamics_bind(MeleeCharacterDynamics* o,ftData* d)
{if(o&&d){d->x10=o->pairs[0];d->x18=o->pairs[1];d->x2C=&o->data;}}
static int span(const MeleeArchive* a,u32 slot,size_t bytes,u32* at,MeleeHostBool* present)
{return melee_archive_pointer(a,slot,at,present)&&(!*present||(*at<=a->data_size&&bytes<=a->data_size-*at));}
MeleeCharacterDynamics* melee_character_dynamics_decode_extra(const MeleeArchive* a,u32 root,unsigned joints,unsigned main_count,unsigned demo_count,unsigned extra)
{
    if(!a||!joints||joints>140||!main_count||main_count>1024||demo_count>1024)return NULL;
    MeleeCharacterDynamics* o=calloc(1,sizeof(*o));if(!o)return NULL;
    unsigned counts[2]={main_count,demo_count};
    for(unsigned k=0;k<2;k++){
        u32 at;MeleeHostBool present;
        if(!span(a,root+0x10+k*8,counts[k]*2,&at,&present))goto fail;
        if(!present){if(counts[k])goto fail;continue;}
        o->pairs[k]=malloc(counts[k]*2+1);if(!o->pairs[k])goto fail;
        memcpy(o->pairs[k],a->bytes+32+at,counts[k]*2);
        for(unsigned i=0;i<counts[k];i++)if((unsigned)o->pairs[k][i][1]+1>o->slots)o->slots=o->pairs[k][i][1]+1;
    }
    u32 at;MeleeHostBool present;if(!span(a,root+0x2c,20,&at,&present)||!present)goto fail;
    o->base=melee_item_dynamics_decode_extra(a,at,joints,extra);if(!o->base||o->base->count>Ft_Dynamics_NumMax)goto fail;
    o->data.dynamicsNum=o->base->count-extra;o->data.x4=o->base->collision_count;
    if(o->base->count){o->data.ftDynamicBones=&o->bones;memcpy(o->bones.array,o->base->dyn_descs,o->base->count*sizeof(BoneDynamicsDesc));}
    if(o->base->collision_count){o->data.x8=o->collision;for(int i=0;i<o->base->collision_count;i++){o->collision[i].x0=o->base->collision_descs[i].bone_id;o->collision[i].x4=o->base->collision_descs[i].offset;o->collision[i].x10=o->base->collision_descs[i].size;}}
    u32 table;if(!span(a,at+16,o->slots*4,&table,&present))goto fail;
    if(present){
        o->data.x10=calloc(o->slots,sizeof(*o->data.x10));if(!o->data.x10)goto fail;
        for(unsigned i=0;i<o->slots;i++){
            u32 row;if(!span(a,table+i*4,o->data.dynamicsNum*4,&row,&present))goto fail;if(!present)continue;
            o->data.x10[i]=calloc(o->data.dynamicsNum?o->data.dynamicsNum:1,sizeof(u32));if(!o->data.x10[i])goto fail;
            for(unsigned j=0;j<(unsigned)o->data.dynamicsNum;j++){
                if(!melee_archive_u32(a,row+j*4,&o->data.x10[i][j]))goto fail;
            }
        }
    }
    return o;
fail:melee_character_dynamics_free(o);return NULL;
}

MeleeCharacterDynamics* melee_character_dynamics_decode(const MeleeArchive* a,u32 root,unsigned joints,unsigned main_count,unsigned demo_count)
{return melee_character_dynamics_decode_extra(a,root,joints,main_count,demo_count,0);}
