#include "melee_kirby_items.h"
#include <stdlib.h>
#include "melee_scene.h"
struct MeleeKirbyItems {MeleeItemArticle* owners[4];void* entries[5];MeleeScene* star;};
void melee_kirby_items_free(MeleeKirbyItems* o){if(o){for(unsigned i=0;i<4;i++)melee_item_article_free(o->owners[i]);melee_scene_free(o->star);free(o);}}
void** melee_kirby_items_entries(MeleeKirbyItems* o){return o?o->entries:NULL;}
MeleeKirbyItems* melee_kirby_items_decode(const MeleeArchive* a,u32 root)
{
    u32 table;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x48,&table,&present)||!present)return NULL;
    const unsigned counts[]={1,1,1,1};
    const ItemKind kinds[]={It_Kind_Kirby_CBeam,It_Kind_Kirby_Hammer,It_Kind_Unk1,It_Kind_Unk2};
    MeleeKirbyItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    for(unsigned i=0;i<4;i++){
        u32 article;if(!melee_archive_pointer(a,table+4*i,&article,&present)||!present)goto fail;
        o->owners[i]=melee_item_article_decode(a,kinds[i],article,counts[i]);if(!o->owners[i])goto fail;
        o->entries[i]=melee_item_article_descriptor(o->owners[i]);
    }
    u32 star;if(!melee_archive_pointer(a,table+16,&star,&present)||!present)goto fail;
    o->star=melee_scene_decode(a,star);if(!o->star)goto fail;
    o->entries[4]=melee_scene_joint_descriptor(o->star);melee_scene_release_objects(o->star);
    return o;
fail:melee_kirby_items_free(o);return NULL;
}
