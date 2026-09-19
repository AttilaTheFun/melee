#ifndef MELEE_NATIVE_FIGHTER_COMMON_H
#define MELEE_NATIVE_FIGHTER_COMMON_H
#include "melee_archive.h"
#include <melee/ft/types.h>

/* Decode the scalar parameter block only; not the remaining PlCo tables. */
MeleeHostBool melee_fighter_common_decode(const MeleeArchive*, ftCommonData*);
#endif
