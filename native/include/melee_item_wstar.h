#ifndef MELEE_NATIVE_ITEM_WSTAR_H
#define MELEE_NATIVE_ITEM_WSTAR_H
#include "melee_archive.h"
#include <melee/it/itCommonItems.h>
typedef struct MeleeItemWstar MeleeItemWstar;
/* Owns scalar attributes and the variable animation/sound array. */
MeleeItemWstar* melee_item_wstar_decode(const MeleeArchive*,uint32_t attributes,uint32_t joint);
itWstarAttributes* melee_item_wstar_attributes(MeleeItemWstar*);
void melee_item_wstar_free(MeleeItemWstar*);
#endif
