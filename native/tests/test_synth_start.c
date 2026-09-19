#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "melee_disc.h"
#include "melee_ax_voice.h"
#include "melee_ax_output.h"
#include "melee_ax_aux.h"
#undef __assert
#include "../../src/sysdolphin/baselib/synth.c"

static void address(u16* high, u16* low, u32 value)
{ *high = value >> 16; *low = value; }

int main(int argc, char** argv)
{
    static _Alignas(32) unsigned char samples[2048];
    for (unsigned i = 0; i < sizeof(samples); i += 2) {
        s16 value = i < 1024 ? 12000 : -12000;
        samples[i] = (u16)value >> 8; samples[i + 1] = value;
    }
    u32 stack[1]; ARInit(stack, 1); u32 base = ARAlloc(sizeof(samples));
    ARStartDMA(ARAM_DIR_MRAM_TO_ARAM, (ARAddress)samples, base, sizeof(samples));
    struct timespec pause = { 0, 1000000 }, start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (ARGetDMAStatus()) {
        nanosleep(&pause, NULL); clock_gettime(CLOCK_MONOTONIC, &now);
        assert(now.tv_sec - start.tv_sec < 10);
    }
    melee_ax_voice_pool_init(); assert(melee_ax_aux_init());
    struct foo entry = { .unk4 = 123, .unk8 = 2, .unkC = 16000 };
    for (unsigned i = 0; i < 2; ++i) {
        AXPBADDR* addr = &entry.voices[i].address;
        addr->format = 10; addr->loopFlag = 1;
        address(&addr->currentAddressHi, &addr->currentAddressLo, base / 2 + i * 512);
        address(&addr->loopAddressHi, &addr->loopAddressLo, base / 2 + i * 512);
        address(&addr->endAddressHi, &addr->endAddressLo, base / 2 + i * 512 + 511);
        entry.voices[i].adpcm.gain = 0x800;
    }
    HSD_Synth_804C29E0[123 & 31] = &entry;
    HSD_Synth_804C28E0_1784[0].x1784 = 1;
    HSD_Synth_804D7754 = 1;
    int id = HSD_Synth_80389334(123, 255, 255, 128, 10, 0, 1, 1, 1, 0, 0);
    assert(id > 0);
    struct HSD_SynthSFXNode* node = getNode(id); assert(node && node->voice_count == 2);
    assert(node->voice[0] != node->voice[1]);
    for (unsigned i = 0; i < 2; ++i) {
        assert(!memcmp(&node->voice[i]->pb.addr, &entry.voices[i].address, sizeof(AXPBADDR)));
        assert(node->voice[i]->pb.src.ratioHi == 0);
        assert(node->voice[i]->pb.src.ratioLo == 32768);
        assert(node->voice[i]->pb.state == 1);
    }
    MeleeAXOutput output = { 0 }; s16 pcm[320];
    assert(melee_ax_output_frame(&output, pcm));
    for (unsigned i = 16; i < 160; ++i) {
        assert(pcm[i * 2] > 8000 && pcm[i * 2] < 9000);
        assert(pcm[i * 2 + 1] < -8000 && pcm[i * 2 + 1] > -9000);
    }
    HSD_SynthSFXStopNode(node); assert(!node->x0);
    assert(HSD_Synth_80389334(999,255,255,128,10,0,1,1,1,0,0) == -1);
    entry.unk8 = 3;
    assert(HSD_Synth_80389334(123,255,255,128,10,0,1,1,1,0,0) == -1);
    MeleeSSMEntry metadata = { .id = 7, .voice_count = 1, .sample_rate = 32000 };
    metadata.voices[0].address.currentAddressLo = 2;
    metadata.voices[0].address.loopAddressLo = 2;
    metadata.voices[0].address.endAddressLo = 63;
    MeleeSSM owned = { .entry_count = 1, .sample_bytes = 32, .entries = &metadata };
    hsd_SynthSFXBankNum = 1;
    hsd_SynthSFXBankHead[0] = hsd_SynthSFXBank[0] = base;
    hsd_SynthSFXBankHead[1] = base + 64;
    assert(native_publish_group(&owned, 0, 100, base));
    struct foo* first = HSD_Synth_804C29E0[7];
    assert(first->voices[0].address.currentAddressLo == (u16)(base * 2 + 2));
    assert(native_publish_group(&owned, 0, 101, base + 32));
    struct foo* second = HSD_Synth_804C29E0[7];
    assert(first != second && second->next == first);
    metadata.sample_rate = 1;
    assert(first->unkC == 32000 && second->unkC == 32000);
    assert(!native_publish_group(&owned, 0, 102, base + 64));
    assert(HSD_Synth_804C29E0[7] == second);
    HSD_Synth_80388E08(100);
    assert(HSD_Synth_804C29E0[7] == second && !second->next);
    HSD_SynthSFXUnloadBank(0);
    assert(!HSD_Synth_804C29E0[7] && !native_sfx_groups[0]);
    assert(hsd_SynthSFXBank[0] == (int)base);
    metadata.voices[0].address.endAddressLo = 64;
    assert(!native_publish_group(&owned, 0, 103, base));
    assert(!HSD_Synth_804C29E0[7] && hsd_SynthSFXBank[0] == (int)base);
    if (argc == 2) {
        MeleeDisc* disc = melee_disc_open(argv[1]); assert(disc);
        int number = melee_disc_find(disc, 0, "audio/end.ssm"); assert(number >= 0);
        const MeleeDiscEntry* file = melee_disc_entry(disc, number);
        void* data = malloc(file->length); assert(data);
        assert(melee_disc_read(disc, number, data, file->length, 0));
        MeleeSSM* bank = melee_ssm_open(data, file->length); assert(bank);
        assert(bank->entry_count == 1 && bank->sample_bytes <= sizeof(samples));
        memcpy(samples, bank->samples, bank->sample_bytes);
        ARStartDMA(ARAM_DIR_MRAM_TO_ARAM, (ARAddress)samples, base, bank->sample_bytes);
        clock_gettime(CLOCK_MONOTONIC, &start);
        while (ARGetDMAStatus()) {
            nanosleep(&pause, NULL); clock_gettime(CLOCK_MONOTONIC, &now);
            assert(now.tv_sec - start.tv_sec < 10);
        }
        melee_ax_voice_pool_init(); assert(melee_ax_aux_init());
        memset(hsd_SynthSFXNodes, 0, sizeof(hsd_SynthSFXNodes));
        memset(HSD_Synth_804C29E0, 0, sizeof(HSD_Synth_804C29E0));
        HSD_Synth_804D774C = NULL;
        HSD_Synth_804D7754 = 0;
        hsd_SynthSFXBankNum = 1;
        hsd_SynthSFXBankHead[0] = hsd_SynthSFXBank[0] = base;
        hsd_SynthSFXBankHead[1] = base + sizeof(samples);
        unsigned sound_id = bank->entries[0].id;
        assert(native_publish_group(bank, 0, number, base));
        assert(!native_publish_group(bank, 0, number, base));
        melee_ssm_close(bank); bank = NULL;
        id = HSD_Synth_80389334(sound_id,255,255,128,10,0,1,1,1,0,0);
        assert(id > 0);
        output = (MeleeAXOutput){ 0 };
        unsigned nonzero = 0;
        for (unsigned frame = 0; frame < 10; ++frame) {
            assert(melee_ax_output_frame(&output, pcm));
            for (unsigned i = 0; i < 160; ++i) {
                assert(pcm[2*i] == pcm[2*i+1]);
                nonzero += pcm[2*i] != 0;
            }
        }
        assert(nonzero);
        HSD_Synth_80388E08(number);
        assert(!native_sfx_groups[0] && !HSD_Synth_804C29E0[sound_id & 31]);
        assert(HSD_SynthSFXCheck(id) == -1);
        HSD_SynthSFXUnloadBank(0);
        assert(hsd_SynthSFXBank[0] == (int)base);
        melee_ssm_close(bank); free(data); melee_disc_close(disc);
        printf("Retail end.ssm: parser to original synth start to stereo PCM, %u nonzero frames of 1600\n", nonzero);
    }
    puts("Original synth start: native stereo records, 16 kHz pitch, AX allocation/setters and mixed PCM passed");
}
