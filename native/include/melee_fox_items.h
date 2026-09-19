#ifndef MELEE_NATIVE_FOX_ITEMS_H
#define MELEE_NATIVE_FOX_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeFoxItems MeleeFoxItems;
/* Four article slots consumed by Fox/Falco OnLoad. Requires HSD pools. */
MeleeFoxItems* melee_fox_items_decode(const MeleeArchive*,u32 ft_data,MeleeHostBool falco);
void** melee_fox_items_entries(MeleeFoxItems*);
void melee_fox_items_free(MeleeFoxItems*);
#endif
