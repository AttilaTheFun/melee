#include "melee_ness_items.h"
#include <stdlib.h>
struct MeleeNessItems {MeleeItemArticle* owners[11];void* entries[11];};
void melee_ness_items_free(MeleeNessItems* o){if(o){for(unsigned i=0;i<11;i++)melee_item_article_free(o->owners[i]);free(o);}}
void** melee_ness_items_entries(MeleeNessItems* o){return o?o->entries:NULL;}
MeleeNessItems* melee_ness_items_decode(const MeleeArchive*a,u32 root)
{
 u32 table;MeleeHostBool p;if(!a||!melee_archive_pointer(a,root+0x48,&table,&p)||!p||table>a->data_size||a->data_size-table<44)return NULL;
 unsigned kinds[]={It_Kind_Ness_PKFire,It_Kind_Ness_PKFire_Flame,It_Kind_Ness_PKFlush,It_Kind_Ness_PKThunder,It_Kind_Ness_PKThunder1,It_Kind_Ness_PKThunder2,It_Kind_Ness_PKThunder3,It_Kind_Ness_PKThunder4,It_Kind_Ness_PKFlush_Explode,It_Kind_Ness_Bat,It_Kind_Ness_Yoyo},states[]={1,1,3,1,1,1,1,1,1,1,0};
 MeleeNessItems*o=calloc(1,sizeof(*o));if(!o)return NULL;
 for(unsigned i=0;i<11;i++){
  u32 at;if(!melee_archive_pointer(a,table+i*4,&at,&p))goto fail;
  if(!p)goto fail;o->owners[i]=melee_item_article_decode(a,kinds[i],at,states[i]);if(!o->owners[i])goto fail;o->entries[i]=melee_item_article_descriptor(o->owners[i]);
 }
 return o;
fail:melee_ness_items_free(o);return NULL;
}
