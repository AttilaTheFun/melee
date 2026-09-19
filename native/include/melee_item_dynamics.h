#ifndef MELEE_NATIVE_ITEM_DYNAMICS_H
#define MELEE_NATIVE_ITEM_DYNAMICS_H
#include "melee_archive.h"
#include <melee/it/types.h>
ItemDynamics* melee_item_dynamics_decode(const MeleeArchive*,uint32_t offset,unsigned bone_count);
/* Additional serialized descriptors used by costume-specific physics. */
ItemDynamics* melee_item_dynamics_decode_extra(const MeleeArchive*,uint32_t offset,unsigned bone_count,unsigned extra);
void melee_item_dynamics_free(ItemDynamics*);
#endif
