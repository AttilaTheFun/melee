/* Native translation of AXFX high-quality reverb. DSP operation order follows
 * extern/dolphin/src/dolphin/axfx/reverb_hi.c:HandleReverb. */
#include <dolphin/axfx.h>
#include <dolphin/os.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#pragma STDC FP_CONTRACT OFF

static int valid(const struct AXFX_REVERBHI* effect)
{
    return effect && isfinite(effect->coloration) && isfinite(effect->time) &&
        isfinite(effect->mix) && isfinite(effect->damping) && isfinite(effect->preDelay) && isfinite(effect->crosstalk) &&
        effect->crosstalk >= 0 && effect->crosstalk <= 1 &&
        effect->coloration >= 0 && effect->coloration <= 1 &&
        effect->time >= 0.01f && effect->time <= 10 &&
        effect->mix >= 0 && effect->mix <= 1 &&
        effect->damping >= 0 && effect->damping <= 1 &&
        effect->preDelay >= 0 && effect->preDelay <= 0.1f;
}

static void clear_work(struct AXFX_REVHI_WORK* work)
{
    for (unsigned i = 0; i < 9; ++i) {
        if (work->AP[i].inputs) __AXFXFree(work->AP[i].inputs);
        if (work->C[i].inputs) __AXFXFree(work->C[i].inputs);
    }
    for (unsigned i = 0; i < 3; ++i)
        if (work->preDelayLine[i]) __AXFXFree(work->preDelayLine[i]);
    memset(work, 0, sizeof(*work));
}

static int create_work(struct AXFX_REVERBHI* effect)
{
    static const s32 lengths[8] = {1789, 1999, 2333, 433, 149, 47, 73, 67};
    struct AXFX_REVHI_WORK* work = &effect->rv;
    memset(work, 0, sizeof(*work));
    for (unsigned channel = 0; channel < 3; ++channel) {
        for (unsigned i = 0; i < 6; ++i) {
            struct AXFX_REVHI_DELAYLINE* line = i < 3 ?
                &work->C[channel * 3 + i] : &work->AP[channel * 3 + i - 3];
            s32 length = lengths[i == 5 ? channel + 5 : i];
            line->length = (length + 2) * sizeof(float);
            line->inputs = __AXFXAlloc(line->length);
            if (!line->inputs) { clear_work(work); return 0; }
            memset(line->inputs, 0, line->length);
            line->outPoint = 2 * sizeof(float);
            if (i < 3) work->combCoef[channel * 3 + i] =
                powf(10.0f, (length * -3) / (32000.0f * effect->time));
        }
    }
    work->allPassCoeff = effect->coloration;
    work->level = effect->mix;
    work->crosstalk = effect->crosstalk;
    float damping = effect->damping < 0.05f ? 0.05f : effect->damping;
    work->damping = 1.0f - (0.05f + 0.8f * damping);
    work->preDelayTime = (s32) (32000.0f * effect->preDelay);
    for (unsigned channel = 0; work->preDelayTime && channel < 3; ++channel) {
        size_t bytes = (size_t) work->preDelayTime * sizeof(float);
        work->preDelayLine[channel] = __AXFXAlloc(bytes);
        if (!work->preDelayLine[channel]) { clear_work(work); return 0; }
        memset(work->preDelayLine[channel], 0, bytes);
        work->preDelayPtr[channel] = work->preDelayLine[channel];
    }
    return 1;
}

int AXFXReverbHiInit(struct AXFX_REVERBHI* effect)
{
    if (!effect) return 0;
    int old = OSDisableInterrupts();
    memset(&effect->rv, 0, sizeof(effect->rv));
    effect->tempDisableFX = 1;
    int success = valid(effect) && create_work(effect);
    if (success) effect->tempDisableFX = 0;
    OSRestoreInterrupts(old);
    return success;
}

int AXFXReverbHiShutdown(struct AXFX_REVERBHI* effect)
{
    if (!effect) return 0;
    int old = OSDisableInterrupts();
    effect->tempDisableFX = 1;
    clear_work(&effect->rv);
    OSRestoreInterrupts(old);
    return 1;
}

int AXFXReverbHiSettings(struct AXFX_REVERBHI* effect)
{
    if (!valid(effect)) return 0;
    int old = OSDisableInterrupts();
    effect->tempDisableFX = 1;
    clear_work(&effect->rv);
    int success = create_work(effect);
    if (success) effect->tempDisableFX = 0;
    OSRestoreInterrupts(old);
    return success;
}

