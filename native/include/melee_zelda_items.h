#ifndef MELEE_NATIVE_ZELDA_ITEMS_H
#define MELEE_NATIVE_ZELDA_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeZeldaItems MeleeZeldaItems;
/* Din's Fire projectile and explosion; requires initialized HSD pools. */
MeleeZeldaItems* melee_zelda_items_decode(const MeleeArchive*,u32);
void** melee_zelda_items_entries(MeleeZeldaItems*);
void melee_zelda_items_free(MeleeZeldaItems*);
#endif
