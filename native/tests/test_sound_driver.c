#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "melee_dvd.h"
#include "melee_ax_output.h"
#include "melee_ax_voice.h"
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/ai.h>
#include <dolphin/ar.h>
#undef __assert
#include "../../src/sysdolphin/baselib/axdriver.c"

int main(int argc, char** argv)
{
    if (argc != 2 && argc != 4) {
        fprintf(stderr, "Usage: test-sound-driver CISO [BANK_INDEX BANK_PATH]\n"); return 2;
    }
    unsigned bank_index = 0;
    if (argc == 4) {
        char* end;
        unsigned long value = strtoul(argv[2], &end, 10);
        assert(*argv[2] && !*end && value < 55); bank_index = value;
    }
    static _Alignas(32) unsigned char heap[1048576];
    assert(OSInitAlloc(heap, heap + sizeof(heap), 1));
    HSD_SetHeap(OSCreateHeap(heap, heap + sizeof(heap)));
    u32 stack[3]; ARInit(stack, 3); ARQInit(); AIInit(NULL);
    assert(melee_dvd_mount(argv[1]));
    AXDriver_8038E498(64, 0, 64, 4 * 1024 * 1024);
    HSD_SynthSFXAllocateBank(2 * 1024 * 1024);
    HSD_SynthSFXAllocateBank(2 * 1024 * 1024);
    assert(HSD_SynthSFXLoad("audio/us/main.ssm", 0, NULL, 0) >= 0);
    if (bank_index) assert(HSD_SynthSFXLoad(argv[3], 1, NULL, 0) >= 0);
    HSD_SynthSFXWaitForLoadCompletion(NULL);
    AXDriver_8038DA70("audio/us/smash2.sem", NULL); assert(native_sem);
    MeleeAXOutput output = {0}; s16 pcm[320];
    unsigned nonzero = 0, started = 0;
    assert(bank_index < native_sem->counts[2]);
    unsigned bank_end = bank_index + 1 < native_sem->counts[2]
        ? native_sem->values[2][bank_index + 1] : native_sem->counts[3];
    unsigned sound_count = bank_end - native_sem->values[2][bank_index];
    for (unsigned sound = 0; sound < sound_count; ++sound) {
        fprintf(stderr, "SEM sound %d\n", sound);
        int id = AXDriver_8038CFF4(bank_index * 10000 + sound, 255, 128, 0, 0); assert(id >= 0);
        ++started;
        for (unsigned frame = 0; frame < 200; ++frame) {
            if (!melee_ax_output_frame(&output, pcm)) {
                fprintf(stderr, "Render rejected at frame %u\n", frame);
                for (unsigned index = 0; index < 64; ++index) {
                    AXVPB* voice = melee_ax_voice_at(index);
                    if (voice->priority) fprintf(stderr,
                        "voice %u state=%u format=%u SRC=%u ratio=%u:%u ITD=%u shifts=%u/%u updates=%u\n",
                        index, voice->pb.state, voice->pb.addr.format,
                        voice->pb.srcSelect, voice->pb.src.ratioHi, voice->pb.src.ratioLo,
                        voice->pb.itd.flag, voice->pb.itd.shiftL, voice->pb.itd.shiftR,
                        voice->updateCounter);
                }
                abort();
            }
            for (unsigned i = 0; i < 320; ++i) nonzero += pcm[i] != 0;
        }
        AXDriverKeyOff(id);
        for (unsigned frame = 0; frame < 40; ++frame)
            assert(melee_ax_output_frame(&output, pcm));
        assert(!AXDriver_804D7794 && AXDriver_804D77C8 == 0 && AXDriver_804D77D0 == 0);
    }
    assert(started == sound_count);
    if (bank_index == 54) {
        const u32* commands = native_sem->references[3][native_sem->values[2][bank_index]];
        assert(sound_count == 1 && commands[3] == 0x06000000);
        assert(!nonzero); /* The retail end command explicitly sets volume 0. */
    } else {
        assert(nonzero);
    }
    HSD_SynthSFXUnloadBank(0);
    HSD_SynthSFXUnloadBank(1);
    for (unsigned frame = 0; frame < 8; ++frame)
        assert(melee_ax_output_frame(&output, pcm));
    AXDriver_8038DCFC(); assert(!native_sem);
    assert(melee_dvd_unmount());
    printf("Original SEM driver bank %u: %u sounds, %u nonzero PCM samples, timed playback and key-off passed\n", bank_index, started, nonzero);
}
