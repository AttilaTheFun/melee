#include "melee_item_wstar.h"
#include "melee_scene.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
struct MeleeItemWstar {itWstarAttributes* attrs;MeleeScene* scenes[7];};
void melee_item_wstar_free(MeleeItemWstar* w){if(w){for(unsigned i=0;i<7;i++)melee_scene_free(w->scenes[i]);free(w->attrs);free(w);}}
itWstarAttributes* melee_item_wstar_attributes(MeleeItemWstar* w){return w?w->attrs:NULL;}
MeleeItemWstar* melee_item_wstar_decode(const MeleeArchive* a,uint32_t at,uint32_t joint){
    u32 count;if(!a||(uint64_t)at+40>a->data_size||!melee_archive_u32(a,at+36,&count)||
        count<2||count>7||(uint64_t)at+40+8*count>a->data_size)return NULL;
    MeleeItemWstar* w=calloc(1,sizeof(*w));if(!w)return NULL;
    w->attrs=calloc(1,offsetof(itWstarAttributes,x28_entries)+count*sizeof(itWstarAttrEntry));if(!w->attrs)goto fail;
    _Static_assert(offsetof(itWstarAttributes,x24_count)==36,"Warp Star scalar prefix");
    for(unsigned i=0;i<9;i++){float v;if(!melee_archive_f32(a,at+4*i,&v)||!isfinite(v))goto fail;memcpy((u8*)w->attrs+4*i,&v,4);}
    if(w->attrs->x4==0)goto fail;w->attrs->x24_count=count;
    for(unsigned i=0;i<count;i++){
        u32 anim,sfx;MeleeHostBool present;
        if(!melee_archive_pointer(a,at+40+8*i,&anim,&present)||!present||!melee_archive_u32(a,at+44+8*i,&sfx))goto fail;
        w->scenes[i]=melee_scene_decode(a,joint);if(!w->scenes[i]||!melee_scene_bind_joint_root(w->scenes[i],anim))goto fail;
        w->attrs->x28_entries[i].x0_anim_joint=melee_scene_animation_descriptor(w->scenes[i]);
        memcpy(&w->attrs->x28_entries[i].x4_sfx,&sfx,4);melee_scene_release_objects(w->scenes[i]);
    }
    return w;
fail:melee_item_wstar_free(w);return NULL;
}
