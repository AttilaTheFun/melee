#ifndef MELEE_NATIVE_DYNAMIC_MODEL_H
#define MELEE_NATIVE_DYNAMIC_MODEL_H
#include "melee_archive.h"
#include <melee/sc/types.h>
typedef struct MeleeDynamicModel MeleeDynamicModel;
/* Owned DynamicModelDesc, including null-terminated animation variant tables.
 * Borrowing HSD instances must be removed before this owner is freed. */
MeleeDynamicModel* melee_dynamic_model_decode(const MeleeArchive*,const char* symbol);
MeleeDynamicModel* melee_dynamic_model_decode_at(const MeleeArchive*,u32 root);
DynamicModelDesc* melee_dynamic_model_descriptor(MeleeDynamicModel*);
unsigned melee_dynamic_model_animation_count(const MeleeDynamicModel*,unsigned kind);
void melee_dynamic_model_free(MeleeDynamicModel*);
#endif
