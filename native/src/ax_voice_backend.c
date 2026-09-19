#include "melee_ax_voice.h"
#include <dolphin/os.h>
#include <stdint.h>
#include <string.h>
#include "../../extern/dolphin/src/dolphin/ax/__ax.h"

static AXVPB voices[AX_MAX_VOICES];
static AXPBITDBUFFER itd[AX_MAX_VOICES] ATTRIBUTE_ALIGN(32);
static u64 generation;

AXVPB* melee_ax_voice_at(unsigned index)
{
    return generation && index < AX_MAX_VOICES ? &voices[index] : NULL;
}

u64 melee_ax_voice_generation(void) { return generation; }

bool melee_ax_voice_owned(const AXVPB* voice)
{
    uintptr_t address = (uintptr_t) voice;
    uintptr_t start = (uintptr_t) voices;
    return address >= start && address - start < sizeof(voices) &&
        (address - start) % sizeof(AXVPB) == 0;
}

void __AXSetPBDefault(AXVPB* voice)
{
    voice->pb.state = 0;
    voice->pb.itd.flag = 0;
    voice->sync = 0xA4;
    voice->updateMS = voice->updateCounter = 0;
    voice->updateWrite = voice->updateData;
    memset(voice->pb.update.updNum, 0, sizeof(voice->pb.update.updNum));
}

void __AXVPBInit(void)
{
    int old = OSDisableInterrupts();
    __AXInitVoiceStacks();
    memset(voices, 0, sizeof(voices));
    memset(itd, 0, sizeof(itd));
    if (!++generation) ++generation;
    for (unsigned i = 0; i < AX_MAX_VOICES; ++i) {
        voices[i].index = i;
        voices[i].itdBuffer = &itd[i];
        __AXSetPBDefault(&voices[i]);
        __AXPushFreeStack(&voices[i]);
    }
    /* pb next/curr/ITD/update split addresses belong to the console DSP's
     * command protocol. Native code uses the full pointers above, never
     * truncated host addresses in those fields. */
    OSRestoreInterrupts(old);
}

void melee_ax_voice_pool_init(void) { __AXVPBInit(); }
AXPBITDBUFFER* melee_ax_voice_itd_at(unsigned index)
{ return index < AX_MAX_VOICES ? &itd[index] : NULL; }
