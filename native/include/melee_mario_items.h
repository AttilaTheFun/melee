#ifndef MELEE_NATIVE_MARIO_ITEMS_H
#define MELEE_NATIVE_MARIO_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeMarioItems MeleeMarioItems;
/* Four original OnLoad slots; the other character's two slots remain NULL. */
MeleeMarioItems* melee_mario_items_decode(const MeleeArchive*,u32 ft_data,MeleeHostBool doctor);
void** melee_mario_items_entries(MeleeMarioItems*);
void melee_mario_items_free(MeleeMarioItems*);
#endif
