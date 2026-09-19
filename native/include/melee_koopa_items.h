#ifndef MELEE_NATIVE_KOOPA_ITEMS_H
#define MELEE_NATIVE_KOOPA_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeKoopaItems MeleeKoopaItems;
/* Bowser and Giga Bowser share one flame article slot. Requires HSD pools. */
MeleeKoopaItems* melee_koopa_items_decode(const MeleeArchive*,u32 ft_data);
void** melee_koopa_items_entries(MeleeKoopaItems*);
void melee_koopa_items_free(MeleeKoopaItems*);
#endif
