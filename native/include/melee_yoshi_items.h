#ifndef MELEE_NATIVE_YOSHI_ITEMS_H
#define MELEE_NATIVE_YOSHI_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeYoshiItems MeleeYoshiItems;
/* Thrown egg, star, Egg Lay. Requires initialized HSD pools. */
MeleeYoshiItems* melee_yoshi_items_decode(const MeleeArchive*,u32);
void** melee_yoshi_items_entries(MeleeYoshiItems*);
void melee_yoshi_items_free(MeleeYoshiItems*);
#endif
