#include "melee_item_hurtbones.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(ItHurtBoneDesc)==32,"Item hurtbone scalar layout");
_Static_assert(offsetof(ItHurtBoneDesc,a_offset)==4&&offsetof(ItHurtBoneDesc,b_offset)==16&&offsetof(ItHurtBoneDesc,scale)==28,"Item hurtbone offsets");
void melee_item_hurtbones_free(ItHurtBoneList* h){if(h){free(h->descs);free(h);}}
ItHurtBoneList* melee_item_hurtbones_decode(const MeleeArchive* a,uint32_t at,unsigned bone_count){
    u32 n,descs;MeleeHostBool present;
    if(!a||at>a->data_size||a->data_size-at<8||!melee_archive_u32(a,at,&n)||n>2||
       !melee_archive_pointer(a,at+4,&descs,&present)||(n&&(!present||descs>a->data_size||a->data_size-descs<32u*n)))return NULL;
    ItHurtBoneList* h=calloc(1,sizeof(*h));if(!h)return NULL;h->count=n;
    h->descs=calloc(n?n:1,sizeof(*h->descs));if(!h->descs)goto fail;
    for(unsigned i=0;i<n;i++){
        ItHurtBoneDesc* d=h->descs+i;u32 bone;float v[7];
        if(!melee_archive_u32(a,descs+32*i,&bone)||(bone&&bone>=bone_count)||bone>0x7fffffff)goto fail;
        d->bone_id=bone;
        for(unsigned k=0;k<7;k++)if(!melee_archive_f32(a,descs+32*i+4+4*k,v+k)||!isfinite(v[k]))goto fail;
        if(v[6]<0)goto fail;
        d->a_offset=(Vec3){v[0],v[1],v[2]};d->b_offset=(Vec3){v[3],v[4],v[5]};d->scale=v[6];
    }return h;
fail:melee_item_hurtbones_free(h);return NULL;
}
