#include "melee_hand_items.h"
#include <stdlib.h>
struct MeleeHandItems {MeleeItemArticle* owners[3];void* entries[3];};
void melee_hand_items_free(MeleeHandItems* o){if(o){for(unsigned i=0;i<3;i++)melee_item_article_free(o->owners[i]);free(o);}}
void** melee_hand_items_entries(MeleeHandItems* o){return o?o->entries:NULL;}
MeleeHandItems* melee_hand_items_decode(const MeleeArchive* a,u32 root,MeleeHostBool crazy)
{
    u32 table;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x48,&table,&present)||!present)return NULL;
    unsigned kinds[]={crazy?It_Kind_CrazyHand_Laser:It_Kind_MasterHand_Laser,
        crazy?It_Kind_CrazyHand_Bullet:It_Kind_MasterHand_Bullet,It_Kind_CrazyHand_Bomb};
    const unsigned states[]={1,2,2};
    MeleeHandItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    for(unsigned i=0;i<3;i++){
        u32 at;if(!melee_archive_pointer(a,table+4*i,&at,&present)||!present)goto fail;
        o->owners[i]=melee_item_article_decode(a,kinds[i],at,states[i]);if(!o->owners[i])goto fail;
        o->entries[i]=melee_item_article_descriptor(o->owners[i]);
    }
    return o;
fail:melee_hand_items_free(o);return NULL;
}
