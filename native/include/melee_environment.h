#ifndef MELEE_NATIVE_ENVIRONMENT_H
#define MELEE_NATIVE_ENVIRONMENT_H
#include "melee_archive.h"
#include <melee/sc/types.h>
#include <sysdolphin/baselib/fog.h>
typedef struct MeleeEnvironment MeleeEnvironment;
/* Owned static camera, light lists with world-position/interest animation, and fog.
 * UINT32_MAX denotes an absent camera, lights or fog in the full decoder.
 * No source bytes survive. Remove borrowing HSD objects before freeing. */
MeleeEnvironment* melee_environment_decode(const MeleeArchive*,uint32_t camera,uint32_t lights,uint32_t fog);
/* Camera and fog only, for scenes with direct light descriptors. */
MeleeEnvironment* melee_environment_decode_camera_fog(const MeleeArchive*,uint32_t camera,uint32_t fog);
HSD_CObjDesc* melee_environment_camera(MeleeEnvironment*);
LightList** melee_environment_lights(MeleeEnvironment*);
HSD_FogDesc* melee_environment_fog(MeleeEnvironment*);
HSD_LightDesc* melee_environment_find_light(MeleeEnvironment*,uint32_t archive_offset);
HSD_LightAnim* melee_environment_find_light_animation(MeleeEnvironment*,uint32_t);
void melee_environment_free(MeleeEnvironment*);
#endif
