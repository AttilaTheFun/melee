#ifndef MELEE_NATIVE_STAGE_MODELS_H
#define MELEE_NATIVE_STAGE_MODELS_H
#include "melee_archive.h"
#include <melee/gr/types.h>
typedef struct MeleeStageModels MeleeStageModels;
/* Lazy map_head model records. Borrowed descriptors require the owner to live
 * longer than every HSD object loaded from them. */
MeleeStageModels* melee_stage_models_decode(const MeleeArchive*);
unsigned melee_stage_models_count(const MeleeStageModels*);
/* For a stage-verified dormant slot, before any model/header lookup. */
MeleeHostBool melee_stage_models_omit_unused_joint_animation(MeleeStageModels*,unsigned);
struct UnkStageDat_x8_t* melee_stage_models_get(MeleeStageModels*,unsigned);
/* Decode joint-to-stage-point mappings, preserving model descriptor identity.
 * Duplicate model roots get a mapping for each native descriptor instance. */
MeleeHostBool melee_stage_models_point_maps(MeleeStageModels*,GroundJointMapEntry**,unsigned*);
MeleeHostBool melee_stage_models_light_overrides(MeleeStageModels*,LightOverrideEntry**,unsigned*);
/* Assemble contiguous native map_head records with owned splines and shared
 * shadow animation descriptors. Material overrides are already applied. */
UnkStageDat* melee_stage_models_header(MeleeStageModels*);
HSD_LightDesc* melee_stage_models_find_light(MeleeStageModels*,uint32_t archive_offset);
HSD_LightAnim* melee_stage_models_find_light_animation(MeleeStageModels*,uint32_t archive_offset);
HSD_ImageDesc* melee_stage_models_find_image(MeleeStageModels*,const char*);
void melee_stage_models_free(MeleeStageModels*);
#endif
