#include <dolphin/axfx.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned allocations, frees, live, fail_at;
static void* allocate(unsigned long bytes)
{
    ++allocations;
    if (allocations == fail_at) return NULL;
    void* result = malloc(bytes);
    assert(result && (uintptr_t) result > UINT32_MAX);
    ++live;
    return result;
}
static void release(void* pointer)
{
    assert(pointer && live);
    --live;
    ++frees;
    free(pointer);
}
/* Independent wide-integer oracle for PPC low-word multiplication and srawi. */
static s32 wrap(s64 value)
{
    value %= 4294967296LL;
    if (value < 0) value += 4294967296LL;
    if (value >= 2147483648LL) value -= 4294967296LL;
    return (s32) value;
}
static s32 gain(s32 sample, u32 coefficient)
{
    s64 product = wrap((s64) sample * coefficient);
    return (s32) (product >= 0 ? product / 128 : -((-product + 127) / 128));
}
int main(void)
{
    _Static_assert(sizeof(*((struct AXFX_BUFFERUPDATE*) 0)->left) == 4, "32-bit AX samples");
    struct AXFX_DELAY delay = {.delay = {6, 11, 16},
        .feedback = {50, 50, 50}, .output = {100, 100, 100}};
    AXFXSetHooks(allocate, release);
    assert(AXFXDelayInit(&delay) == 1 && live == 3);
    s32 samples[3][160] = {{0}};
    struct AXFX_BUFFERUPDATE buffer = {samples[0], samples[1], samples[2]};
    for (unsigned frame = 0; frame < 10; ++frame) {
        memset(samples, 0, sizeof(samples));
        if (!frame) for (unsigned c = 0; c < 3; ++c) samples[c][0] = 12800;
        AXFXDelayCallback(&buffer, &delay);
        for (unsigned c = 0; c < 3; ++c) {
            unsigned period = c + 1;
            s32 expected = frame && frame % period == 0 ? 12800 >> (frame / period - 1) : 0;
            assert(samples[c][0] == expected);
            for (unsigned i = 1; i < 160; ++i) assert(samples[c][i] == 0);
        }
    }
    /* Reset the lines and compare every sample with a separate history model,
     * including negative values and deliberate 32-bit multiply/add overflow. */
    assert(AXFXDelaySettings(&delay));
    s32 reference[3][480] = {{0}};
    u32 positions[3] = {0};
    u32 random = 0xA50779u;
    for (unsigned frame = 0; frame < 30; ++frame) {
        s32 expected[3][160];
        for (unsigned c = 0; c < 3; ++c) {
            for (unsigned i = 0; i < 160; ++i) {
                random = random * 1664525u + 1013904223u;
                s32 input = wrap(random);
                if (i == 0) input = INT32_MIN;
                if (i == 1) input = INT32_MAX;
                samples[c][i] = input;
                unsigned index = positions[c] * 160 + i;
                s32 delayed = reference[c][index];
                reference[c][index] = wrap((s64) input + gain(delayed, 64));
                expected[c][i] = gain(delayed, 128);
            }
            positions[c] = (positions[c] + 1) % (c + 1);
        }
        AXFXDelayCallback(&buffer, &delay);
        assert(!memcmp(samples, expected, sizeof(samples)));
    }
    s32* previous = delay.left;
    delay.delay[0] = 5;
    assert(!AXFXDelaySettings(&delay) && delay.left == previous);
    delay.delay[0] = UINT32_MAX;
    assert(!AXFXDelaySettings(&delay) && delay.left == previous);
    delay.delay[0] = 6;
    delay.feedback[1] = 101;
    assert(!AXFXDelaySettings(&delay) && delay.left == previous);
    delay.feedback[1] = 50;
    assert(AXFXDelayShutdown(&delay) && live == 0);
    assert(AXFXDelayShutdown(&delay) && live == 0);
    for (unsigned failure = 1; failure <= 3; ++failure) {
        fail_at = allocations + failure;
        assert(!AXFXDelayInit(&delay));
        assert(!live && !delay.left && !delay.right && !delay.sur);
        assert(AXFXDelayShutdown(&delay));
    }
    fail_at = 0;
    assert(AXFXDelayInit(&delay));
    assert(AXFXDelayShutdown(&delay) && !live);
    assert(!AXFXDelayInit(NULL) && !AXFXDelaySettings(NULL) && !AXFXDelayShutdown(NULL));
    printf("AX delay: three-channel impulse timing, 14400 samples against PPC arithmetic oracle, allocation failures and repeated shutdown passed (%u frees)\n", frees);
}
