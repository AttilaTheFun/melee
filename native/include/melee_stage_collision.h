#ifndef MELEE_NATIVE_STAGE_COLLISION_H
#define MELEE_NATIVE_STAGE_COLLISION_H
#include "melee_archive.h"
#include <melee/mp/types.h>
/* Mutable owned data: original mpPruneEmptyLines edits the line records. */
MapCollData* melee_stage_collision_decode(const MeleeArchive*);
void melee_stage_collision_free(MapCollData*);
#endif
