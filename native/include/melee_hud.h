#ifndef MELEE_NATIVE_HUD_H
#define MELEE_NATIVE_HUD_H
#include "melee_archive.h"
#include <sysdolphin/baselib/archive.h>
/* Owns all public HUD model tables and the damage placement scene.
 * Requires HSD pools. Remove borrowing objects before native_destroy. */
HSD_Archive* melee_hud_decode(const MeleeArchive*);
struct IfDamageState;
/* Original death animation operating on the native damage-state layout. */
void melee_hud_death_animation(struct IfDamageState*);
#endif
