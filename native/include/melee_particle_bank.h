#ifndef MELEE_NATIVE_PARTICLE_BANK_H
#define MELEE_NATIVE_PARTICLE_BANK_H
#include "melee_archive.h"
#include <sysdolphin/baselib/psstructs.h>
typedef struct MeleeParticleBank MeleeParticleBank;
/* Embedded command/texture banks use offsets relative to their own starts.
 * Owns descriptors and bytecode/texture payloads. No borrowed source bytes. */
MeleeParticleBank* melee_particle_bank_decode(const MeleeArchive*,uint32_t commands,size_t command_bytes,uint32_t textures,size_t texture_bytes);
/* Must be checked before passing a palette to GX. NULL remains a source NULL;
 * unresolved nonzero offsets are marked without constructing invalid pointers. */
int melee_particle_bank_palette_resolved(const void*);
void melee_particle_bank_free(MeleeParticleBank*);
unsigned melee_particle_bank_command_count(const MeleeParticleBank*);
unsigned melee_particle_bank_texture_count(const MeleeParticleBank*);
HSD_PSCmdList** melee_particle_bank_commands(MeleeParticleBank*);
HSD_PSTexGroup** melee_particle_bank_textures(MeleeParticleBank*);
#endif
