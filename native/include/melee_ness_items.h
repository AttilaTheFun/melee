#ifndef MELEE_NATIVE_NESS_ITEMS_H
#define MELEE_NATIVE_NESS_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeNessItems MeleeNessItems;
/* Eleven article slots consumed by Ness OnLoad. Requires HSD pools. */
MeleeNessItems* melee_ness_items_decode(const MeleeArchive*,u32 ft_data);
void** melee_ness_items_entries(MeleeNessItems*);
void melee_ness_items_free(MeleeNessItems*);
#endif
