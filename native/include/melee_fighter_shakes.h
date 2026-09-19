#ifndef MELEE_NATIVE_FIGHTER_SHAKES_H
#define MELEE_NATIVE_FIGHTER_SHAKES_H
#include "melee_archive.h"
#include <melee/ft/fighter.h>

typedef struct MeleeFighterShakes MeleeFighterShakes;
MeleeFighterShakes* melee_fighter_shakes_decode(const MeleeArchive*);
/* Three damage tables followed by grab mash and smash charge tables. */
struct Fighter_ShakeTable_t* melee_fighter_shakes_tables(MeleeFighterShakes*);
void melee_fighter_shakes_free(MeleeFighterShakes*);
#endif
