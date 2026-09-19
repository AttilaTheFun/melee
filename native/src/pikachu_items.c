#include "melee_pikachu_items.h"
#include <stdlib.h>
struct MeleePikachuItems {MeleeItemArticle* owners[3];void* entries[3];};
void melee_pikachu_items_free(MeleePikachuItems* o){if(o){for(unsigned i=0;i<3;i++)melee_item_article_free(o->owners[i]);free(o);}}
void** melee_pikachu_items_entries(MeleePikachuItems* o){return o?o->entries:NULL;}
MeleePikachuItems* melee_pikachu_items_decode(const MeleeArchive* a,u32 root,MeleeHostBool pichu)
{
    u32 table;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x48,&table,&present)||!present)return NULL;
    const unsigned counts[]={1,2,1};
    const ItemKind kinds[]={pichu?It_Kind_Pichu_Thunder:It_Kind_Pikachu_Thunder,
        pichu?It_Kind_Pichu_TJolt_Ground:It_Kind_Pikachu_TJolt_Ground,
        pichu?It_Kind_Pichu_TJolt_Air:It_Kind_Pikachu_TJolt_Air};
    MeleePikachuItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    for(unsigned i=0;i<3;i++){
        u32 article;if(!melee_archive_pointer(a,table+4*i,&article,&present)||!present)goto fail;
        o->owners[i]=melee_item_article_decode(a,kinds[i],article,counts[i]);if(!o->owners[i])goto fail;
        o->entries[i]=melee_item_article_descriptor(o->owners[i]);
    }
    return o;
fail:melee_pikachu_items_free(o);return NULL;
}
