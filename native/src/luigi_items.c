#include "melee_luigi_items.h"
#include <stdlib.h>
struct MeleeLuigiItems {MeleeItemArticle* owner;void* entries[1];};
void melee_luigi_items_free(MeleeLuigiItems* o){if(o){melee_item_article_free(o->owner);free(o);}}
void** melee_luigi_items_entries(MeleeLuigiItems* o){return o?o->entries:NULL;}
MeleeLuigiItems* melee_luigi_items_decode(const MeleeArchive* a,u32 root)
{
    u32 table,article;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x48,&table,&present)||!present||
       !melee_archive_pointer(a,table,&article,&present)||!present)return NULL;
    MeleeLuigiItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    o->owner=melee_item_article_decode(a,It_Kind_Luigi_Fire,article,1);
    if(!o->owner){melee_luigi_items_free(o);return NULL;}
    o->entries[0]=melee_item_article_descriptor(o->owner);return o;
}
