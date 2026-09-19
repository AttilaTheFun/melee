#include "melee_mario_items.h"
#include <stdlib.h>
struct MeleeMarioItems {MeleeItemArticle* owners[4];void* entries[4];};
void melee_mario_items_free(MeleeMarioItems* o){if(o){for(unsigned i=0;i<4;i++)melee_item_article_free(o->owners[i]);free(o);}}
void** melee_mario_items_entries(MeleeMarioItems* o){return o?o->entries:NULL;}
MeleeMarioItems* melee_mario_items_decode(const MeleeArchive* a,u32 root,MeleeHostBool doctor)
{
    u32 table;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x48,&table,&present)||!present||
       table>a->data_size||a->data_size-table<16)return NULL;
    const unsigned kinds[]={It_Kind_Mario_Fire,It_Kind_DrMario_Vitamin,It_Kind_Mario_Cape,It_Kind_DrMario_Sheet};
    const unsigned states[]={1,6,2,2};
    MeleeMarioItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    for(unsigned i=0;i<4;i++){
        u32 at;if(!melee_archive_pointer(a,table+4*i,&at,&present))goto fail;
        if((i&1)!=(unsigned)!!doctor){if(present)goto fail;continue;}
        if(!present)goto fail;
        o->owners[i]=melee_item_article_decode(a,kinds[i],at,states[i]);
        if(!o->owners[i])goto fail;
        o->entries[i]=melee_item_article_descriptor(o->owners[i]);
    }
    return o;
fail:melee_mario_items_free(o);return NULL;
}
