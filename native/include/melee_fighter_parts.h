#ifndef MELEE_NATIVE_FIGHTER_PARTS_H
#define MELEE_NATIVE_FIGHTER_PARTS_H
#include "melee_archive.h"
#include <melee/ft/fighter.h>

/* Retail PlCo includes a shared Egg Lay animation skeleton at index 33. */
enum { MELEE_FIGHTER_PART_TABLE_COUNT = 34 };
typedef struct MeleeFighterParts MeleeFighterParts;
MeleeFighterParts* melee_fighter_parts_decode(const MeleeArchive*);
FighterPartsTable** melee_fighter_parts_tables(MeleeFighterParts*);
struct Fighter_804D6540_t** melee_fighter_parts_accessories(MeleeFighterParts*);
void melee_fighter_parts_free(MeleeFighterParts*);
#endif
