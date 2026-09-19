#ifndef MELEE_NATIVE_FIGHTER_MODIFIERS_H
#define MELEE_NATIVE_FIGHTER_MODIFIERS_H
#include "melee_archive.h"
#include <melee/ft/fighter.h>

typedef struct {
    float throws[26][3];
    float swing[6][5];
    float staling[9];
    struct Fighter_804D6524_t scale;
    struct Fighter_804D6520_t bunny;
    struct Fighter_804D651C_t metal;
    struct Fighter_804D6518_t gravity;
} MeleeFighterModifiers;
/* Transactional scalar conversion of PlCo entries 1–3 and 12–15. */
MeleeHostBool melee_fighter_modifiers_decode(const MeleeArchive*, MeleeFighterModifiers*);
#endif
