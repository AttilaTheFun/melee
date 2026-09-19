#include <dolphin.h>
#include <dolphin/ax.h>
#include <dolphin/axfx.h>

#ifdef MELEE_NATIVE
#include <limits.h>
#include <string.h>

/* PPC mullw/add keep the low 32 bits. Use unsigned arithmetic so feedback
 * overflow has the original wrapping behavior without signed C overflow. */
static s32 sample_bits(u32 bits)
{
    s32 sample;
    memcpy(&sample, &bits, sizeof(sample));
    return sample;
}

void AXFXDelayCallback(struct AXFX_BUFFERUPDATE* buffers,
                       struct AXFX_DELAY* delay)
{
    int old = OSDisableInterrupts();
    s32* output[3] = {buffers->left, buffers->right, buffers->surround};
    s32* history[3] = {delay->left, delay->right, delay->sur};
    for (unsigned channel = 0; channel < 3; ++channel) {
        if (!history[channel] || !output[channel] ||
            !delay->currentSize[channel] ||
            delay->currentPos[channel] >= delay->currentSize[channel])
            OSPanic(__FILE__, __LINE__, "Invalid native delay callback state");
        s32* block = history[channel] + delay->currentPos[channel] * 160;
        for (unsigned i = 0; i < 160; ++i) {
            u32 delayed = (u32) block[i];
            s32 feedback = sample_bits(delayed * delay->currentFeedback[channel]) >> 7;
            block[i] = sample_bits((u32) output[channel][i] + (u32) feedback);
            output[channel][i] = sample_bits(delayed * delay->currentOutput[channel]) >> 7;
        }
        delay->currentPos[channel] = (delay->currentPos[channel] + 1) %
                                    delay->currentSize[channel];
    }
    OSRestoreInterrupts(old);
}

int AXFXDelayShutdown(struct AXFX_DELAY* delay)
{
    if (!delay) return 0;
    int old = OSDisableInterrupts();
    if (delay->left) __AXFXFree(delay->left);
    if (delay->right) __AXFXFree(delay->right);
    if (delay->sur) __AXFXFree(delay->sur);
    delay->left = delay->right = delay->sur = NULL;
    memset(delay->currentSize, 0, sizeof(delay->currentSize));
    memset(delay->currentPos, 0, sizeof(delay->currentPos));
    OSRestoreInterrupts(old);
    return 1;
}

int AXFXDelaySettings(struct AXFX_DELAY* delay)
{
    if (!delay) return 0;
    u32 blocks[3];
    for (unsigned i = 0; i < 3; ++i) {
        if (delay->delay[i] < 6 || delay->feedback[i] > 100 || delay->output[i] > 100)
            return 0;
        u64 count = (((u64) delay->delay[i] - 5) * 32 + 159) / 160;
        if (count > UINT32_MAX / (160 * sizeof(s32))) return 0;
        blocks[i] = (u32) count;
    }
    int old = OSDisableInterrupts();
    AXFXDelayShutdown(delay);
    s32** history[3] = {&delay->left, &delay->right, &delay->sur};
    for (unsigned i = 0; i < 3; ++i) {
        size_t bytes = (size_t) blocks[i] * 160 * sizeof(s32);
        *history[i] = __AXFXAlloc(bytes);
        if (!*history[i]) {
            AXFXDelayShutdown(delay);
            OSRestoreInterrupts(old);
            return 0;
        }
        memset(*history[i], 0, bytes);
        delay->currentSize[i] = blocks[i];
        delay->currentFeedback[i] = (delay->feedback[i] * 128) / 100;
        delay->currentOutput[i] = (delay->output[i] * 128) / 100;
    }
    OSRestoreInterrupts(old);
    return 1;
}

