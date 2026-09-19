#include "melee_item_model.h"
#include "melee_scene.h"
#include <stdlib.h>
#include <string.h>
struct MeleeItemModel {ItemModelDesc desc;MeleeScene* scene;};
void melee_item_model_free(MeleeItemModel* m){if(m){melee_scene_free(m->scene);free(m);}}
ItemModelDesc* melee_item_model_descriptor(MeleeItemModel* m){return m?&m->desc:NULL;}
MeleeItemModel* melee_item_model_decode(const MeleeArchive* a,uint32_t at){
    u32 joint,bones,attach;MeleeHostBool present;
    if(!a||at>a->data_size||a->data_size-at<16||!melee_archive_pointer(a,at,&joint,&present)||
       !melee_archive_u32(a,at+4,&bones)||bones>100||!melee_archive_u32(a,at+8,&attach))return NULL;
    MeleeItemModel* m=calloc(1,sizeof(*m));if(!m)return NULL;
    m->desc.x4_bone_count=bones;memcpy(&m->desc.x8_bone_attach_id,&attach,4);m->desc.xC_bit_field=a->bytes[32+at+12];
    if(present){
        m->scene=melee_scene_decode(a,joint);if(!m->scene)goto fail;
        if(bones&&melee_scene_joint_count(m->scene)>100)goto fail;
        m->desc.x0_joint=melee_scene_joint_descriptor(m->scene);melee_scene_release_objects(m->scene);
    }else if(bones)goto fail;
    return m;
fail:melee_item_model_free(m);return NULL;
}
