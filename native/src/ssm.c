#include "melee_ssm.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static uint32_t be32(const unsigned char* p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}

/* Each AX field is a 16-bit integer, including signed coefficient/history
 * bit patterns. memcpy avoids indexing past a struct member as an array. */
static void halfwords(void* output, const unsigned char* input, size_t size)
{
    for (size_t i = 0; i < size; i += 2) {
        uint16_t value = (uint16_t)input[i] << 8 | input[i + 1];
        memcpy((unsigned char*)output + i, &value, sizeof(value));
    }
}

void melee_ssm_close(MeleeSSM* bank)
{
    if (!bank) return;
    free(bank->entries);
    free(bank->samples);
    free(bank);
}

MeleeSSM* melee_ssm_open(const void* bytes, size_t size)
{
    _Static_assert(sizeof(AXPBADDR) == 16, "SSM address layout");
    _Static_assert(sizeof(AXPBADPCM) == 40, "SSM ADPCM layout");
    _Static_assert(sizeof(AXPBADPCMLOOP) == 6, "SSM loop layout");
    if (!bytes || size < 16 || size > INT32_MAX) return NULL;
    const unsigned char* p = bytes;
    size_t header = be32(p), sample_bytes = be32(p + 4);
    uint32_t count = be32(p + 8), base = be32(p + 12);
    /* Header length excludes the first 16 bytes. Each entry has an eight
     * byte prefix and one or two 64-byte voice records. */
    if (header > size - 16 || count > header / 72 || base > INT32_MAX ||
        (count && count - 1 > (uint32_t)INT32_MAX - base)) return NULL;
    size_t metadata_end = 16 + header;
    size_t sample_start = (metadata_end + 31) & ~(size_t)31;
    if (sample_start > size || sample_bytes > size - sample_start) return NULL;
    MeleeSSM* bank = calloc(1, sizeof(*bank));
    if (!bank) return NULL;
    bank->entry_count = count;
    bank->sample_bytes = sample_bytes;
    if (count) {
        bank->entries = calloc(count, sizeof(*bank->entries));
        if (!bank->entries) goto failed;
    }
    size_t cursor = 16;
    for (uint32_t i = 0; i < count; ++i) {
        if (metadata_end - cursor < 8) goto failed;
        MeleeSSMEntry* entry = &bank->entries[i];
        entry->id = base + i;
        entry->voice_count = be32(p + cursor);
        entry->sample_rate = be32(p + cursor + 4);
        cursor += 8;
        if (entry->voice_count < 1 || entry->voice_count > 2 ||
            !entry->sample_rate || entry->sample_rate > INT32_MAX ||
            entry->voice_count > (metadata_end - cursor) / 64) goto failed;
        for (uint32_t v = 0; v < entry->voice_count; ++v) {
            MeleeSSMVoice* voice = &entry->voices[v];
            halfwords(&voice->address, p + cursor, 16);
            halfwords(&voice->adpcm, p + cursor + 16, 40);
            halfwords(&voice->loop, p + cursor + 56, 6);
            cursor += 64;
        }
    }
    if (cursor != metadata_end) goto failed;
    if (sample_bytes) {
        bank->samples = malloc(sample_bytes);
        if (!bank->samples) goto failed;
        memcpy(bank->samples, p + sample_start, sample_bytes);
    }
    return bank;
failed:
    melee_ssm_close(bank);
    return NULL;
}
