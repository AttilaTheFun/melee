#ifndef MELEE_AX_POOL_MIX_H
#define MELEE_AX_POOL_MIX_H
#include "melee_ax_mix.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Render the initialized 64-voice pool into fresh frame buses. Retired voices
 * contribute their previous rendered samples to depop, including freed slots.
 * Success consumes depop/sync flags and scheduled updates. Failure changes
 * neither voices, pending fades nor output. Pool reset also resets the render
 * history. This does not run auxiliary effects, frame callbacks or a device. */
bool melee_ax_voice_pool_render(MeleeAXMixBuffers* buffers);
/* Apply the original AXCL surround-history seed after depop initialization,
 * before voices. Modes 0/1 require 160 previous surround samples; modes 2/3
 * keep the depop buses. The raw renderer above has no history seed. */
bool melee_ax_voice_pool_render_output(MeleeAXMixBuffers* buffers,
                                        const s32* previous_surround, unsigned mode);
#ifdef __cplusplus
}
#endif
#endif
