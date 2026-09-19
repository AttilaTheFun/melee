#include "melee_ax_depop.h"
#include <dolphin/os.h>
#include <string.h>

static void add(s32* sum, s16 sample)
{
    u32 bits = (u32) *sum + (u32) (s32) sample;
    memcpy(sum, &bits, sizeof(bits));
}

bool melee_ax_depop_add(MeleeAXDepop* state, const AXPBDPOP* last)
{
    if (!state || !last) return false;
    int old = OSDisableInterrupts();
    add(&state->main[0], last->aL);
    add(&state->main[1], last->aR);
    add(&state->main[2], last->aS);
    add(&state->auxA[0], last->aAuxAL);
    add(&state->auxA[1], last->aAuxAR);
    add(&state->auxA[2], last->aAuxAS);
    add(&state->auxB[0], last->aAuxBL);
    add(&state->auxB[1], last->aAuxBR);
    add(&state->auxB[2], last->aAuxBS);
    OSRestoreInterrupts(old);
    return true;
}

static void fade(s32* pending, s32 samples[MELEE_AX_FRAME_SAMPLES])
{
    /* Original AXSPB __AXDepopFade: signed division truncates toward zero;
     * discard sub-frame remainders and cap each sample's change at 20. */
    s32 step = *pending / MELEE_AX_FRAME_SAMPLES;
    if (!step) {
        *pending = 0;
        memset(samples, 0, MELEE_AX_FRAME_SAMPLES * sizeof(s32));
        return;
    }
    if (step > 20) step = 20;
    if (step < -20) step = -20;
    s32 value = *pending;
    for (unsigned i = 0; i < MELEE_AX_FRAME_SAMPLES; ++i) {
        samples[i] = value;
        value -= step;
    }
    *pending = value;
}

bool melee_ax_depop_begin_frame(MeleeAXDepop* state, MeleeAXMixBuffers* buffers)
{
    if (!state || !buffers) return false;
    int old = OSDisableInterrupts();
    for (unsigned channel = 0; channel < 3; ++channel) {
        fade(&state->main[channel], buffers->main[channel]);
        fade(&state->auxA[channel], buffers->auxA[channel]);
        fade(&state->auxB[channel], buffers->auxB[channel]);
    }
    OSRestoreInterrupts(old);
    return true;
}
