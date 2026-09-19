#include "melee_link_items.h"
#include "melee_scene.h"
#include <stdlib.h>
struct MeleeLinkItems {MeleeItemArticle* owners[6];void* entries[7];MeleeScene* accessory;};
void melee_link_items_free(MeleeLinkItems* o){if(o){for(unsigned i=0;i<6;i++)melee_item_article_free(o->owners[i]);melee_scene_free(o->accessory);free(o);}}
void** melee_link_items_entries(MeleeLinkItems* o){return o?o->entries:NULL;}
MeleeLinkItems* melee_link_items_decode(const MeleeArchive* a,u32 root,int young)
{
    u32 table;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x48,&table,&present)||!present)return NULL;
    const unsigned counts[]={4,3,0,1,6,2};
    const ItemKind kinds[]={young?It_Kind_CLink_Bomb:It_Kind_Link_Bomb,young?It_Kind_CLink_Boomerang:It_Kind_Link_Boomerang,young?It_Kind_CLink_HShot:It_Kind_Link_HShot,young?It_Kind_CLink_Arrow:It_Kind_Link_Arrow,young?It_Kind_CLink_Bow:It_Kind_Link_Bow,It_Kind_CLink_Milk};
    MeleeLinkItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    for(unsigned i=0;i<6;i++){
        u32 at;if(!melee_archive_pointer(a,table+4*i,&at,&present))goto fail;
        if(i==5&&!young){if(present)goto fail;continue;}
        if(!present)goto fail;
        o->owners[i]=melee_item_article_decode(a,kinds[i],at,counts[i]);if(!o->owners[i])goto fail;
        o->entries[i]=melee_item_article_descriptor(o->owners[i]);
    }
    u32 model;if(!melee_archive_pointer(a,table+24,&model,&present)||!present)goto fail;
    o->accessory=melee_scene_decode(a,model);if(!o->accessory)goto fail;
    melee_scene_release_objects(o->accessory);o->entries[6]=melee_scene_joint_descriptor(o->accessory);
    return o;
fail:melee_link_items_free(o);return NULL;
}
