#ifndef MELEE_NATIVE_PEACH_ITEMS_H
#define MELEE_NATIVE_PEACH_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleePeachItems MeleePeachItems;
/* Explosion, turnip, parasol, Toad and spores; requires initialized HSD pools. */
MeleePeachItems* melee_peach_items_decode(const MeleeArchive*,u32);
void** melee_peach_items_entries(MeleePeachItems*);
void melee_peach_items_free(MeleePeachItems*);
#endif
