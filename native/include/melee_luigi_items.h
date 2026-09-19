#ifndef MELEE_NATIVE_LUIGI_ITEMS_H
#define MELEE_NATIVE_LUIGI_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeLuigiItems MeleeLuigiItems;
/* Luigi has one fireball article slot. Requires HSD pools. */
MeleeLuigiItems* melee_luigi_items_decode(const MeleeArchive*,u32 ft_data);
void** melee_luigi_items_entries(MeleeLuigiItems*);
void melee_luigi_items_free(MeleeLuigiItems*);
#endif
