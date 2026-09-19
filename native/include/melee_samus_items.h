#ifndef MELEE_NATIVE_SAMUS_ITEMS_H
#define MELEE_NATIVE_SAMUS_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeSamusItems MeleeSamusItems;
/* Four articles plus the animated throw attachment; requires HSD pools. */
MeleeSamusItems* melee_samus_items_decode(const MeleeArchive*,u32);
void** melee_samus_items_entries(MeleeSamusItems*);
void melee_samus_items_free(MeleeSamusItems*);
#endif
