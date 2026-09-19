#include "melee_ssm.h"
#include "melee_disc.h"
#include "melee_ax_decode.h"
#include <dolphin/ar.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void relocate(u16* high, u16* low, u32 offset)
{
    u32 value = ((u32)*high << 16) | *low;
    assert(value <= UINT32_MAX - offset);
    value += offset; *high = value >> 16; *low = value;
}

static unsigned decode_bank(const MeleeSSM* bank)
{
    assert(bank->sample_bytes && !(bank->sample_bytes & 31));
    void* aligned = NULL;
    assert(!posix_memalign(&aligned, 32, bank->sample_bytes));
    memcpy(aligned, bank->samples, bank->sample_bytes);
    u32 base = ARAlloc(bank->sample_bytes); assert(base);
    ARStartDMA(ARAM_DIR_MRAM_TO_ARAM, (ARAddress)aligned, base, bank->sample_bytes);
    struct timespec start, now, pause = { 0, 1000000 };
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (ARGetDMAStatus()) {
        nanosleep(&pause, NULL); clock_gettime(CLOCK_MONOTONIC, &now);
        assert(now.tv_sec - start.tv_sec < 10);
    }
    unsigned nonzero = 0;
    for (unsigned i = 0; i < bank->entry_count; ++i)
        for (unsigned v = 0; v < bank->entries[i].voice_count; ++v) {
            const MeleeSSMVoice* voice = &bank->entries[i].voices[v];
            AXPB pb = { .state = 1 };
            pb.addr = voice->address; pb.adpcm = voice->adpcm;
            pb.adpcmLoop = voice->loop;
            /* Retail SSM voice addresses are ADPCM nibble offsets. */
            assert(pb.addr.format == 0);
            relocate(&pb.addr.currentAddressHi, &pb.addr.currentAddressLo, base * 2);
            relocate(&pb.addr.endAddressHi, &pb.addr.endAddressLo, base * 2);
            relocate(&pb.addr.loopAddressHi, &pb.addr.loopAddressLo, base * 2);
            for (unsigned sample = 0; sample < 160; ++sample) {
                s16 pcm; assert(melee_ax_decode_sample(&pb, &pcm));
                nonzero += pcm != 0;
            }
        }
    u32 released; ARFree(&released); assert(released == bank->sample_bytes);
    free(aligned);
    return nonzero;
}

static void store(unsigned char* p, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = value >> (24 - i * 8);
}

int main(int argc, char** argv)
{
    unsigned char storage[193] = { 0 }, *bytes = storage + 1;
    /* Two-channel entry: 8 + 2*64 metadata bytes, padded to offset 160. */
    store(bytes, 136); store(bytes + 4, 32);
    store(bytes + 8, 1); store(bytes + 12, 123);
    store(bytes + 16, 2); store(bytes + 20, 32000);
    for (unsigned v = 0; v < 2; ++v)
        for (unsigned i = 0; i < 64; ++i) bytes[24 + v * 64 + i] = v * 64 + i;
    memset(bytes + 160, 0xA7, 32);
    MeleeSSM* bank = melee_ssm_open(bytes, 192); assert(bank);
    assert(bank->entry_count == 1 && bank->sample_bytes == 32);
    assert(bank->entries[0].id == 123 && bank->entries[0].sample_rate == 32000);
    assert(bank->entries[0].voice_count == 2);
    for (unsigned v = 0; v < 2; ++v) {
        MeleeSSMVoice* voice = &bank->entries[0].voices[v];
        unsigned add = v * 0x4040;
        assert(voice->address.currentAddressHi == 0x0C0D + add);
        assert(voice->adpcm.a[0][0] == 0x1011 + add);
        assert(voice->adpcm.yn2 == 0x3637 + add);
        assert(voice->loop.loop_yn2 == 0x3C3D + add);
    }
    memset(bytes + 160, 0, 32);
    for (unsigned i = 0; i < 32; ++i) assert(bank->samples[i] == 0xA7);
    melee_ssm_close(bank);
    for (unsigned size = 0; size < 192; ++size) assert(!melee_ssm_open(bytes, size));
    const unsigned offsets[] = { 0, 4, 8, 12, 16, 20 };
    for (unsigned i = 0; i < 6; ++i) {
        unsigned char bad[192]; memcpy(bad, bytes, 192);
        store(bad + offsets[i], UINT32_MAX);
        assert(!melee_ssm_open(bad, sizeof(bad)));
    }
    for (unsigned voices = 0; voices < 4; voices += 3) {
        store(bytes + 16, voices); assert(!melee_ssm_open(bytes, 192));
    }
    assert(!melee_ssm_open(NULL, 192)); melee_ssm_close(NULL);
    if (argc == 2) {
        MeleeDisc* disc = melee_disc_open(argv[1]); assert(disc);
        u32 ar_stack[1]; ARInit(ar_stack, 1);
        unsigned banks = 0, entries = 0, voices = 0, stereo = 0, nonzero = 0;
        for (uint32_t i = 0; i < melee_disc_entry_count(disc); ++i) {
            const MeleeDiscEntry* file = melee_disc_entry(disc, i);
            size_t length = strlen(file->name);
            if (file->directory || length < 4 || strcmp(file->name + length - 4, ".ssm")) continue;
            void* data = malloc(file->length); assert(data);
            assert(melee_disc_read(disc, i, data, file->length, 0));
            bank = melee_ssm_open(data, file->length);
            if (!bank) fprintf(stderr, "SSM parse failed: %s\n", file->name);
            assert(bank);
            nonzero += decode_bank(bank);
            ++banks; entries += bank->entry_count;
            for (unsigned j = 0; j < bank->entry_count; ++j) {
                voices += bank->entries[j].voice_count;
                stereo += bank->entries[j].voice_count == 2;
            }
            melee_ssm_close(bank); free(data);
        }
        assert(banks && nonzero);
        printf("Retail SSM metadata: %u banks, %u entries, %u voices, %u stereo entries\n",
               banks, entries, voices, stereo);
        printf("Native ADPCM: first 160 samples of every voice decoded, %u nonzero samples\n", nonzero);
        melee_disc_close(disc);
    }
    puts("SSM metadata: host halfwords, owned sample bytes, stereo layout and malformed lengths/counts passed");
}
