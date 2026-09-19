#include "melee_item_mushroom.h"
#include "melee_scene.h"
#include <math.h>
#include <stdlib.h>
struct MeleeItemMushroom {KinokoAttrs attrs;MeleeScene* scenes[2];};
void melee_item_mushroom_free(MeleeItemMushroom* m){if(m){for(unsigned i=0;i<2;i++)melee_scene_free(m->scenes[i]);free(m);}}
KinokoAttrs* melee_item_mushroom_attributes(MeleeItemMushroom* m){return m?&m->attrs:NULL;}
MeleeItemMushroom* melee_item_mushroom_decode(const MeleeArchive* a,uint32_t at,uint32_t joint){
    if(!a||(uint64_t)at+16>a->data_size)return NULL;
    MeleeItemMushroom* m=calloc(1,sizeof(*m));if(!m)return NULL;
    if(!melee_archive_f32(a,at,&m->attrs.x0)||!isfinite(m->attrs.x0)||
       !melee_archive_f32(a,at+4,&m->attrs.x4)||!isfinite(m->attrs.x4))goto fail;
    for(unsigned i=0;i<2;i++){
        u32 anim;MeleeHostBool present;if(!melee_archive_pointer(a,at+8+4*i,&anim,&present)||!present)goto fail;
        m->scenes[i]=melee_scene_decode(a,joint);if(!m->scenes[i]||!melee_scene_bind_joint_root(m->scenes[i],anim))goto fail;
        m->attrs.animations[i]=melee_scene_animation_descriptor(m->scenes[i]);melee_scene_release_objects(m->scenes[i]);
    }
    return m;
fail:melee_item_mushroom_free(m);return NULL;
}
