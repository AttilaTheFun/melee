#ifndef MELEE_NATIVE_ITEM_MUSHROOM_H
#define MELEE_NATIVE_ITEM_MUSHROOM_H
#include "melee_archive.h"
#include <melee/it/kinds/itkinoko.h>
typedef struct MeleeItemMushroom MeleeItemMushroom;
MeleeItemMushroom* melee_item_mushroom_decode(const MeleeArchive*,uint32_t attributes,uint32_t joint);
KinokoAttrs* melee_item_mushroom_attributes(MeleeItemMushroom*);
void melee_item_mushroom_free(MeleeItemMushroom*);
#endif
