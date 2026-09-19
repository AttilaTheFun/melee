#include "melee_fighter_models.h"
#include <stdlib.h>

struct MeleeFighterModels { MeleeScene* scenes[3]; };
void melee_fighter_models_free(MeleeFighterModels* owner)
{
    if (owner) {
        for (unsigned i=0;i<3;i++) melee_scene_free(owner->scenes[i]);
        free(owner);
    }
}
HSD_Joint* melee_fighter_models_joint(MeleeFighterModels* owner,unsigned index)
{
    return owner&&index<3?melee_scene_joint_descriptor(owner->scenes[index]):NULL;
}
HSD_AnimJoint* melee_fighter_models_respawn_animation(MeleeFighterModels* owner)
{
    return owner?melee_scene_animation_descriptor(owner->scenes[0]):NULL;
}
static int required(const MeleeArchive* a,u32 at,u32* target)
{
    MeleeHostBool present;
    return melee_archive_pointer(a,at,target,&present)&&present;
}
MeleeFighterModels* melee_fighter_models_decode(const MeleeArchive* a)
{
    u32 root,pair,joints[3],animation;
    if(!a||!melee_archive_find(a,"ftLoadCommonData",&root)||
       !required(a,root+32,&pair)||!required(a,pair,&joints[0])||
       !required(a,pair+4,&animation)||!required(a,root+64,&joints[1])||
       !required(a,root+80,&joints[2]))return NULL;
    MeleeFighterModels* owner=calloc(1,sizeof(*owner));if(!owner)return NULL;
    for(unsigned i=0;i<3;i++){
        owner->scenes[i]=melee_scene_decode(a,joints[i]);
        if(!owner->scenes[i])goto fail;
        if(!i&&!melee_scene_bind_joints(owner->scenes[i],animation))goto fail;
        melee_scene_release_objects(owner->scenes[i]);
    }
    return owner;
fail:melee_fighter_models_free(owner);return NULL;
}
