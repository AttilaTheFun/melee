#ifndef MELEE_NATIVE_SCENE_DESC_H
#define MELEE_NATIVE_SCENE_DESC_H
#include "melee_archive.h"
#include <sysdolphin/baselib/archive.h>
#include <melee/sc/forward.h>
typedef struct MeleeSceneDesc MeleeSceneDesc;
/* Converts scene models with animation variant tables, cameras and lights.
 * Requires HSD heap/pools. Camera, light and fog tracks retain owned data;
 * unsupported constraints and animation paths are rejected.
 * Remove all HSD objects borrowing these descriptors before freeing owner. */
MeleeSceneDesc* melee_scene_desc_decode(const MeleeArchive* archive,uint32_t root);
SceneDesc* melee_scene_desc_data(MeleeSceneDesc* scene);
void melee_scene_desc_free(MeleeSceneDesc* scene);
HSD_Archive* melee_single_scene_decode(const MeleeArchive*,const char*);
HSD_Archive* melee_approach_decode(const MeleeArchive*);
HSD_Archive* melee_cutscene_decode(const MeleeArchive*);
HSD_Archive* melee_demo_wait_decode(const MeleeArchive*);
HSD_Archive* melee_demo_result_decode(const MeleeArchive*);
HSD_Archive* melee_adventure_intro_decode(const MeleeArchive*);
HSD_Archive* melee_staffroll_decode(const MeleeArchive*);
HSD_Archive* melee_ending_decode(const MeleeArchive*);
HSD_Archive* melee_intro_decode(const MeleeArchive* archive);
HSD_Archive* melee_homerun_hud_decode(const MeleeArchive*);
HSD_Archive* melee_training_decode(const MeleeArchive*);
const void* melee_intro_motion_bytes(HSD_Archive*,const char*,size_t*);
#endif
