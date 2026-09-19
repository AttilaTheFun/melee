#include "melee_samus_items.h"
#include "melee_scene.h"
#include <melee/ft/kinds/ftSamus/types.h>
#include <stdlib.h>
struct MeleeSamusItems {
    MeleeItemArticle* articles[4];void* entries[5];
    MeleeScene* attachment;MeleeScene* throws[4];
    struct UNK_SAMUS_S1 beam;HSD_AnimJoint* throw_anims[4];
};
void melee_samus_items_free(MeleeSamusItems* o)
{
    if(!o)return;
    for(unsigned i=0;i<4;i++){melee_item_article_free(o->articles[i]);melee_scene_free(o->throws[i]);}
    melee_scene_free(o->attachment);free(o);
}
void** melee_samus_items_entries(MeleeSamusItems* o){return o?o->entries:NULL;}
static int required(const MeleeArchive* a,u32 at,u32* value)
{MeleeHostBool present;return melee_archive_pointer(a,at,value,&present)&&present;}
MeleeSamusItems* melee_samus_items_decode(const MeleeArchive* a,u32 root)
{
    u32 table;if(!a||!required(a,root+0x48,&table))return NULL;
    MeleeSamusItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    const ItemKind kinds[]={It_Kind_Samus_Bomb,It_Kind_Samus_Charge,It_Kind_Samus_Missile,It_Kind_Samus_GBeam};
    const unsigned counts[]={2,9,4,0};
    for(unsigned i=0;i<4;i++){
        u32 at;if(!required(a,table+4*i,&at))goto fail;
        o->articles[i]=melee_item_article_decode(a,kinds[i],at,counts[i]);if(!o->articles[i])goto fail;
        o->entries[i]=melee_item_article_descriptor(o->articles[i]);
    }
    u32 beam,model,anims,base,material;
    if(!required(a,table+16,&beam)||!required(a,beam,&model)||!required(a,beam+4,&anims)||
       !required(a,beam+8,&base)||!required(a,beam+12,&material))goto fail;
    o->attachment=melee_scene_decode(a,model);if(!o->attachment)goto fail;
    if(!melee_scene_bind_joints(o->attachment,base))goto fail;
    if(!melee_scene_bind_materials(o->attachment,material))goto fail;
    o->beam.x0_joint=melee_scene_joint_descriptor(o->attachment);
    o->beam.x8_anim_joint=melee_scene_animation_descriptor(o->attachment);
    o->beam.xC_matanim_joint=melee_scene_material_descriptor(o->attachment);
    melee_scene_release_objects(o->attachment);
    for(unsigned i=0;i<4;i++){
        u32 anim;if(!required(a,anims+4*i,&anim))goto fail;
        o->throws[i]=melee_scene_decode(a,model);if(!o->throws[i]||!melee_scene_bind_joints(o->throws[i],anim))goto fail;
        o->throw_anims[i]=melee_scene_animation_descriptor(o->throws[i]);melee_scene_release_objects(o->throws[i]);
    }
    o->beam.x4_anim_joints=o->throw_anims;o->entries[4]=&o->beam;return o;
fail:melee_samus_items_free(o);return NULL;
}
