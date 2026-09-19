#include "melee_visibility.h"
#include <melee/ft/types.h>
#include <melee/ft/ftparts.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
extern bool ftAction_ModelCommand(Fighter_GObj*,u32);
extern void ftMr_Init_OnDeath(HSD_GObj*);
extern void ftFx_Init_OnDeath(HSD_GObj*);
extern void ftPe_Init_OnDeath(HSD_GObj*);
extern void ftGw_Init_OnDeath(HSD_GObj*);
extern void ftKb_Init_ResetModelParts(HSD_GObj*);
struct MeleeVisibility {
    Fighter* fighter;
    FtPartsVis vis;
    DObjList list;
    HSD_DObj* objects;
    MeleeHostBool controlled[124];
};
static MeleeHostBool range(const MeleeArchive* a,uint32_t at,size_t n)
{return at<=a->data_size && n<=a->data_size-at;}
static MeleeHostBool ref(const MeleeArchive* a,uint32_t at,uint32_t* target)
{MeleeHostBool present;if(!melee_archive_pointer(a,at,target,&present))return false;if(!present)*target=UINT32_MAX;return true;}
void melee_visibility_free(MeleeVisibility* v)
{
    if(!v)return;
    FtPartsVisLookup* lookup=v->vis.xC[0];
    if(lookup)for(size_t i=0;i<v->vis.model_num;i++){
        if(lookup[i].x4)for(int j=0;j<lookup[i].x0;j++)free(lookup[i].x4[j].x4);
        free(lookup[i].x4);
    }
    free(lookup);free(v->list.data);free(v->objects);free(v->fighter);free(v);
}
size_t melee_visibility_group_count(const MeleeVisibility* v){return v?v->vis.model_num:0;}
size_t melee_visibility_choice_count(const MeleeVisibility* v,size_t group)
{return v && group<v->vis.model_num && v->vis.xC[0]?(size_t)v->vis.xC[0][group].x0:0;}
MeleeHostBool melee_visibility_hidden(const MeleeVisibility* v,size_t drawable)
{return v && drawable<v->list.count && (v->list.data[drawable]->flags&1);}
MeleeHostBool melee_visibility_controls(const MeleeVisibility* v,size_t drawable)
{return v && drawable<v->list.count && v->controlled[drawable];}
MeleeHostBool melee_visibility_bind(MeleeVisibility* v,HSD_DObj* const* objects,size_t count)
{
    if(!v || count!=v->list.count || (count&&!objects))return false;
    for(size_t i=0;i<count;i++){
        if(!objects[i])return false;
        for(size_t j=0;j<i;j++)if(objects[j]==objects[i])return false;
    }
    u32 hidden[124];
    for(size_t i=0;i<count;i++)hidden[i]=v->list.data[i]->flags&DOBJ_HIDDEN;
    for(size_t i=0;i<count;i++){
        if(v->controlled[i])objects[i]->flags=(objects[i]->flags&~DOBJ_HIDDEN)|hidden[i];
        v->list.data[i]=objects[i];
    }
    return true;
}
MeleeHostBool melee_visibility_defaults(MeleeVisibility* v,const int* choices,size_t count)
{
    if(!v || count!=v->vis.model_num || (count && !choices))return false;
    for(size_t i=0;i<count;i++)if(choices[i]<INT8_MIN || choices[i]>INT8_MAX)return false;
    HSD_GObj gobj={0};gobj.user_data=v->fighter;
    for(size_t i=0;i<count;i++)ftParts_80074A4C(&gobj,(int)i,choices[i]);
    return true;
}
MeleeHostBool melee_visibility_command(MeleeVisibility* v,uint32_t word)
{
    if(!v)return false;
    HSD_GObj gobj={0};gobj.user_data=v->fighter;
    v->fighter->x5AC=v->vis;v->fighter->dobj_list=v->list;
    if(!ftAction_ModelCommand(&gobj,word))return false;
    v->vis=v->fighter->x5AC;
    ftParts_80074D7C(&v->vis,0,&v->list);
    ftParts_80074B6C(v->fighter,&v->vis,0,&v->list);
    return true;
}
MeleeHostBool melee_visibility_init_fighter(MeleeVisibility* v,uint32_t kind)
{
    if(!v)return false;
    void (*initialize)(HSD_GObj*)=NULL;size_t groups;
    switch(kind){
    case Ft_Kind_Mario:initialize=ftMr_Init_OnDeath;groups=1;break;
    case Ft_Kind_Fox:initialize=ftFx_Init_OnDeath;groups=1;break;
    case Ft_Kind_Kirby:initialize=ftKb_Init_ResetModelParts;groups=2;break;
    case Ft_Kind_Peach:initialize=ftPe_Init_OnDeath;groups=7;break;
    case Ft_Kind_GameWatch:initialize=ftGw_Init_OnDeath;groups=11;break;
    default:return false;
    }
    if(v->vis.model_num!=groups)return false;
    HSD_GObj gobj={0};gobj.user_data=v->fighter;v->fighter->kind=(FighterKind)kind;
    initialize(&gobj);
    return melee_visibility_command(v,32u<<26);
}
MeleeHostBool melee_visibility_select(MeleeVisibility* v,const int* choices,size_t count)
{
    if(!v || count!=v->vis.model_num || (count && !choices))return false;
    for(size_t i=0;i<count;i++)if(choices[i]<INT8_MIN || choices[i]>INT8_MAX)return false;
    ftParts_80074D7C(&v->vis,0,&v->list);
    for(size_t i=0;i<count;i++)v->fighter->x5F4_arr[i].idx=choices[i];
    ftParts_80074B6C(v->fighter,&v->vis,0,&v->list);
    return true;
}
static MeleeVisibility* decode_lookup(const MeleeArchive*,uint32_t,uint32_t,size_t,size_t);
MeleeVisibility* melee_visibility_decode(const MeleeArchive* a,uint32_t desc,size_t costume,unsigned table,size_t drawables)
{
    if(!a || !a->bytes || table>3 || costume>UINT8_MAX || drawables>124 || !range(a,desc,8))return NULL;
    uint32_t groups,vis,lookup;
    if(!melee_archive_u32(a,desc,&groups) || groups>11 || !ref(a,desc+4,&vis) || !range(a,vis,16) || costume>(a->data_size-vis-16)/16 || !ref(a,vis+(uint32_t)costume*16+table*4,&lookup))return NULL;
    if(lookup==UINT32_MAX && !ref(a,vis+table*4,&lookup))return NULL;
    return decode_lookup(a,groups,lookup,costume,drawables);
}
MeleeVisibility* melee_visibility_decode_lookup(const MeleeArchive* a,uint32_t groups,uint32_t lookup,size_t drawables)
{
    return decode_lookup(a,groups,lookup,0,drawables);
}
static MeleeVisibility* decode_lookup(const MeleeArchive* a,uint32_t groups,uint32_t lookup,size_t costume,size_t drawables)
{
    if(!a||!a->bytes||groups>11||drawables>124||costume>UINT8_MAX)return NULL;
    MeleeVisibility* v=calloc(1,sizeof(*v));if(!v)return NULL;
    v->fighter=calloc(1,sizeof(*v->fighter));v->objects=calloc(drawables?drawables:1,sizeof(*v->objects));v->list.data=calloc(drawables?drawables:1,sizeof(*v->list.data));
    if(!v->fighter || !v->objects || !v->list.data)goto fail;
    v->list.count=(u32)drawables;v->vis.model_num=groups;
    v->fighter->x619_costume_id=(u8)costume;
    for(size_t i=0;i<groups;i++){v->fighter->x5F4_arr[i].prev=-1;v->fighter->x5F4_arr[i].idx=-1;}
    for(size_t i=0;i<drawables;i++)v->list.data[i]=v->objects+i;
    if(lookup!=UINT32_MAX){
        if(!range(a,lookup,groups*8))goto fail;
        v->vis.xC[0]=calloc(groups?groups:1,sizeof(FtPartsVisLookup));if(!v->vis.xC[0])goto fail;
        for(size_t i=0;i<groups;i++){
            uint32_t count,entries;
            if(!melee_archive_u32(a,lookup+i*8,&count) || count>INT_MAX || !ref(a,lookup+i*8+4,&entries) || (count && !range(a,entries,(size_t)count*8)))goto fail;
            FtPartsVisLookup* group=v->vis.xC[0]+i;group->x0=(int)count;
            group->x4=calloc(count?count:1,sizeof(TempS));if(!group->x4)goto fail;
            for(size_t j=0;j<count;j++){
                uint32_t length,indices;
                if(!melee_archive_u32(a,entries+j*8,&length) || length>INT_MAX || !ref(a,entries+j*8+4,&indices) || (length && !range(a,indices,length)))goto fail;
                TempS* choice=group->x4+j;choice->x0=(int)length;
                choice->x4=malloc(length?length:1);if(!choice->x4)goto fail;
                if(length)memcpy(choice->x4,a->bytes+32+indices,length);
                for(size_t k=0;k<length;k++)if(choice->x4[k]>=drawables)goto fail;
            }
        }
    }
    v->vis.cleared[0]=true;ftParts_80074D7C(&v->vis,0,&v->list);
    for(size_t i=0;i<drawables;i++)v->controlled[i]=!!(v->objects[i].flags&1);
    return v;
fail:melee_visibility_free(v);return NULL;
}
