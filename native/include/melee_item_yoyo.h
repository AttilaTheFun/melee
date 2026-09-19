#ifndef MELEE_NATIVE_ITEM_YOYO_H
#define MELEE_NATIVE_ITEM_YOYO_H
#include "melee_archive.h"
#include <melee/it/itYoyo.h>
typedef struct MeleeItemYoyo MeleeItemYoyo;
MeleeItemYoyo* melee_item_yoyo_decode(const MeleeArchive*,u32);
itYoyoAttributes* melee_item_yoyo_attributes(MeleeItemYoyo*);
void melee_item_yoyo_free(MeleeItemYoyo*);
#endif
