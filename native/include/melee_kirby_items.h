#ifndef MELEE_NATIVE_KIRBY_ITEMS_H
#define MELEE_NATIVE_KIRBY_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeKirbyItems MeleeKirbyItems;
/* Four attack articles and a fifth swallowed-opponent star model; requires initialized HSD pools. */
MeleeKirbyItems* melee_kirby_items_decode(const MeleeArchive*,u32);
void** melee_kirby_items_entries(MeleeKirbyItems*);
void melee_kirby_items_free(MeleeKirbyItems*);
#endif
