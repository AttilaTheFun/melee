#include <assert.h>
#include <stdio.h>
#include <string.h>
#undef __assert
#include "../../src/sysdolphin/baselib/synth.c"

static unsigned freed[64];
void AXFreeVoice(AXVPB* voice)
{
    /* The range scan must serialize address reads and voice retirement with
     * the mixer. Nested disable returns false while that gate is held. */
    BOOL previous = OSDisableInterrupts();
    assert(!previous);
    OSRestoreInterrupts(previous);
    assert(voice->index < 64);
    ++freed[voice->index];
}

int main(void)
{
    AXVPB voices[8] = { 0 };
    const u32 lo = 0x12340, hi = 0x12560;
    const u32 addresses[] = { lo - 1, lo, lo + 1, hi - 1, hi,
                              hi + 1, lo, lo };
    for (unsigned i = 0; i < 8; ++i) {
        voices[i].index = i;
        voices[i].pb.addr.currentAddressHi = addresses[i] >> 16;
        voices[i].pb.addr.currentAddressLo = addresses[i];
        /* Adjacent ADPCM data must not become part of the address. */
        memset(&voices[i].pb.adpcm, 0xA5, sizeof(voices[i].pb.adpcm));
        hsd_SynthSFXNodes[i].x0 = i + 1;
        hsd_SynthSFXNodes[i].voice_count = 1;
        hsd_SynthSFXNodes[i].voice[0] = &voices[i];
    }
    /* A stereo sound retires both voices through its primary node. */
    hsd_SynthSFXNodes[2].voice_count = 2;
    hsd_SynthSFXNodes[2].voice[1] = &voices[6];
    hsd_SynthSFXNodes[6].x0 = -1;
    /* Inactive entries can retain an address inside the bank. */
    hsd_SynthSFXNodes[7].x0 = 0;
    hsd_SynthSFXBankHead[0] = lo / 2;
    hsd_SynthSFXBankHead[1] = hi / 2;
    HSD_SynthSFXStopRange(0);
    for (unsigned i = 0; i < 8; ++i) {
        bool expected = i == 1 || i == 2 || i == 3 || i == 6;
        assert(freed[i] == expected);
        if (expected) assert(hsd_SynthSFXNodes[i].x0 == 0);
    }
    /* Repeated and empty ranges do not retire additional voices. */
    HSD_SynthSFXStopRange(0);
    stopRange(hi, hi);
    assert(freed[1] == 1 && freed[2] == 1 && freed[3] == 1 && freed[6] == 1);
    assert(!freed[0] && !freed[4] && !freed[5] && !freed[7]);
    BOOL previous = OSDisableInterrupts();
    assert(previous);
    OSRestoreInterrupts(previous);
    puts("Synth bank retirement: split address, half-open bounds, stereo, inactive voices and interrupt gate passed");
}
