#include "melee_yoshi_items.h"
#include <stdlib.h>
#include "melee_scene.h"
struct MeleeYoshiItems {MeleeItemArticle* owners[3];void* entries[4];MeleeScene* capture_model;};
void melee_yoshi_items_free(MeleeYoshiItems* o){if(o){for(unsigned i=0;i<3;i++)melee_item_article_free(o->owners[i]);melee_scene_free(o->capture_model);free(o);}}
void** melee_yoshi_items_entries(MeleeYoshiItems* o){return o?o->entries:NULL;}
MeleeYoshiItems* melee_yoshi_items_decode(const MeleeArchive* a,u32 root)
{
    u32 table;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x48,&table,&present)||!present)return NULL;
    const unsigned counts[]={2,1,0};
    const ItemKind kinds[]={It_Kind_Yoshi_EggThrow,It_Kind_Yoshi_Star,It_Kind_Yoshi_EggLay};
    MeleeYoshiItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    for(unsigned i=0;i<3;i++){
        u32 article;if(!melee_archive_pointer(a,table+4*i,&article,&present)||!present)goto fail;
        o->owners[i]=melee_item_article_decode(a,kinds[i],article,counts[i]);if(!o->owners[i])goto fail;
        o->entries[i]=melee_item_article_descriptor(o->owners[i]);
    }
    u32 model;if(!melee_archive_pointer(a,table+12,&model,&present)||!present)goto fail;
    o->capture_model=melee_scene_decode(a,model);if(!o->capture_model)goto fail;
    melee_scene_release_objects(o->capture_model);
    o->entries[3]=melee_scene_joint_descriptor(o->capture_model);
    return o;
fail:melee_yoshi_items_free(o);return NULL;
}
