#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "melee_dvd.h"
#include "melee_disc.h"
#include "melee_aram.h"
#include "melee_ax_voice.h"
#include "melee_ax_output.h"
#include "melee_ax_aux.h"
#include <sysdolphin/baselib/initialize.h>
#undef __assert
#include "../../src/sysdolphin/baselib/synth.c"

static int completed;
static void loaded(int entrynum, int mode)
{
    assert(entrynum >= 0 && mode == 42);
    assert(native_sfx_groups[0]);
    ++completed;
}
static void put32(unsigned char* p, u32 value)
{ p[0] = value >> 24; p[1] = value >> 16; p[2] = value >> 8; p[3] = value; }

static void entry_path(MeleeDisc* disc, u32 number, char* output, size_t size)
{
    if (!number) { assert(size); output[0] = 0; return; }
    const MeleeDiscEntry* entry = melee_disc_entry(disc, number); assert(entry);
    entry_path(disc, entry->parent, output, size);
    size_t length = strlen(output);
    int written = snprintf(output + length, size - length, "%s%s",
                           length ? "/" : "", entry->name);
    assert(written >= 0 && (size_t)written < size - length);
}

int main(int argc, char** argv)
{
    static _Alignas(32) unsigned char heap[1048576], image[0x1080];
    assert(OSInitAlloc(heap, heap + sizeof(heap), 1));
    HSD_SetHeap(OSCreateHeap(heap, heap + sizeof(heap)));
    u32 stack[3]; ARInit(stack, 3); ARQInit();
    assert(!AICheckInit()); AIInit(NULL); assert(AICheckInit());
    assert(!AIGetStreamVolLeft() && !AIGetStreamVolRight());
    AISetStreamVolLeft(123); AISetStreamVolRight(45); AIInit(NULL);
    assert(AIGetStreamVolLeft() == 123 && AIGetStreamVolRight() == 45);
    AIReset(); AIInit(NULL);
    assert(!AIGetStreamVolLeft() && !AIGetStreamVolRight());
    HSD_SynthInit(64, 0, 64, 2 * 1024 * 1024);
    HSD_SynthSFXAllocateBank(2 * 1024 * 1024);
    assert(melee_ax_aux_ready() && AIGetDSPSampleRate() == AI_SAMPLERATE_32KHZ);
    assert(AIGetStreamVolLeft() == 255 && AIGetStreamVolRight() == 255);
    u32 base = hsd_SynthSFXBankHead[0];
    memcpy(image, "GALE01\0\2", 8); put32(image + 0x1c, 0xc2339f3d);
    put32(image + 0x424, 0x800); put32(image + 0x428, 34);
    put32(image + 0x800, 0x01000000); put32(image + 0x808, 2);
    put32(image + 0x810, 0x1000); put32(image + 0x814, 128);
    memcpy(image + 0x818, "asset.ssm", 10);
    unsigned char* file = image + 0x1000;
    put32(file, 72); put32(file + 4, 32); put32(file + 8, 1); put32(file + 12, 7);
    put32(file + 16, 1); put32(file + 20, 32000);
    put32(file + 28, 2); put32(file + 32, 63); put32(file + 36, 2);
    file[75] = 4;
    memset(file + 96, 0x77, 32);
    for (unsigned i = 0; i < 32; i += 8) file[96 + i] = 4;
    char path[] = "/tmp/melee-synth-load-XXXXXX";
    int fd = mkstemp(path); assert(fd >= 0);
    assert(write(fd, image, sizeof(image)) == sizeof(image)); close(fd);
    assert(melee_dvd_mount(path));
    bool enabled = OSDisableInterrupts();
    for (unsigned i = 0; i < 3; ++i)
        assert(HSD_SynthSFXLoad("asset.ssm", 0, loaded, 42) == 1);
    assert(HSD_SynthSFXGetPendingLoadCount() == 3);
    OSRestoreInterrupts(enabled);
    HSD_SynthSFXWaitForLoadCompletion(NULL);
    assert(completed == 3 && hsd_SynthSFXBank[0] == (int)(base + 96));
    unsigned char actual[96]; assert(melee_aram_read(base, actual, sizeof(actual)));
    for (unsigned i = 0; i < 3; ++i) assert(!memcmp(actual + i * 32, file + 96, 32));
    int sound = HSD_Synth_80389334(7,255,255,128,10,0,1,1,1,0,0);
    assert(sound > 0);
    MeleeAXOutput output = { 0 }; s16 pcm[320];
    assert(melee_ax_output_frame(&output, pcm));
    assert(HSD_Synth_804D775C == 1);
    unsigned nonzero = 0;
    for (unsigned i = 0; i < 320; ++i) nonzero += pcm[i] != 0;
    assert(nonzero);
    HSD_SynthSFXUnloadBank(0);
    assert(!native_sfx_groups[0] && !HSD_Synth_804C29E0[7]);
    assert(HSD_SynthSFXLoad("missing.ssm", 0, loaded, 42) == -1);
    /* Cancel active and queued requests while the worker is excluded. */
    enabled = OSDisableInterrupts();
    HSD_SynthSFXLoad("asset.ssm", 0, loaded, 42);
    HSD_SynthSFXLoad("asset.ssm", 0, loaded, 42);
    assert(HSD_SynthSFXCancelLoad(1));
    assert(HSD_SynthSFXCancelLoad(1));
    assert(!HSD_SynthSFXGetPendingLoadCount() && HSD_Synth_804D772C == 1);
    OSRestoreInterrupts(enabled);
    HSD_SynthSFXBankDeflag(0);
    HSD_SynthSFXBankDeflagSync();
    assert(!HSD_Synth_804D772C && !HSD_Synth_804D7738);
    assert(completed == 3 && !native_sfx_groups[0]);
    enabled = OSDisableInterrupts();
    HSD_SynthSFXLoad("asset.ssm", 0, loaded, 42);
    HSD_Synth_80388E08(1);
    OSRestoreInterrupts(enabled);
    HSD_SynthSFXWaitForLoadCompletion(NULL);
    assert(completed == 3 && !native_sfx_groups[0]);
    enabled = OSDisableInterrupts();
    HSD_SynthSFXLoad("asset.ssm", 0, loaded, 42);
    HSD_SynthSFXLoad("asset.ssm", 0, loaded, 42);
    HSD_SynthSFXUnloadBank(0);
    OSRestoreInterrupts(enabled);
    HSD_SynthSFXWaitForLoadCompletion(NULL);
    assert(completed == 3 && !native_sfx_groups[0]);
    hsd_SynthSFXBankHead[1] = base + 16;
    HSD_SynthSFXLoad("asset.ssm", 0, loaded, 42);
    HSD_SynthSFXWaitForLoadCompletion(NULL);
    assert(completed == 3 && !native_sfx_groups[0]);
    assert(hsd_SynthSFXBank[0] == (int)base);
    hsd_SynthSFXBankHead[1] = base + 2 * 1024 * 1024;
    assert(melee_dvd_unmount()); unlink(path);
    if (argc == 2) {
        assert(melee_dvd_mount(argv[1]));
        MeleeDisc* disc = melee_disc_open(argv[1]); assert(disc);
        unsigned banks = 0, sounds = 0, channels = 0;
        for (u32 i = 0; i < melee_disc_entry_count(disc); ++i) {
            const MeleeDiscEntry* file_entry = melee_disc_entry(disc, i);
            size_t length = strlen(file_entry->name);
            if (file_entry->directory || length < 4 ||
                strcmp(file_entry->name + length - 4, ".ssm")) continue;
            char bank_path[512]; entry_path(disc, i, bank_path, sizeof(bank_path));
            assert(HSD_SynthSFXLoad(bank_path, 0, loaded, 42) == (int)i);
            HSD_SynthSFXWaitForLoadCompletion(NULL);
            if (!native_sfx_groups[0]) fprintf(stderr, "Bank failed: %s\n", bank_path);
            assert(native_sfx_groups[0] && completed == 4 + (int)banks);
            struct NativeSFXGroup* group = native_sfx_groups[0];
            for (u32 j = 0; j < group->count; ++j) {
                int id = group->entries[j].unk4;
                sound = HSD_Synth_80389334(id,255,255,128,10,0,1,1,1,0,0);
                if (sound <= 0) fprintf(stderr, "Start failed: %s sound %d\n", bank_path, id);
                assert(sound > 0);
                if (!melee_ax_output_frame(&output, pcm)) {
                    fprintf(stderr, "Mix failed: %s sound %d\n", bank_path, id);
                    abort();
                }
                channels += group->entries[j].unk8;
                ++sounds;
                struct HSD_SynthSFXNode* live = getNode(sound);
                if (live) HSD_SynthSFXStopNode(live);
            }
            ++banks;
            HSD_SynthSFXUnloadBank(0);
        }
        assert(banks == 110 && sounds == 3085 && channels == 3375);
        assert(HSD_Synth_804D775C == 1 + (int)sounds);
        melee_disc_close(disc); assert(melee_dvd_unmount());
        printf("Retail public loader and original synth: %u banks, %u sounds, %u channels rendered for one native mixer frame each\n",
               banks, sounds, channels);
    }
    puts("Native SSM loader: queued DVD/ARAM transfers, completion callbacks, PCM, cancellation and pending unload passed");
}
