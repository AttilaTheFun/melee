/* AX source samples, decoded directly by the native CPU from virtual ARAM.
 * The normal AX formats are ADPCM=0, PCM16=10 and PCM8=25. */
#include "melee_ax_decode.h"
#include <stdio.h>
#include "melee_aram.h"
#include <dolphin/os.h>
#include <stdint.h>

static u32 address(u16 high, u16 low) { return (u32) high * 65536 + low; }
static s32 signed_word(u16 bits) { return bits < 32768 ? bits : (s32) bits - 65536; }
static s64 floor_shift(s64 value, unsigned shift)
{
    s64 divisor = (s64) 1 << shift;
    return value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor);
}
static s16 saturate(s64 value)
{ return value > 32767 ? 32767 : value < -32768 ? -32768 : (s16) value; }

static bool decode(AXPB* pb, s16* output)
{
    if (!pb->state) { *output = 0; return true; }
    if (pb->state != 1 || pb->type > 1) return false;
    u32 position = address(pb->addr.currentAddressHi, pb->addr.currentAddressLo);
    u32 end = address(pb->addr.endAddressHi, pb->addr.endAddressLo);
    u32 loop = address(pb->addr.loopAddressHi, pb->addr.loopAddressLo);
    u16 format = pb->addr.format;
    if (format != 0 && format != 10 && format != 25) return false;
    if (!format && ((position & 15) < 2 || (end & 15) < 2 || (pb->addr.loopFlag && (loop & 15) < 2)))
        return false; /* Disabled loop targets are not read by the DSP. */
    unsigned predictor = (pb->adpcm.pred_scale >> 4) & 7;
    s32 first = signed_word(pb->adpcm.a[predictor][0]);
    s32 second = signed_word(pb->adpcm.a[predictor][1]);
    s32 previous = signed_word(pb->adpcm.yn1);
    s32 older = signed_word(pb->adpcm.yn2);
    unsigned char bytes[2];
    s16 decoded;
    if (!format) {
        if (!melee_aram_read(position / 2, bytes, 1)) return false;
        unsigned nibble = position & 1 ? bytes[0] & 15 : bytes[0] >> 4;
        s32 residual = nibble < 8 ? (s32) nibble : (s32) nibble - 16;
        s64 prediction = (s64) first * previous + (s64) second * older + 1024;
        decoded = saturate(residual * (1 << (pb->adpcm.pred_scale & 15)) +
                           floor_shift(prediction, 11));
    } else {
        s32 raw;
        unsigned shift;
        if (format == 10) {
            if (position > UINT32_MAX / 2 ||
                !melee_aram_read(position * 2, bytes, 2)) return false;
            raw = signed_word((u16) bytes[0] * 256 + bytes[1]);
            shift = 11;
        } else {
            if (!melee_aram_read(position, bytes, 1)) return false;
            raw = bytes[0];
            shift = 0;
        }
        s64 value = floor_shift((s64) signed_word(pb->adpcm.gain) * raw, shift) +
                    floor_shift((s64) first * previous, shift) +
                    floor_shift((s64) second * older, shift);
        decoded = (s16) signed_word((u16) value); /* PCM wraps to 16 bits. */
    }
    pb->adpcm.yn2 = pb->adpcm.yn1;
    pb->adpcm.yn1 = (u16) decoded;
    pb->adpcm.pred_scale &= 0x7F;
    bool ended = position == end;
    ++position;
    if (!format && !(position & 15)) {
        if (!melee_aram_read(position / 2, bytes, 1)) return false;
        pb->adpcm.pred_scale = bytes[0] & 0x7F;
        position += 2;
    }
    if (ended) {
        position = loop;
        if (pb->addr.loopFlag) {
            pb->adpcm.pred_scale = pb->adpcmLoop.loop_pred_scale & 0x7F;
            if (pb->type != 1) {
                pb->adpcm.yn1 = pb->adpcmLoop.loop_yn1;
                pb->adpcm.yn2 = pb->adpcmLoop.loop_yn2;
            }
        } else {
            pb->state = 0;
        }
    }
    pb->addr.currentAddressHi = position >> 16;
    pb->addr.currentAddressLo = position;
    *output = decoded;
    return true;
}

bool melee_ax_decode_sample(AXPB* parameters, s16* sample)
{
    if (!parameters || !sample) return false;
    int old = OSDisableInterrupts();
    AXPB next = *parameters;
    s16 decoded;
    bool success = decode(&next, &decoded);
    if (success) { *parameters = next; *sample = decoded; }
    OSRestoreInterrupts(old);
    return success;
}

#include "ax_coefficients.inc"

bool melee_ax_resample_native(AXPB* parameters, s16* samples, size_t count)
{
    return melee_ax_resample(parameters, samples, count, native_ax_coefficients,
                             sizeof(native_ax_coefficients) / sizeof(native_ax_coefficients[0]));
}

bool melee_ax_resample(AXPB* parameters, s16* samples, size_t count,
                      const s16* coefficients, size_t coefficient_count)
{
    if (!parameters || count > 160 || (count && !samples)) return false;
    if (!count) return true;
    int old = OSDisableInterrupts();
    AXPB next = *parameters;
    s16 output[160];
    bool success = false;
    if (next.state > 1 || next.srcSelect > 2) goto finish;
    if (next.srcSelect == 0 && (!coefficients || next.coefSelect > 2 ||
        coefficient_count < ((size_t) next.coefSelect + 1) * 512)) goto finish;
    u32 ratio = address(next.src.ratioHi, next.src.ratioLo);
    if (next.srcSelect != 2 && ratio > 0x40000) goto finish;
    u32 phase = next.src.currentAddressFrac;
    s16 history[4];
    for (unsigned j = 0; j < 4; ++j) history[j] = signed_word(next.src.last_samples[j]);
    for (size_t i = 0; i < count; ++i) {
        unsigned consume = 1;
        if (next.srcSelect != 2) {
            phase += ratio;
            consume = phase >> 16;
            phase &= 0xFFFF;
        }
        s16 sample = 0;
        for (unsigned j = 0; j < consume; ++j) {
            if (!decode(&next, &sample)) goto finish;
            for (unsigned k = 0; k < 3; ++k) history[k] = history[k + 1];
            history[3] = sample;
        }
        if (next.srcSelect == 2) {
            output[i] = sample;
        } else if (next.srcSelect == 1) {
            s64 value = (s64) history[0] * (65536 - phase) + (s64) history[1] * phase;
            output[i] = (s16) floor_shift(value, 16);
        } else {
            const s16* taps = coefficients + (size_t) next.coefSelect * 512 + (phase >> 9) * 4;
            s64 value = 0;
            for (unsigned j = 0; j < 4; ++j) value += (s64) history[j] * taps[j];
            output[i] = saturate(floor_shift(value, 15));
        }
    }
    for (unsigned j = 0; j < 4; ++j) next.src.last_samples[j] = (u16) history[j];
    next.src.currentAddressFrac = phase;
    *parameters = next;
    for (size_t i = 0; i < count; ++i) samples[i] = output[i];
    success = true;
finish:
    if(!success) fprintf(stderr,"AX resample rejected state=%u type=%u format=%u current=%08x end=%08x loop=%08x src=%u coef=%u ratio=%04x%04x\n", next.state,next.type,next.addr.format,address(next.addr.currentAddressHi,next.addr.currentAddressLo),address(next.addr.endAddressHi,next.addr.endAddressLo),address(next.addr.loopAddressHi,next.addr.loopAddressLo),next.srcSelect,next.coefSelect,next.src.ratioHi,next.src.ratioLo);
    OSRestoreInterrupts(old);
    return success;
}
