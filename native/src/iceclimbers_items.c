#include "melee_iceclimbers_items.h"
#include <stdlib.h>
struct MeleeIceClimbersItems {MeleeItemArticle* owners[3];void* entries[3];};
void melee_iceclimbers_items_free(MeleeIceClimbersItems* o){if(o){for(unsigned i=0;i<3;i++)melee_item_article_free(o->owners[i]);free(o);}}
void** melee_iceclimbers_items_entries(MeleeIceClimbersItems* o){return o?o->entries:NULL;}
MeleeIceClimbersItems* melee_iceclimbers_items_decode(const MeleeArchive* a,u32 root)
{
    u32 table;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x48,&table,&present)||!present)return NULL;
    const unsigned counts[]={1,1,0};
    const ItemKind kinds[]={It_Kind_IceClimber_Ice,It_Kind_IceClimber_Blizzard,It_Kind_IceClimber_GumStrings};
    MeleeIceClimbersItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    for(unsigned i=0;i<3;i++){
        u32 article;if(!melee_archive_pointer(a,table+4*i,&article,&present)||!present)goto fail;
        o->owners[i]=melee_item_article_decode(a,kinds[i],article,counts[i]);if(!o->owners[i])goto fail;
        o->entries[i]=melee_item_article_descriptor(o->owners[i]);
    }
    return o;
fail:melee_iceclimbers_items_free(o);return NULL;
}
