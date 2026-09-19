#ifndef MELEE_NATIVE_CHARACTER_COLLISION_H
#define MELEE_NATIVE_CHARACTER_COLLISION_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeCharacterCollision MeleeCharacterCollision;
MeleeCharacterCollision* melee_character_collision_decode(const MeleeArchive*,u32 ft_data,unsigned joints);
/* Assigns entries 0x30–0x40 only; borrowed until the owner is freed. */
void melee_character_collision_bind(MeleeCharacterCollision*,ftData*);
void melee_character_collision_free(MeleeCharacterCollision*);
#endif
