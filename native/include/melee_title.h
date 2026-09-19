#ifndef MELEE_NATIVE_TITLE_H
#define MELEE_NATIVE_TITLE_H
#include "melee_archive.h"
#include <melee/sc/types.h>
#include <sysdolphin/baselib/fog.h>
#include <sysdolphin/baselib/sobjlib.h>
typedef struct MeleeTitle MeleeTitle;
typedef struct {
    StaticModelDesc models[2];
    HSD_CObjDesc* camera;
    LightList** lights;
    HSD_FogDesc fog;
    HSD_SObjDesc mark;
} MeleeTitleData;
/* Requires initialized HSD pools. Owns every descriptor and pixel referenced
 * by the title exports. Remove borrowing HSD objects before freeing. */
MeleeTitle* melee_title_decode(const MeleeArchive* archive);
MeleeTitleData* melee_title_data(MeleeTitle* title);
void melee_title_free(MeleeTitle* title);
#endif
