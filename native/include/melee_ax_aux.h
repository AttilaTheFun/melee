#ifndef MELEE_AX_AUX_H
#define MELEE_AX_AUX_H
#include "melee_ax_mix.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Initialize/reset only while audio is stopped. Clears all three ring slots
 * and registered effects. Returns false if called recursively by an effect. */
bool melee_ax_aux_init(void);
/* Readiness preflight. Hold the interrupt gate across this check and the
 * subsequent process call if other mixer state will be committed first. */
bool melee_ax_aux_ready(void);
/* Transfer one mixed frame to the original AX auxiliary ring, accumulate its
 * delayed returns into main L/R/S, and process/rotate the CPU effect slot.
 * Preserves the SDK's two-frame latency and its aux-A gate for both transfers.
 * Returns false without effects or bus changes before init, for null input,
 * recursively, or in unsupported mode 4 (DPL2). Registered effects execute
 * under the native interrupt gate. This does not produce device output. */
bool melee_ax_aux_process_frame(MeleeAXMixBuffers* buffers);
#ifdef __cplusplus
}
#endif
#endif
