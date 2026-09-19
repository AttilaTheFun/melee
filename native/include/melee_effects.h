#ifndef MELEE_NATIVE_EFFECTS_H
#define MELEE_NATIVE_EFFECTS_H
#include "melee_particle_bank.h"
#include <melee/ef/types.h>
typedef struct MeleeEffects MeleeEffects;
MeleeEffects* melee_effects_decode(const MeleeArchive*,const char* symbol);
unsigned melee_effects_count(const MeleeEffects*);
EF_EffectDesc* melee_effects_model(MeleeEffects*,unsigned index);
MeleeParticleBank* melee_effects_particles(MeleeEffects*);
void melee_effects_free(MeleeEffects*);
#endif
