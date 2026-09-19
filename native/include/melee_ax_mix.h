#ifndef MELEE_AX_MIX_H
#define MELEE_AX_MIX_H
#include <dolphin/ax.h>
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { MELEE_AX_FRAME_SAMPLES = 160, MELEE_AX_MS_SAMPLES = 32 };
/* Each triplet is left, right, surround. Callers clear these accumulators at
 * frame start; voices add to them, preserving previous voices' contributions. */
typedef struct MeleeAXMixBuffers {
    s32 main[3][MELEE_AX_FRAME_SAMPLES];
    s32 auxA[3][MELEE_AX_FRAME_SAMPLES];
    s32 auxB[3][MELEE_AX_FRAME_SAMPLES];
} MeleeAXMixBuffers;
/* Process one 1 ms quantum into offset millisecond*32 (millisecond 0..4).
 * Applies source conversion, signed GameCube envelope, channel gain/ramping
 * and per-channel last-sample depop values. Failure changes neither PB nor
 * buses. Inactive voices are a successful no-op. This stateless entry point
 * rejects ITD; use the frame variant with history below. FIR and DPL2
 * processing remain unsupported and fail explicitly.
 * Caller applies scheduled PB updates before each quantum. No effects,
 * depop-tail synthesis, voice ownership callbacks or device output here. */
bool melee_ax_mix_voice_ms(AXPB* parameters, MeleeAXMixBuffers* buffers,
                           unsigned millisecond);
/* Process all five quanta, applying PB word-offset/value pairs before each.
 * parameters->update.updNum selects pairs for each millisecond. updates is a
 * full native pointer, never the PB's console split address. word_count is
 * even and at most 128. Failure rolls back the entire frame, including buses.
 * Does not clear the caller-owned update schedule or run voice callbacks. */
bool melee_ax_mix_voice_frame(AXPB* parameters, const u16* updates,
                              size_t word_count, MeleeAXMixBuffers* buffers);
/* Stateful version supporting ITD. The complete frame, including history,
 * commits only on success. A null history retains the stateless behavior. */
bool melee_ax_mix_voice_frame_itd(AXPB* parameters, const u16* updates,
                                  size_t word_count, MeleeAXMixBuffers* buffers,
                                  AXPBITDBUFFER* history);
#ifdef __cplusplus
}
#endif
#endif
