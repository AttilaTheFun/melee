#ifndef MELEE_NATIVE_ITEM_FOODS_H
#define MELEE_NATIVE_ITEM_FOODS_H
#include "melee_archive.h"
#include <melee/it/itCommonItems.h>
typedef struct MeleeItemFoods MeleeItemFoods;
/* Owns all food model descriptors and scalar records. Requires HSD pools. */
MeleeItemFoods* melee_item_foods_decode(const MeleeArchive*,uint32_t offset);
itFoodsNativeAttributes* melee_item_foods_attributes(MeleeItemFoods*);
void melee_item_foods_free(MeleeItemFoods*);
#endif
