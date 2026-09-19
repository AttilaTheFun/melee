#ifndef MELEE_NATIVE_CHARACTER_SELECT_H
#define MELEE_NATIVE_CHARACTER_SELECT_H
#include "melee_archive.h"
#include <sysdolphin/baselib/archive.h>
#include <sysdolphin/baselib/forward.h>
typedef struct {
    HSD_CObjDesc* cam;
    HSD_LightDesc* light0;
    HSD_LightDesc* light1;
    HSD_FogDesc* fog;
} MeleeCSSModels;
typedef struct {
    HSD_Joint* joint;
    HSD_AnimJoint* anim;
    HSD_MatAnimJoint* matanim;
    HSD_ShapeAnimJoint* shapeanim;
} MeleeCSSAnimation;
typedef struct {
    MeleeCSSModels models;
    MeleeCSSAnimation animations[9];
} MeleeCSSData;
typedef struct {
    MeleeCSSModels models;
    MeleeCSSAnimation animations[12];
} MeleeStageSelectionData;
HSD_Archive* melee_stage_selection_decode(const MeleeArchive*);
/* MnSlChr native descriptors; destroy only after borrowing HSD objects. */
HSD_Archive* melee_character_select_decode(const MeleeArchive*);
#endif