static void advance(struct AXFX_REVHI_DELAYLINE* line)
{
    line->inPoint += 4;
    line->outPoint += 4;
    if (line->inPoint == line->length) line->inPoint = 0;
    if (line->outPoint == line->length) line->outPoint = 0;
}

static float comb(struct AXFX_REVHI_DELAYLINE* line, float coefficient, float input)
{
    line->inputs[line->inPoint / 4] = fmaf(coefficient, line->lastOutput, input);
    line->lastOutput = line->inputs[line->outPoint / 4];
    advance(line);
    return line->lastOutput;
}

static float all_pass(struct AXFX_REVHI_DELAYLINE* line, float coefficient, float input)
{
    float value = fmaf(coefficient, line->lastOutput, input);
    line->inputs[line->inPoint / 4] = value;
    float output = -fmaf(coefficient, value, -line->lastOutput); /* fnmsubs */
    line->lastOutput = line->inputs[line->outPoint / 4];
    advance(line);
    return output;
}

static s32 integer_sample(float value)
{
    /* fctiwz: truncate, saturate overflow, and use INT32_MIN for NaN. */
    if (isnan(value) || value <= -2147483648.0f) return INT32_MIN;
    if (value >= 2147483648.0f) return INT32_MAX;
    return (s32) value;
}

void DoCrossTalk(s32* left, s32* right, float cross, float inverse)
{
    /* The original paired-single code scales only the right coefficients by
     * 0.6 before multiplying, and rounds fctiw using the current rounding mode. */
    float right_cross = cross * 0.6f;
    float right_inverse = inverse * 0.6f;
    for (unsigned i = 0; i < 160; ++i) {
        float l = (float) left[i], r = (float) right[i];
        float output_l = l * inverse + r * cross;
        float output_r = l * right_cross + r * right_inverse;
        left[i] = integer_sample(nearbyintf(output_l));
        right[i] = integer_sample(nearbyintf(output_r));
    }
}

void AXFXReverbHiCallback(struct AXFX_BUFFERUPDATE* buffers,
                          struct AXFX_REVERBHI* effect)
{
    int old = OSDisableInterrupts();
    if (!effect || !buffers) OSPanic(__FILE__, __LINE__, "Invalid native reverb callback");
    if (!effect->tempDisableFX) {
        struct AXFX_REVHI_WORK* work = &effect->rv;
        s32* channels[3] = {buffers->left, buffers->right, buffers->surround};
        if (work->crosstalk != 0) {
            if (!channels[0] || !channels[1])
                OSPanic(__FILE__, __LINE__, "Invalid native crosstalk channels");
            float cross = 0.5f * work->crosstalk;
            DoCrossTalk(channels[0], channels[1], cross, 1.0f - cross);
        }
        float wet = work->level * 0.6f;
        float dry = 0.6f - wet;
        for (unsigned channel = 0; channel < 3; ++channel) {
            if (!channels[channel] || !work->C[3 * channel].inputs)
                OSPanic(__FILE__, __LINE__, "Uninitialized native reverb channel");
            for (unsigned i = 0; i < 160; ++i) {
                float original = (float) channels[channel][i];
                float input = original;
                if (work->preDelayTime) {
                    /* Verified DOL 0x8035BEB4 / 0x8035C04C write the
                     * cursor into sample memory, leaving preDelayPtr fixed.
                     * Preserve the audible one-sample delay without that write. */
                    input = work->preDelayLine[channel][0];
                    work->preDelayLine[channel][0] = original;
                }
                float first = comb(&work->C[3 * channel], work->combCoef[3 * channel], input);
                float second = comb(&work->C[3 * channel + 1], work->combCoef[3 * channel + 1], input);
                float third = comb(&work->C[3 * channel + 2], work->combCoef[3 * channel + 2], input);
                float value = all_pass(&work->AP[3 * channel], work->allPassCoeff, (first + second) + third);
                value = all_pass(&work->AP[3 * channel + 1], work->allPassCoeff, value);
                value = fmaf(work->damping, work->lpLastout[channel], value * 0.3f);
                work->lpLastout[channel] = value;
                value = all_pass(&work->AP[3 * channel + 2], work->allPassCoeff, value);
                channels[channel][i] = integer_sample(fmaf(wet, value, dry * original));
            }
        }
    }
    OSRestoreInterrupts(old);
}
