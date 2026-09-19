#ifndef MELEE_FIGHTER_SELECTION_PROBE_H
#define MELEE_FIGHTER_SELECTION_PROBE_H
#include "melee_scene.h"
typedef struct FighterSelectionProbe FighterSelectionProbe;
FighterSelectionProbe* fighter_selection_probe_create(MeleeScene*,const char* data,const char* symbol,const char* motion);
MeleeHostBool fighter_selection_probe_step(FighterSelectionProbe*,float frame,float delta);
void fighter_selection_probe_free(FighterSelectionProbe*);
#endif
