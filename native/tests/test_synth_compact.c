#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "melee_aram.h"
#include "melee_ax_voice.h"
#include <sysdolphin/baselib/initialize.h>
#undef __assert
#include "../../src/sysdolphin/baselib/synth.c"

#define LARGE 0x9020
#define BANK_SIZE (32 + LARGE + 64)
int main(void)
{
    static _Alignas(32) unsigned char heap[1048576], encoded[BANK_SIZE];
    unsigned char copied[LARGE + 64];
    assert(OSInitAlloc(heap, heap + sizeof(heap), 1));
    HSD_SetHeap(OSCreateHeap(heap, heap + sizeof(heap)));
    u32 stack[1]; ARInit(stack, 1); ARQInit();
    u32 base = ARAlloc(BANK_SIZE);
    for (unsigned i = 0; i < sizeof(encoded); ++i) encoded[i] = i * 13 + i / 251;
    ARStartDMA(ARAM_DIR_MRAM_TO_ARAM, (ARAddress)encoded, base, BANK_SIZE);
    struct timespec pause = {0, 1000000}, start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (ARGetDMAStatus()) {
        nanosleep(&pause, NULL); clock_gettime(CLOCK_MONOTONIC, &now);
        assert(now.tv_sec - start.tv_sec < 10);
    }
    melee_ax_voice_pool_init();
    hsd_SynthSFXBankNum = 1;
    hsd_SynthSFXBankHead[0] = hsd_SynthSFXBank[0] = base;
    hsd_SynthSFXBankHead[1] = base + BANK_SIZE;
    MeleeSSMEntry entry = { .id = 10, .sample_rate = 32000, .voice_count = 1 };
    entry.voices[0].address.currentAddressLo = 2;
    entry.voices[0].address.loopAddressLo = 2;
    entry.voices[0].address.endAddressLo = 63;
    MeleeSSM bank = { .entries = &entry, .entry_count = 1, .sample_bytes = 32 };
    assert(native_publish_group(&bank, 0, 100, base));
    entry.id = 11; bank.sample_bytes = LARGE;
    assert(native_publish_group(&bank, 0, 101, base + 32));
    entry.id = 12; bank.sample_bytes = 64;
    assert(native_publish_group(&bank, 0, 102, base + 32 + LARGE));
    HSD_Synth_80388E08(100);
    HSD_Synth_804C28E0_1784[0].x1784 = 1;
    int sound = HSD_Synth_80389334(11,255,255,128,10,0,1,1,1,0,0);
    assert(sound > 0 && HSD_SynthSFXCheck(sound) == sound);
    bool enabled = OSDisableInterrupts();
    HSD_SynthSFXBankDeflag(0);
    assert(sfxGroupDataReaddressCounter == 2);
    assert(HSD_Synth_80389334(11,255,255,128,10,0,1,1,1,0,0) == -1);
    OSRestoreInterrupts(enabled);
    HSD_SynthSFXBankDeflagSync();
    assert(HSD_SynthSFXCheck(sound) == -1);
    assert(!sfxGroupDataReaddressCounter);
    assert(hsd_SynthSFXBank[0] == (int)(base + LARGE + 64));
    assert(melee_aram_read(base, copied, sizeof(copied)));
    assert(!memcmp(copied, encoded + 32, sizeof(copied)));
    for (unsigned id = 11; id <= 12; ++id) {
        struct foo* record = HSD_Synth_804C29E0[id]; assert(record);
        AXPBADDR* a = &record->voices[0].address;
        u32 expected = (base + (id == 12 ? LARGE : 0)) * 2;
        assert((((u32)a->currentAddressHi << 16) | a->currentAddressLo) == expected + 2);
        assert((((u32)a->loopAddressHi << 16) | a->loopAddressLo) == expected + 2);
        assert((((u32)a->endAddressHi << 16) | a->endAddressLo) == expected + 63);
    }
    HSD_SynthSFXBankDeflag(0); HSD_SynthSFXBankDeflagSync();
    HSD_SynthSFXUnloadBank(0);
    HSD_SynthSFXBankDeflag(0); HSD_SynthSFXBankDeflagSync();
    assert(hsd_SynthSFXBank[0] == (int)base);
    puts("Native synth compaction: overlapping multi-chunk ARAM moves, two groups, voice retirement, metadata relocation and bank cursor passed");
}
