#ifndef MELEE_NATIVE_HAND_ITEMS_H
#define MELEE_NATIVE_HAND_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeHandItems MeleeHandItems;
/* Owns the three serialized boss projectile articles. Requires HSD pools. */
MeleeHandItems* melee_hand_items_decode(const MeleeArchive*,u32 ft_data,MeleeHostBool crazy);
void** melee_hand_items_entries(MeleeHandItems*);
void melee_hand_items_free(MeleeHandItems*);
#endif
