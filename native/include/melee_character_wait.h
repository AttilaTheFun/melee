#ifndef MELEE_NATIVE_CHARACTER_WAIT_H
#define MELEE_NATIVE_CHARACTER_WAIT_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeCharacterWait MeleeCharacterWait;
MeleeCharacterWait* melee_character_wait_decode(const MeleeArchive*,u32 ft_data,unsigned animation_count);
void melee_character_wait_bind(MeleeCharacterWait*,ftData*);
void melee_character_wait_free(MeleeCharacterWait*);
#endif
