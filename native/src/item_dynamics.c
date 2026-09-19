#include "melee_item_dynamics.h"
#include <melee/lb/types.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(struct lb_00F9_UnkDesc1Inner)==60,"Dynamics parameter scalars");
static int array(const MeleeArchive* a,u32 slot,u32 n,unsigned stride,u32* at){
    MeleeHostBool present;return melee_archive_pointer(a,slot,at,&present)&&(!n||(present&&*at<=a->data_size&&(size_t)n*stride<=a->data_size-*at));
}
static int floats(const MeleeArchive* a,u32 at,void* out,unsigned n){
    for(unsigned i=0;i<n;i++){float f;if(!melee_archive_f32(a,at+4*i,&f)||!isfinite(f))return 0;memcpy((u8*)out+4*i,&f,4);}return 1;
}
void melee_item_dynamics_free(ItemDynamics* d){if(d){if(d->dyn_descs)for(int i=0;i<d->count;i++)free(d->dyn_descs[i].dyn_desc.params);free(d->dyn_descs);free(d->collision_descs);free(d);}}
ItemDynamics* melee_item_dynamics_decode_extra(const MeleeArchive* a,u32 at,unsigned bones,unsigned extra){
    u32 n,nc,ds,cs;if(!a||at>a->data_size||a->data_size-at<16||!melee_archive_u32(a,at,&n)||n>24||
       extra>24-n||!melee_archive_u32(a,at+8,&nc)||nc>2||!array(a,at+4,n+extra,24,&ds)||!array(a,at+12,nc,20,&cs))return NULL;
    n+=extra;
    ItemDynamics* d=calloc(1,sizeof(*d));if(!d)return NULL;d->count=n;d->collision_count=nc;
    d->dyn_descs=calloc(n?n:1,sizeof(*d->dyn_descs));d->collision_descs=calloc(nc?nc:1,sizeof(*d->collision_descs));if(!d->dyn_descs||!d->collision_descs)goto fail;
    for(unsigned i=0;i<n;i++){
        BoneDynamicsDesc* b=d->dyn_descs+i;u32 root=ds+24*i,bone,count,params;
        if(!melee_archive_u32(a,root,&bone)||bone>=bones||bone>0x7fffffff||!melee_archive_u32(a,root+8,&count)||count>320||!array(a,root+4,count,60,&params))goto fail;
        b->bone_id=bone;b->dyn_desc.count=count;if(!floats(a,root+12,&b->dyn_desc.pos,3))goto fail;
        b->dyn_desc.params=calloc(count?count:1,sizeof(*b->dyn_desc.params));if(!b->dyn_desc.params)goto fail;
        for(unsigned k=0;k<count;k++)if(!floats(a,params+60*k,b->dyn_desc.params+k,15))goto fail;
    }
    for(unsigned i=0;i<nc;i++){
        ItCollDynamicsDesc* c=d->collision_descs+i;u32 bone;if(!melee_archive_u32(a,cs+20*i,&bone)||bone>=bones||bone>0x7fffffff)goto fail;c->bone_id=bone;
        if(!floats(a,cs+20*i+4,&c->offset,3)||!floats(a,cs+20*i+16,&c->size,1)||c->size<0)goto fail;
    }return d;
fail:melee_item_dynamics_free(d);return NULL;
}

ItemDynamics* melee_item_dynamics_decode(const MeleeArchive* a,u32 at,unsigned bones)
{return melee_item_dynamics_decode_extra(a,at,bones,0);}
