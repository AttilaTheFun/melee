#ifndef MELEE_AX_STREAM_H
#define MELEE_AX_STREAM_H
#include "melee_ax_output.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MeleeAXStream {
    MeleeAXOutput output;
    s16 pending[MELEE_AX_FRAME_SAMPLES * 2];
    u64 generation;
    unsigned position;
    bool initialized, pulling, failed;
} MeleeAXStream;
/* One stream owns the pool's output. Initialize/reset only while stopped.
 * Does not initialize voices, aux, callbacks or an audio device. */
bool melee_ax_stream_init(MeleeAXStream* stream);
/* Pull up to 4096 stereo frames at 32 kHz into interleaved host s16 L/R.
 * Retains unused PCM; changing pull sizes cannot change audio/callback order.
 * A render failure keeps an already-produced prefix, silences the remainder
 * and latches failure. Later reads produce silence without callbacks until
 * stream reset or pool reset. Invalid arguments/reentrancy change nothing.
 * Pool reset also discards buffered PCM from the previous generation. */
bool melee_ax_stream_read(MeleeAXStream* stream, s16* stereo, size_t frames);
#ifdef __cplusplus
}
#endif
#endif
