#include "melee_item_animation.h"
MeleeScene* melee_item_animation_decode(const MeleeArchive* a,uint32_t joint,uint32_t state){
    uint32_t roots[3];MeleeHostBool present[3];
    if(!a||state>a->data_size||a->data_size-state<16||joint==UINT32_MAX)return NULL;
    for(unsigned i=0;i<3;i++)if(!melee_archive_pointer(a,state+4*i,&roots[i],&present[i]))return NULL;
    MeleeScene* s=melee_scene_decode(a,joint);if(!s)return NULL;
    for(unsigned i=0;i<3;i++)if(present[i]){
        int okay=i==0?melee_scene_bind_joints(s,roots[i]):i==1?melee_scene_bind_materials(s,roots[i]):melee_scene_bind_shapes(s,roots[i]);
        if(!okay){melee_scene_free(s);return NULL;}
    }
    return s;
}
