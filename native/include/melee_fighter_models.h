#ifndef MELEE_NATIVE_FIGHTER_MODELS_H
#define MELEE_NATIVE_FIGHTER_MODELS_H
#include "melee_scene.h"
typedef struct MeleeFighterModels MeleeFighterModels;
/* PlCo entry 8's animated respawn platform, entry 16's trophy platform,
 * and entry 20's shared material model. Requires initialized HSD pools. */
MeleeFighterModels* melee_fighter_models_decode(const MeleeArchive*);
HSD_Joint* melee_fighter_models_joint(MeleeFighterModels*, unsigned index);
HSD_AnimJoint* melee_fighter_models_respawn_animation(MeleeFighterModels*);
void melee_fighter_models_free(MeleeFighterModels*);
#endif
