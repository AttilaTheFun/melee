#ifndef MELEE_NATIVE_SHEIK_ITEMS_H
#define MELEE_NATIVE_SHEIK_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeSheikItems MeleeSheikItems;
/* Needles, Vanish and chain; requires initialized HSD pools. */
MeleeSheikItems* melee_sheik_items_decode(const MeleeArchive*,u32);
void** melee_sheik_items_entries(MeleeSheikItems*);
void melee_sheik_items_free(MeleeSheikItems*);
#endif
