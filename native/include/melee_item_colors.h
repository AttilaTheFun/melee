#ifndef MELEE_NATIVE_ITEM_COLORS_H
#define MELEE_NATIVE_ITEM_COLORS_H
#include "melee_archive.h"
#include <melee/lb/types.h>
typedef struct MeleeItemColors MeleeItemColors;
/* Explicit table count; owns reachable scripts and native metadata. */
MeleeItemColors* melee_item_colors_decode(const MeleeArchive*,uint32_t table,unsigned count);
/* Fighter color callbacks additionally support effect, sound and rumble commands. */
MeleeItemColors* melee_fighter_colors_decode(const MeleeArchive*,uint32_t table,unsigned count);
/* Stage parameters store script pointers without per-entry metadata. */
MeleeItemColors* melee_stage_colors_decode(const MeleeArchive*,uint32_t table,unsigned count);
struct Fighter_804D653C_t* melee_item_colors_entries(MeleeItemColors*);
void melee_item_colors_free(MeleeItemColors*);
#endif
