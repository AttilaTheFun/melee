#ifndef MELEE_NATIVE_GAMEWATCH_ITEMS_H
#define MELEE_NATIVE_GAMEWATCH_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeGameWatchItems MeleeGameWatchItems;
/* Ten attack props and Chef projectiles plus the fighter outline lookup.
 * Returns eleven entries; requires initialized HSD pools. */
MeleeGameWatchItems* melee_gamewatch_items_decode(const MeleeArchive*,u32);
void** melee_gamewatch_items_entries(MeleeGameWatchItems*);
void melee_gamewatch_items_free(MeleeGameWatchItems*);
#endif
