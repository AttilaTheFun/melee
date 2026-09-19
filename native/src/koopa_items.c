#include "melee_koopa_items.h"
#include <stdlib.h>
struct MeleeKoopaItems {MeleeItemArticle* owner;void* entries[1];};
void melee_koopa_items_free(MeleeKoopaItems* o){if(o){melee_item_article_free(o->owner);free(o);}}
void** melee_koopa_items_entries(MeleeKoopaItems* o){return o?o->entries:NULL;}
MeleeKoopaItems* melee_koopa_items_decode(const MeleeArchive* a,u32 root)
{
    u32 table,article;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x48,&table,&present)||!present||
       !melee_archive_pointer(a,table,&article,&present)||!present)return NULL;
    MeleeKoopaItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    o->owner=melee_item_article_decode(a,It_Kind_Koopa_Flame,article,1);
    if(!o->owner){melee_koopa_items_free(o);return NULL;}
    o->entries[0]=melee_item_article_descriptor(o->owner);return o;
}
