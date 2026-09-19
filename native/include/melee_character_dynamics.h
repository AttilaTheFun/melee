#ifndef MELEE_NATIVE_CHARACTER_DYNAMICS_H
#define MELEE_NATIVE_CHARACTER_DYNAMICS_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeCharacterDynamics MeleeCharacterDynamics;
/* Owns main/demo selector pairs, dynamic descriptors and integer chain cutoffs. */
MeleeCharacterDynamics* melee_character_dynamics_decode(const MeleeArchive*,u32 ft_data,unsigned joints,unsigned main_count,unsigned demo_count);
/* Own extra costume descriptors while retaining the normal active count. */
MeleeCharacterDynamics* melee_character_dynamics_decode_extra(const MeleeArchive*,u32 ft_data,unsigned joints,unsigned main_count,unsigned demo_count,unsigned extra);
void melee_character_dynamics_bind(MeleeCharacterDynamics*,ftData*);
void melee_character_dynamics_free(MeleeCharacterDynamics*);
#endif
