#include "melee_fox_items.h"
#include <stdlib.h>
struct MeleeFoxItems {MeleeItemArticle* owners[4];void* entries[4];};
void melee_fox_items_free(MeleeFoxItems* o){if(o){for(unsigned i=0;i<4;i++)melee_item_article_free(o->owners[i]);free(o);}}
void** melee_fox_items_entries(MeleeFoxItems* o){return o?o->entries:NULL;}
MeleeFoxItems* melee_fox_items_decode(const MeleeArchive*a,u32 root,MeleeHostBool falco)
{
 u32 table;MeleeHostBool p;if(!a||!melee_archive_pointer(a,root+0x48,&table,&p)||!p||table>a->data_size||a->data_size-table<16)return NULL;
 unsigned kinds[]={falco?It_Kind_Falco_Laser:It_Kind_Fox_Laser,falco?It_Kind_Falco_Blaster:It_Kind_Fox_Blaster,It_Kind_Fox_Illusion,It_Kind_Falco_Phantasm},states[]={2,9,3,3};
 MeleeFoxItems*o=calloc(1,sizeof(*o));if(!o)return NULL;
 for(unsigned i=0;i<4;i++){
  u32 at;if(!melee_archive_pointer(a,table+i*4,&at,&p))goto fail;
  if(i==(falco?2:3)){if(p)goto fail;continue;}
  if(!p)goto fail;o->owners[i]=melee_item_article_decode(a,kinds[i],at,states[i]);if(!o->owners[i])goto fail;o->entries[i]=melee_item_article_descriptor(o->owners[i]);
 }
 return o;
fail:melee_fox_items_free(o);return NULL;
}
