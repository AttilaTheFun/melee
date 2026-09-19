#ifndef MELEE_NATIVE_FIGHTER_COMMAND_H
#define MELEE_NATIVE_FIGHTER_COMMAND_H
#include "melee_archive.h"
#include <melee/lb/types.h>
/* Convert a supported single-word gameplay command. Multiword/control-flow
 * commands return false; output is unchanged on failure. This translates
 * representation only; runtime target indexes require consumer validation. */
MeleeHostBool melee_fighter_command_single(u32 word,union CmdUnion* out);
/* Exact serialized word count required; out must hold count CmdUnion slots.
 * Hitbox spawn remains unsupported. */
MeleeHostBool melee_fighter_command_decode(const u32* words,size_t count,union CmdUnion* out);
/* Hitbox creation additionally requires the following serialized word to
 * preserve the retail interpreter's lookahead. Exactly five payload words. */
MeleeHostBool melee_fighter_hitbox_decode(const u32* words,size_t count,u32 following_word,union CmdUnion* out);
#endif
