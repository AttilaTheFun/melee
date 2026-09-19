/* Native translation of AXFX standard reverb. DSP operation order follows
 * extern/dolphin/src/dolphin/axfx/reverb_std.c:HandleReverb. */
#include <dolphin/axfx.h>
#include <dolphin/os.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#pragma STDC FP_CONTRACT OFF

static int valid(const struct AXFX_REVERBSTD* effect)
{
    return effect && isfinite(effect->coloration) && isfinite(effect->time) &&
        isfinite(effect->mix) && isfinite(effect->damping) && isfinite(effect->preDelay) &&
        effect->coloration >= 0 && effect->coloration <= 1 &&
        effect->time >= 0.01f && effect->time <= 10 &&
        effect->mix >= 0 && effect->mix <= 1 &&
        effect->damping >= 0 && effect->damping <= 1 &&
        effect->preDelay >= 0 && effect->preDelay <= 0.1f;
}

static void clear_work(struct AXFX_REVSTD_WORK* work)
{
    for (unsigned i = 0; i < 6; ++i) {
        if (work->AP[i].inputs) __AXFXFree(work->AP[i].inputs);
        if (work->C[i].inputs) __AXFXFree(work->C[i].inputs);
    }
    for (unsigned i = 0; i < 3; ++i)
        if (work->preDelayLine[i]) __AXFXFree(work->preDelayLine[i]);
    memset(work, 0, sizeof(*work));
}

static int create_work(struct AXFX_REVERBSTD* effect)
{
    static const s32 lengths[4] = {1789, 1999, 433, 149};
    struct AXFX_REVSTD_WORK* work = &effect->rv;
    memset(work, 0, sizeof(*work));
    for (unsigned channel = 0; channel < 3; ++channel) {
        for (unsigned i = 0; i < 4; ++i) {
            struct AXFX_REVSTD_DELAYLINE* line = i < 2 ?
                &work->C[channel * 2 + i] : &work->AP[channel * 2 + i - 2];
            line->length = (lengths[i] + 2) * sizeof(float);
            line->inputs = __AXFXAlloc(line->length);
            if (!line->inputs) { clear_work(work); return 0; }
            memset(line->inputs, 0, line->length);
            line->outPoint = 2 * sizeof(float);
            if (i < 2) work->combCoef[channel * 2 + i] =
                powf(10.0f, (lengths[i] * -3) / (32000.0f * effect->time));
        }
    }
    work->allPassCoeff = effect->coloration;
    work->level = effect->mix;
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

int AXFXReverbStdInit(struct AXFX_REVERBSTD* effect)
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

int AXFXReverbStdShutdown(struct AXFX_REVERBSTD* effect)
{
    if (!effect) return 0;
    int old = OSDisableInterrupts();
    effect->tempDisableFX = 1;
    clear_work(&effect->rv);
    OSRestoreInterrupts(old);
    return 1;
}

int AXFXReverbStdSettings(struct AXFX_REVERBSTD* effect)
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

static void advance(struct AXFX_REVSTD_DELAYLINE* line)
{
    line->inPoint += 4;
    line->outPoint += 4;
    if (line->inPoint == line->length) line->inPoint = 0;
    if (line->outPoint == line->length) line->outPoint = 0;
}

static float comb(struct AXFX_REVSTD_DELAYLINE* line, float coefficient, float input)
{
    line->inputs[line->inPoint / 4] = fmaf(coefficient, line->lastOutput, input);
    line->lastOutput = line->inputs[line->outPoint / 4];
    advance(line);
    return line->lastOutput;
}

static float all_pass(struct AXFX_REVSTD_DELAYLINE* line, float coefficient, float input)
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

void AXFXReverbStdCallback(struct AXFX_BUFFERUPDATE* buffers,
                          struct AXFX_REVERBSTD* effect)
{
    int old = OSDisableInterrupts();
    if (!effect || !buffers) OSPanic(__FILE__, __LINE__, "Invalid native reverb callback");
    if (!effect->tempDisableFX) {
        struct AXFX_REVSTD_WORK* work = &effect->rv;
        s32* channels[3] = {buffers->left, buffers->right, buffers->surround};
        float wet = work->level * 0.6f;
        float dry = 0.6f - wet;
        for (unsigned channel = 0; channel < 3; ++channel) {
            if (!channels[channel] || !work->C[2 * channel].inputs)
                OSPanic(__FILE__, __LINE__, "Uninitialized native reverb channel");
            for (unsigned i = 0; i < 160; ++i) {
                float original = (float) channels[channel][i];
                float input = original;
                if (work->preDelayTime) {
                    float* cursor = work->preDelayPtr[channel];
                    input = *cursor;
                    *cursor++ = original;
                    /* The SDK wraps at N-1. Keep that timing for N>=2;
                     * its N=1 case overruns, so use a single safe sample. */
                    s32 count = work->preDelayTime > 1 ? work->preDelayTime - 1 : 1;
                    if (cursor == work->preDelayLine[channel] + count)
                        cursor = work->preDelayLine[channel];
                    work->preDelayPtr[channel] = cursor;
                }
                float first = comb(&work->C[2 * channel], work->combCoef[2 * channel], input);
                float second = comb(&work->C[2 * channel + 1], work->combCoef[2 * channel + 1], input);
                float value = all_pass(&work->AP[2 * channel], work->allPassCoeff, first + second);
                value = fmaf(work->damping, work->lpLastout[channel], value * 0.3f);
                work->lpLastout[channel] = value;
                value = all_pass(&work->AP[2 * channel + 1], work->allPassCoeff, value);
                channels[channel][i] = integer_sample(fmaf(wet, value, dry * original));
            }
        }
    }
    OSRestoreInterrupts(old);
}
