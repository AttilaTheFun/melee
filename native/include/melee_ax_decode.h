#ifndef MELEE_AX_DECODE_H
#define MELEE_AX_DECODE_H
#include <dolphin/ax.h>
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Decode one source-rate sample from committed ARAM. Updates address/history
 * and loop/end state on success. Failure changes neither PB nor output.
 * No pitch conversion, volume envelope, mixing or audio-device output. */
bool melee_ax_decode_sample(AXPB* parameters, s16* sample);
/* Render up to 160 source-converted samples, preserving fractional phase and
 * four-sample history across calls. Four-tap mode requires the selected DSP
 * coefficient table (512 signed words per mode). No fallback is substituted.
 * Failure leaves the full PB and output unchanged; no envelope/mixing occurs. */
bool melee_ax_resample(AXPB* parameters, s16* samples, size_t count,
                      const s16* coefficients, size_t coefficient_count);
/* Native playback entry point with pinned, reconstructed Dolphin DSP tables.
 * The same transactional and block-size rules apply. */
bool melee_ax_resample_native(AXPB* parameters, s16* samples, size_t count);
#ifdef __cplusplus
}
#endif
#endif
