#ifndef MELEE_SSM_H
#define MELEE_SSM_H
#include <dolphin/ax.h>
#include <stddef.h>
#include <stdint.h>

/* Host metadata, independent of the packed console linked-list layout.
 * Sample addresses remain relative to the bank; loading into ARAM and
 * publishing synthesizer entries are separate operations. */
typedef struct {
    AXPBADDR address;
    AXPBADPCM adpcm;
    AXPBADPCMLOOP loop;
} MeleeSSMVoice;
typedef struct {
    uint32_t id, sample_rate, voice_count;
    MeleeSSMVoice voices[2];
} MeleeSSMEntry;
typedef struct {
    uint32_t entry_count, sample_bytes;
    MeleeSSMEntry* entries;
    unsigned char* samples;
} MeleeSSM;
MeleeSSM* melee_ssm_open(const void* bytes, size_t size);
void melee_ssm_close(MeleeSSM* bank);
#endif
