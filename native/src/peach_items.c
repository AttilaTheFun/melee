#include "melee_peach_items.h"
#include <stdlib.h>
struct MeleePeachItems {MeleeItemArticle* owners[5];void* entries[5];};
void melee_peach_items_free(MeleePeachItems* o){if(o){for(unsigned i=0;i<5;i++)melee_item_article_free(o->owners[i]);free(o);}}
void** melee_peach_items_entries(MeleePeachItems* o){return o?o->entries:NULL;}
MeleePeachItems* melee_peach_items_decode(const MeleeArchive* a,u32 root)
{
    u32 table;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x48,&table,&present)||!present)return NULL;
    const unsigned counts[]={2,3,2,2,1};
    const ItemKind kinds[]={It_Kind_Peach_Explode,It_Kind_Peach_Turnip,It_Kind_Peach_Parasol,It_Kind_Peach_Toad,It_Kind_Peach_ToadSpore};
    MeleePeachItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    for(unsigned i=0;i<5;i++){
        u32 article;if(!melee_archive_pointer(a,table+4*i,&article,&present)||!present)goto fail;
        o->owners[i]=melee_item_article_decode(a,kinds[i],article,counts[i]);if(!o->owners[i])goto fail;
        o->entries[i]=melee_item_article_descriptor(o->owners[i]);
    }
    return o;
fail:melee_peach_items_free(o);return NULL;
}
