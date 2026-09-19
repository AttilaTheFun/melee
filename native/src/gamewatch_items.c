#include "melee_gamewatch_items.h"
#include <stdlib.h>
#include <string.h>
#include <melee/ft/dobjlist.h>
_Static_assert(offsetof(itGamewatchchef_ItemVars,x4)==sizeof(void*),"Chef index follows shared outline pointer");
struct MeleeGameWatchItems {MeleeItemArticle* owners[10];void* entries[11];FtPartsVisLookup outline[11];};
void melee_gamewatch_items_free(MeleeGameWatchItems* o){if(o){for(unsigned i=0;i<10;i++)melee_item_article_free(o->owners[i]);for(unsigned i=0;i<11;i++){if(o->outline[i].x4)for(int j=0;j<o->outline[i].x0;j++)free(o->outline[i].x4[j].x4);free(o->outline[i].x4);}free(o);}}
void** melee_gamewatch_items_entries(MeleeGameWatchItems* o){return o?o->entries:NULL;}
MeleeGameWatchItems* melee_gamewatch_items_decode(const MeleeArchive* a,u32 root)
{
    u32 table;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x48,&table,&present)||!present)return NULL;
    const unsigned counts[]={4,1,1,2,2,2,1,2,2,2};
    const ItemKind kinds[]={It_Kind_GameWatch_Greenhouse,It_Kind_GameWatch_Manhole,It_Kind_GameWatch_Fire,It_Kind_GameWatch_Parachute,It_Kind_GameWatch_Turtle,It_Kind_GameWatch_Breath,It_Kind_GameWatch_Judge,It_Kind_GameWatch_Panic,It_Kind_GameWatch_Chef,It_Kind_GameWatch_Rescue};
    MeleeGameWatchItems* o=calloc(1,sizeof(*o));if(!o)return NULL;
    for(unsigned i=0;i<10;i++){
        u32 article;if(!melee_archive_pointer(a,table+4*i,&article,&present)||!present)goto fail;
        o->owners[i]=melee_item_article_decode(a,kinds[i],article,counts[i]);if(!o->owners[i])goto fail;
        o->entries[i]=melee_item_article_descriptor(o->owners[i]);
    }
    u32 outline;if(!melee_archive_pointer(a,table+40,&outline,&present)||!present)goto fail;
    for(unsigned i=0;i<11;i++){
        u32 count,choices;if(!melee_archive_u32(a,outline+8*i,&count)||count>128||!melee_archive_pointer(a,outline+8*i+4,&choices,&present)||(count&&!present))goto fail;
        o->outline[i].x0=count;o->outline[i].x4=calloc(count?count:1,sizeof(TempS));if(!o->outline[i].x4)goto fail;
        for(unsigned j=0;j<count;j++){
            u32 length,list;if(!melee_archive_u32(a,choices+8*j,&length)||length>124||!melee_archive_pointer(a,choices+8*j+4,&list,&present)||(length&&!present)||list>a->data_size||length>a->data_size-list)goto fail;
            TempS* choice=&o->outline[i].x4[j];choice->x0=length;choice->x4=malloc(length?length:1);if(!choice->x4)goto fail;
            memcpy(choice->x4,a->bytes+32+list,length);for(unsigned k=0;k<length;k++)if(choice->x4[k]>=124)goto fail;
        }
    }
    o->entries[10]=o->outline;
    return o;
fail:melee_gamewatch_items_free(o);return NULL;
}
