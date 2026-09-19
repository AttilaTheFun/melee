#ifndef MELEE_NATIVE_ICECLIMBERS_ITEMS_H
#define MELEE_NATIVE_ICECLIMBERS_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeIceClimbersItems MeleeIceClimbersItems;
/* Popo Ice Shot, Blizzard and recovery rope; requires initialized HSD pools.
 * Nana has unresolved shared model references and cannot be decoded alone. */
MeleeIceClimbersItems* melee_iceclimbers_items_decode(const MeleeArchive*,u32);
void** melee_iceclimbers_items_entries(MeleeIceClimbersItems*);
void melee_iceclimbers_items_free(MeleeIceClimbersItems*);
#endif
