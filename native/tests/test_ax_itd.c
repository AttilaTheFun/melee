#include "melee_ax_itd.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    s16 input[32], output[2][32]; AXPBITDBUFFER history;
    for (unsigned i = 0; i < 32; ++i) input[i] = 1000 + i;
    for (unsigned shift = 0; shift < 32; ++shift)
        for (unsigned target = 0; target < 32; ++target) {
            for (unsigned i = 0; i < 32; ++i) history.data[i] = -1000 + i;
            AXPBITD state = { .flag = 1, .shiftL = shift, .shiftR = 31 - shift,
                .targetShiftL = target, .targetShiftR = 31 - target };
            assert(melee_ax_itd_ms(&state, &history, input, output));
            for (unsigned c = 0; c < 2; ++c)
                for (unsigned i = 0; i < 32; ++i) {
                    int sample = i + (c ? 31 - shift : shift);
                    assert(output[c][i] == (sample < 32 ? sample - 1000 : sample - 32 + 1000));
                }
            int next = shift + (shift < target) - (shift > target);
            assert(state.shiftL == next && state.shiftR == 31 - next);
            assert(!memcmp(history.data, input, sizeof(input)));
        }
    memset(&history, 0, sizeof(history)); memset(input, 0, sizeof(input)); input[0] = 1234;
    AXPBITD state = { .flag = 1, .shiftR = 31, .targetShiftR = 31 };
    assert(melee_ax_itd_ms(&state, &history, input, output));
    for (unsigned i = 0; i < 32; ++i) {
        assert(!output[0][i]); assert(output[1][i] == (i == 1 ? 1234 : 0));
    }
    memset(input, 0, sizeof(input));
    assert(melee_ax_itd_ms(&state, &history, input, output));
    assert(output[0][0] == 1234);
    for (unsigned i = 1; i < 32; ++i) assert(!output[0][i]);
    for (unsigned i = 0; i < 32; ++i) assert(!output[1][i]);
    state.flag = 0; history.data[0] = 4567;
    assert(melee_ax_itd_ms(&state, &history, input, output));
    assert(history.data[0] == 4567 && output[0][0] == 0 && output[1][0] == 0);
    for (unsigned bad = 0; bad < 5; ++bad) {
        AXPBITD invalid = { .flag = 1 };
        if (bad == 0) invalid.flag = 2;
        if (bad == 1) invalid.shiftL = 32;
        if (bad == 2) invalid.shiftR = 65535;
        if (bad == 3) invalid.targetShiftL = 32;
        if (bad == 4) invalid.targetShiftR = 65535;
        AXPBITD saved = invalid; AXPBITDBUFFER saved_history = history;
        memset(output, 0xA7, sizeof(output)); s16 saved_output[2][32];
        memcpy(saved_output, output, sizeof(output));
        assert(!melee_ax_itd_ms(&invalid, &history, input, output));
        assert(!memcmp(&invalid, &saved, sizeof(saved)));
        assert(!memcmp(&history, &saved_history, sizeof(history)));
        assert(!memcmp(output, saved_output, sizeof(output)));
    }
    puts("AX ITD quantum: all shifts/targets, post-mix interpolation, 1/32-sample delays, history and rejection passed");
}
