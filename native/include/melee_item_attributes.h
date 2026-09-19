#ifndef MELEE_NATIVE_ITEM_ATTRIBUTES_H
#define MELEE_NATIVE_ITEM_ATTRIBUTES_H
#include "melee_archive.h"
#include <melee/it/types.h>
MeleeHostBool melee_item_attributes_decode(const MeleeArchive*,uint32_t offset,ItemAttr*);
#endif
