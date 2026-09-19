#ifndef MELEE_NATIVE_ITEM_HURTBONES_H
#define MELEE_NATIVE_ITEM_HURTBONES_H
#include "melee_archive.h"
#include <melee/it/types.h>
/* bone_count is the model's dynamic bone-table count; zero still permits the
 * root sentinel bone 0. The returned list owns all descriptor storage. */
ItHurtBoneList* melee_item_hurtbones_decode(const MeleeArchive*,uint32_t offset,unsigned bone_count);
void melee_item_hurtbones_free(ItHurtBoneList*);
#endif
