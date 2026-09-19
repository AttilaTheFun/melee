#ifndef MELEE_AX_VOICE_H
#define MELEE_AX_VOICE_H
#include <dolphin/ax.h>
#include <stdbool.h>
/* Initialize only while audio is stopped. This prepares voice ownership and
 * CPU parameter blocks; it does not initialize a mixer or audio device. */
void melee_ax_voice_pool_init(void);
bool melee_ax_voice_owned(const AXVPB* voice);
/* Mixer internals: caller holds the native interrupt gate while using these
 * pointers or observing the generation. Generation zero means uninitialized. */
AXVPB* melee_ax_voice_at(unsigned index);
AXPBITDBUFFER* melee_ax_voice_itd_at(unsigned index);
u64 melee_ax_voice_generation(void);
#endif
