#ifndef MELEE_NATIVE_CHARACTER_AUX_H
#define MELEE_NATIVE_CHARACTER_AUX_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeCharacterAux MeleeCharacterAux;
MeleeCharacterAux* melee_character_aux_decode(const MeleeArchive*,u32 ft_data);
void melee_character_aux_bind(MeleeCharacterAux*,ftData*);
void melee_character_aux_free(MeleeCharacterAux*);
#endif
