#ifndef MELEE_NATIVE_CHARACTER_PART_ANIMS_H
#define MELEE_NATIVE_CHARACTER_PART_ANIMS_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeCharacterPartAnims MeleeCharacterPartAnims;
/* Explicit table sizes are required: the archive has no count fields for
 * these pointer arrays. Owns SRT animation trees and copied track streams.
 * External references become null, matching lbArchive_InitializeDAT. */
MeleeCharacterPartAnims* melee_character_part_anims_decode(const MeleeArchive*,u32 ft_data,unsigned joints,const unsigned* animation_counts,unsigned groups);
void melee_character_part_anims_bind(MeleeCharacterPartAnims*,ftData*);
void melee_character_part_anims_free(MeleeCharacterPartAnims*);
#endif
