#ifndef MELEE_NATIVE_LIGHTS_H
#define MELEE_NATIVE_LIGHTS_H
#include "melee_archive.h"
#include <sysdolphin/baselib/forward.h>
typedef struct MeleeLights MeleeLights;
/* Owns a converted light descriptor chain and all referenced scalar/world data.
 * Custom classes, constraints and cyclic chains are rejected. */
MeleeLights* melee_lights_decode(const MeleeArchive* archive, uint32_t root);
HSD_LightDesc* melee_lights_descriptor(MeleeLights* lights);
size_t melee_lights_count(const MeleeLights* lights);
HSD_LightDesc* melee_lights_find(MeleeLights*,uint32_t archive_offset);
void melee_lights_free(MeleeLights* lights);
#endif
