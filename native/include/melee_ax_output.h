#ifndef MELEE_AX_OUTPUT_H
#define MELEE_AX_OUTPUT_H
#include "melee_ax_mix.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One output owner per voice pool. Zero-initialize before first use. History
 * is invalidated automatically by voice-pool reset. */
typedef struct MeleeAXOutput {
    u64 generation;
    s32 surround[MELEE_AX_FRAME_SAMPLES];
} MeleeAXOutput;
/* Render a complete 5 ms frame at 32 kHz through pool/depop and aux effects.
 * Both pool and aux must already be initialized. Uses current AX mode 0..3.
 * Output is 160 host-endian signed-16 left/right pairs (not the console DMA
 * right/left ordering). Failure leaves PCM/history/pool unchanged and invokes
 * no effect callbacks. Reentrant rendering is rejected. No device or clock
 * is started. The registered AX user callback runs once after each successful
 * frame's auxiliary processing and prepares parameters for the next frame. */
bool melee_ax_output_frame(MeleeAXOutput* state, s16* stereo);
#ifdef __cplusplus
}
#endif
#endif
