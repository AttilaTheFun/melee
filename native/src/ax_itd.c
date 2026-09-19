/* SPDX-License-Identifier: GPL-2.0-or-later
 * AX ITD behavior traced in the repository's DSPCode.c, matching Dolphin's
 * historical DSP_UC_6A696CE7 analysis. See native/README.md for provenance. */
#include "melee_ax_itd.h"
#include <string.h>

static u16 approach(u16 current, u16 target)
{ return current < target ? current + 1 : current > target ? current - 1 : current; }

bool melee_ax_itd_ms(AXPBITD* state, AXPBITDBUFFER* history,
                     const s16 input[32], s16 output[2][32])
{
    if (!state || !history || !input || !output || state->flag > 1) return false;
    if (state->flag && (state->shiftL > 31 || state->shiftR > 31 ||
                       state->targetShiftL > 31 || state->targetShiftR > 31))
        return false;
    s16 window[64];
    memcpy(window, history->data, 32 * sizeof(s16));
    memcpy(window + 32, input, 32 * sizeof(s16));
    unsigned left = state->flag ? state->shiftL : 32;
    unsigned right = state->flag ? state->shiftR : 32;
    memcpy(output[0], window + left, 32 * sizeof(s16));
    memcpy(output[1], window + right, 32 * sizeof(s16));
    if (state->flag) {
        memcpy(history->data, window + 32, 32 * sizeof(s16));
        state->shiftL = approach(state->shiftL, state->targetShiftL);
        state->shiftR = approach(state->shiftR, state->targetShiftR);
    }
    return true;
}
