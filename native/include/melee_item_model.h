#ifndef MELEE_NATIVE_ITEM_MODEL_H
#define MELEE_NATIVE_ITEM_MODEL_H
#include "melee_archive.h"
#include <melee/it/types.h>
typedef struct MeleeItemModel MeleeItemModel;
MeleeItemModel* melee_item_model_decode(const MeleeArchive*,uint32_t offset);
ItemModelDesc* melee_item_model_descriptor(MeleeItemModel*);
void melee_item_model_free(MeleeItemModel*);
#endif
