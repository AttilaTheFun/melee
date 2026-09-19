#include "melee_sheik_items.h"
#include <stdlib.h>
#include "melee_scene.h"
struct MeleeSheikItems {MeleeItemArticle* owners[4];void* entries[6];MeleeScene* chain_poses[2];};
void melee_sheik_items_free(MeleeSheikItems* o){if(o){for(unsigned i=0;i<4;i++)melee_item_article_free(o->owners[i]);for(unsigned i=0;i<2;i++)melee_scene_free(o->chain_poses[i]);free(o);}}
void** melee_sheik_items_entries(MeleeSheikItems* o){return o?o->entries:NULL;}
MeleeSheikItems* melee_sheik_items_decode(const MeleeArchive* a,u32 root)
{
    u32 table;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x48,&table,&present)||!present)return NULL;
    const unsigned counts[]={5,1,1,0};
    const ItemKind kinds[]={It_Kind_Seak_NeedleThrow,It_Kind_Seak_NeedleHeld,It_Kind_Seak_Vanish,It_Kind_Seak_Chain};
    MeleeSheikItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    for(unsigned i=0;i<4;i++){
        u32 article;if(!melee_archive_pointer(a,table+4*i,&article,&present)||!present)goto fail;
        o->owners[i]=melee_item_article_decode(a,kinds[i],article,counts[i]);if(!o->owners[i])goto fail;
        o->entries[i]=melee_item_article_descriptor(o->owners[i]);
    }
    for(unsigned i=0;i<2;i++){
        u32 model;if(!melee_archive_pointer(a,table+16+4*i,&model,&present)||!present)goto fail;
        o->chain_poses[i]=melee_scene_decode(a,model);if(!o->chain_poses[i])goto fail;
        melee_scene_release_objects(o->chain_poses[i]);
        o->entries[4+i]=melee_scene_joint_descriptor(o->chain_poses[i]);
    }
    return o;
fail:melee_sheik_items_free(o);return NULL;
}
