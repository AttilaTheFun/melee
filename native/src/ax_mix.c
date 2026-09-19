/* SPDX-License-Identifier: GPL-2.0-or-later
 * Native AX processing, informed by Dolphin's AXVoice/AX mixer behavior.
 * See the pinned reference and licensing notice in native/README.md. */
#include "melee_ax_mix.h"
#include "melee_ax_decode.h"
#include "melee_ax_itd.h"
#include <dolphin/os.h>
#include <stdint.h>
#include <string.h>

static s32 signed_volume(u16 value)
{
    return value < 0x8000 ? (s32) value : (s32) value - 0x10000;
}

static s16 gain_sample(s16 sample, s32 gain)
{
    s64 product = (s64) sample * gain;
    /* Define arithmetic shift even on hosts whose signed shift differs. */
    s64 scaled = product >= 0 ? product / 32768 : -((-product + 32767) / 32768);
    return scaled < -32768 ? -32768 : scaled > 32767 ? 32767 : (s16) scaled;
}

static void mix_channel(s32* bus, const s16* input, u16* volume, u16 delta,
                        s16* last)
{
    for (unsigned i = 0; i < MELEE_AX_MS_SAMPLES; ++i) {
        s16 sample = gain_sample(input[i], *volume);
        /* Accumulation is a 32-bit wrap, not signed C overflow. */
        u32 sum = (u32) bus[i] + (u32) (s32) sample;
        memcpy(&bus[i], &sum, sizeof(sum));
        *volume = (u16) (*volume + delta);
        *last = sample;
    }
}

static bool mix_voice_ms(AXPB* parameters, MeleeAXMixBuffers* buffers,
                          unsigned millisecond, AXPBITDBUFFER* history)
{
    if (!parameters || !buffers || millisecond >= 5) return false;
    int old = OSDisableInterrupts();
    AXPB next = *parameters;
    bool success = false;
    if (!next.state) { success = true; goto finish; }
    if (next.state != 1 || next.fir.numCoefs || (next.itd.flag && !history) ||
        (next.mixerCtrl & ~0xF)) goto finish;
    s16 samples[MELEE_AX_MS_SAMPLES];
    if (!melee_ax_resample_native(&next, samples, MELEE_AX_MS_SAMPLES)) goto finish;
    for (unsigned i = 0; i < MELEE_AX_MS_SAMPLES; ++i) {
        samples[i] = gain_sample(samples[i], signed_volume(next.ve.currentVolume));
        next.ve.currentVolume = (u16) (next.ve.currentVolume + next.ve.currentDelta);
    }
    s16 delayed[2][MELEE_AX_MS_SAMPLES];
    const s16* left = samples;
    const s16* right = samples;
    if (next.itd.flag) {
        if (!melee_ax_itd_ms(&next.itd, history, samples, delayed)) goto finish;
        left = delayed[0]; right = delayed[1];
    }
    unsigned offset = millisecond * MELEE_AX_MS_SAMPLES;
    bool ramp = (next.mixerCtrl & 8) != 0;
#define MIX(BUS, CHANNEL, INPUT, VOLUME, DELTA, LAST) \
    mix_channel(buffers->BUS[CHANNEL] + offset, INPUT, &next.mix.VOLUME, \
                ramp ? next.mix.DELTA : 0, &next.dpop.LAST)
    MIX(main, 0, left, vL, vDeltaL, aL);
    MIX(main, 1, right, vR, vDeltaR, aR);
    if (next.mixerCtrl & 4) MIX(main, 2, samples, vS, vDeltaS, aS);
    if (next.mixerCtrl & 1) {
        MIX(auxA, 0, left, vAuxAL, vDeltaAuxAL, aAuxAL);
        MIX(auxA, 1, right, vAuxAR, vDeltaAuxAR, aAuxAR);
        if (next.mixerCtrl & 4) MIX(auxA, 2, samples, vAuxAS, vDeltaAuxAS, aAuxAS);
    }
    if (next.mixerCtrl & 2) {
        MIX(auxB, 0, left, vAuxBL, vDeltaAuxBL, aAuxBL);
        MIX(auxB, 1, right, vAuxBR, vDeltaAuxBR, aAuxBR);
        if (next.mixerCtrl & 4) MIX(auxB, 2, samples, vAuxBS, vDeltaAuxBS, aAuxBS);
    }
#undef MIX
    *parameters = next;
    success = true;
finish:
    OSRestoreInterrupts(old);
    return success;
}

bool melee_ax_mix_voice_ms(AXPB* parameters, MeleeAXMixBuffers* buffers,
                           unsigned millisecond)
{ return mix_voice_ms(parameters, buffers, millisecond, NULL); }

bool melee_ax_mix_voice_frame_itd(AXPB* parameters, const u16* updates,
                                  size_t word_count, MeleeAXMixBuffers* buffers,
                                  AXPBITDBUFFER* history)
{
    if (!parameters || !buffers || word_count > 128 || (word_count & 1) ||
        (word_count && !updates)) return false;
    int old = OSDisableInterrupts();
    AXPB next = *parameters;
    AXPBITDBUFFER next_history;
    if (history) next_history = *history;
    MeleeAXMixBuffers output = *buffers;
    u16 commands[128];
    if (word_count) memcpy(commands, updates, word_count * sizeof(u16));
    bool success = false;
    for (unsigned ms = 0; ms < 5; ++ms) {
        /* Counts are PB fields and may themselves be changed by a prior
         * quantum's updates. Re-read and bound each selection before use. */
        size_t first = 0;
        for (unsigned previous = 0; previous < ms; ++previous)
            first += next.update.updNum[previous];
        size_t count = next.update.updNum[ms];
        if (first > word_count / 2 || count > word_count / 2 - first) goto finish;
        for (size_t pair = first; pair < first + count; ++pair) {
            unsigned offset = commands[pair * 2];
            if (offset >= sizeof(AXPB) / sizeof(u16)) goto finish;
            /* PB storage consists only of 16-bit scalar fields. memcpy avoids
             * treating the struct as an aliased u16 array on the native ABI. */
            memcpy((unsigned char*) &next + offset * sizeof(u16),
                   &commands[pair * 2 + 1], sizeof(u16));
        }
        if (!mix_voice_ms(&next, &output, ms, history ? &next_history : NULL)) goto finish;
    }
    *parameters = next;
    *buffers = output;
    if (history) *history = next_history;
    success = true;
finish:
    OSRestoreInterrupts(old);
    return success;
}

bool melee_ax_mix_voice_frame(AXPB* parameters, const u16* updates,
                              size_t word_count, MeleeAXMixBuffers* buffers)
{ return melee_ax_mix_voice_frame_itd(parameters, updates, word_count, buffers, NULL); }
