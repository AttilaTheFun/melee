#ifndef MELEE_NATIVE_MEWTWO_ITEMS_H
#define MELEE_NATIVE_MEWTWO_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeMewtwoItems MeleeMewtwoItems;
/* Disable and Shadow Ball; requires initialized HSD pools. */
MeleeMewtwoItems* melee_mewtwo_items_decode(const MeleeArchive*,u32);
void** melee_mewtwo_items_entries(MeleeMewtwoItems*);
void melee_mewtwo_items_free(MeleeMewtwoItems*);
#endif
