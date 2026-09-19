#ifndef MELEE_NATIVE_CHARACTER_PARTS_H
#define MELEE_NATIVE_CHARACTER_PARTS_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeCharacterParts MeleeCharacterParts;
/* Owns visibility choices, costume texture selectors and five bone IDs.
 * Drawable indexes are checked against the engine maximum; the installed
 * costume still needs validation against its actual drawable/bone lists. */
MeleeCharacterParts* melee_character_parts_decode(const MeleeArchive*,u32 ft_data,unsigned costumes,unsigned joints);
void melee_character_parts_bind(MeleeCharacterParts*,ftData*);
void melee_character_parts_free(MeleeCharacterParts*);
/* Decode a standalone visibility descriptor (e.g. a costume hat). */
MeleeCharacterParts* melee_character_visibility_decode(const MeleeArchive*,u32 descriptor,unsigned costumes);
FtPartsDesc* melee_character_visibility_descriptor(MeleeCharacterParts*);
#endif
