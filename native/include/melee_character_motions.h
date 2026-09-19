#ifndef MELEE_NATIVE_CHARACTER_MOTIONS_H
#define MELEE_NATIVE_CHARACTER_MOTIONS_H
#include "melee_animation.h"
typedef struct MeleeCharacterMotions MeleeCharacterMotions;
typedef struct {
    const char* name;
    u32 offset,size,flags,script_offset;
} MeleeCharacterMotion;
/* Owns the index, combined AJ bytes and native action-script arena. */
MeleeCharacterMotions* melee_character_motions_decode(const MeleeArchive*,u32 ft_data,
    unsigned count,const void* aj,size_t aj_size);
/* Preserve table indices while loading only the range belonging to one demo
 * archive. Unloaded entries have no animation or script. */
MeleeCharacterMotions* melee_character_motions_decode_range(const MeleeArchive*,u32 table_slot,
    unsigned total,unsigned first,unsigned loaded,const void* bytes,size_t size);
const MeleeCharacterMotion* melee_character_motions_entry(MeleeCharacterMotions*,unsigned index);
FigaTree* melee_character_motions_tree(MeleeCharacterMotions*,unsigned index);
union CmdUnion;
union CmdUnion* melee_character_motions_script(MeleeCharacterMotions*,unsigned index);
struct Fighter_WaitAnimData;
struct Fighter_WaitAnimData* melee_character_motions_records(MeleeCharacterMotions*);
void melee_character_motions_free(MeleeCharacterMotions*);
#endif
