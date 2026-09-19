#include "melee_mewtwo_items.h"
#include <stdlib.h>
struct MeleeMewtwoItems {MeleeItemArticle* owners[2];void* entries[2];};
void melee_mewtwo_items_free(MeleeMewtwoItems* o){if(o){for(unsigned i=0;i<2;i++)melee_item_article_free(o->owners[i]);free(o);}}
void** melee_mewtwo_items_entries(MeleeMewtwoItems* o){return o?o->entries:NULL;}
MeleeMewtwoItems* melee_mewtwo_items_decode(const MeleeArchive* a,u32 root)
{
    u32 table;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x48,&table,&present)||!present)return NULL;
    const unsigned counts[]={1,10};
    const ItemKind kinds[]={It_Kind_Mewtwo_Disable,It_Kind_Mewtwo_ShadowBall};
    MeleeMewtwoItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    for(unsigned i=0;i<2;i++){
        u32 article;if(!melee_archive_pointer(a,table+4*i,&article,&present)||!present)goto fail;
        o->owners[i]=melee_item_article_decode(a,kinds[i],article,counts[i]);if(!o->owners[i])goto fail;
        o->entries[i]=melee_item_article_descriptor(o->owners[i]);
    }
    return o;
fail:melee_mewtwo_items_free(o);return NULL;
}
