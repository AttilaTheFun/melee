#ifndef MELEE_NATIVE_CHARACTER_SOUNDS_H
#define MELEE_NATIVE_CHARACTER_SOUNDS_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeCharacterSounds MeleeCharacterSounds;
MeleeCharacterSounds* melee_character_sounds_decode(const MeleeArchive*,u32 ft_data);
FtSFX* melee_character_sounds_header(MeleeCharacterSounds*);
void melee_character_sounds_free(MeleeCharacterSounds*);
#endif
