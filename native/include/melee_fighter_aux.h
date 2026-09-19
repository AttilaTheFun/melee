#ifndef MELEE_NATIVE_FIGHTER_AUX_H
#define MELEE_NATIVE_FIGHTER_AUX_H
#include "melee_archive.h"
#include <melee/sfx/crowdsfx.h>
typedef struct {
    u8 colors[3][20]; /* PlCo entries 17–19: five RGBA byte records each. */
    CrowdConfig crowd;
} MeleeFighterAux;
MeleeHostBool melee_fighter_aux_decode(const MeleeArchive*, MeleeFighterAux*);
#endif