int AXFXDelayInit(struct AXFX_DELAY* delay)
{
    if (!delay) return 0;
    int old = OSDisableInterrupts();
    delay->left = delay->right = delay->sur = NULL;
    memset(delay->currentSize, 0, sizeof(delay->currentSize));
    memset(delay->currentPos, 0, sizeof(delay->currentPos));
    int result = AXFXDelaySettings(delay);
    OSRestoreInterrupts(old);
    return result;
}
#else
void AXFXDelayCallback(struct AXFX_BUFFERUPDATE* bufferUpdate,
                       struct AXFX_DELAY* delay)
{
    s32 l;
    s32 r;
    s32 s;
    s32* lBuf;
    s32* rBuf;
    s32* sBuf;
    u32 i;
    s32* left;
    s32* right;
    s32* sur;

    left = bufferUpdate->left;
    right = bufferUpdate->right;
    sur = bufferUpdate->surround;
    lBuf = delay->left + (delay->currentPos[0] * 0xA0);
    rBuf = delay->right + (delay->currentPos[1] * 0xA0);
    sBuf = delay->sur + (delay->currentPos[2] * 0xA0);

    for (i = 0; i < 160; i++) {
        l = *lBuf;
        r = *rBuf;
        s = *sBuf;
        *lBuf++ = *left + ((s32) (l * delay->currentFeedback[0]) >> 7);
        *rBuf++ = *right + ((s32) (r * delay->currentFeedback[1]) >> 7);
        *sBuf++ = *sur + ((s32) (s * delay->currentFeedback[2]) >> 7);
        *left++ = (s32) (l * delay->currentOutput[0]) >> 7;
        *right++ = (s32) (r * delay->currentOutput[1]) >> 7;
        *sur++ = (s32) (s * delay->currentOutput[2]) >> 7;
    }
    delay->currentPos[0] =
        (s32) ((delay->currentPos[0] + 1) % delay->currentSize[0]);
    delay->currentPos[1] =
        (s32) ((delay->currentPos[1] + 1) % delay->currentSize[1]);
    delay->currentPos[2] =
        (s32) ((delay->currentPos[2] + 1) % delay->currentSize[2]);
}

int AXFXDelaySettings(struct AXFX_DELAY* delay)
{
    unsigned long i;
    s32* l;
    s32* r;
    s32* s;
    int old;

    AXFXDelayShutdown(delay);
    old = OSDisableInterrupts();

    for (i = 0; i < 3; i++) {
        delay->currentSize[i] = (((delay->delay[i] - 5) << 5) + 0x9F) / 160U;
        delay->currentPos[i] = 0;
        delay->currentFeedback[i] = (delay->feedback[i] << 7) / 100U;
        delay->currentOutput[i] = (delay->output[i] << 7) / 100U;
    }
    delay->left = __AXFXAlloc(delay->currentSize[0] * 0xA0 * 4);
    delay->right = __AXFXAlloc(delay->currentSize[1] * 0xA0 * 4);
    delay->sur = __AXFXAlloc(delay->currentSize[2] * 0xA0 * 4);
    ASSERTLINE(0x47, delay->left != NULL);
    ASSERTLINE(0x48, delay->right != NULL);
    ASSERTLINE(0x49, delay->sur != NULL);
    l = delay->left;
    r = delay->right;
    s = delay->sur;
    for (i = 0; i < delay->currentSize[0] * 0xA0; i++) {
        *l++ = 0;
    }
    for (i = 0; i < delay->currentSize[1] * 0xA0; i++) {
        *r++ = 0;
    }
    for (i = 0; i < delay->currentSize[2] * 0xA0; i++) {
        *s++ = 0;
    }
    OSRestoreInterrupts(old);
    return 1;
}

int AXFXDelayInit(struct AXFX_DELAY* delay)
{
    int old;

    old = OSDisableInterrupts();
    delay->left = NULL;
    delay->right = NULL;
    delay->sur = NULL;
    OSRestoreInterrupts(old);
    AXFXDelaySettings(delay);
}

int AXFXDelayShutdown(struct AXFX_DELAY* delay)
{
    int old;

    old = OSDisableInterrupts();
    if (delay->left) {
        __AXFXFree(delay->left);
    }
    if (delay->right) {
        __AXFXFree(delay->right);
    }
    if (delay->sur) {
        __AXFXFree(delay->sur);
    }
    OSRestoreInterrupts(old);
    return 1;
}

#endif
