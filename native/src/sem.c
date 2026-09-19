#include "melee_sem.h"
#include <limits.h>
#include <stdlib.h>

void melee_sem_close(MeleeSEM* sem)
{
    if (!sem) return;
    for (unsigned i = 0; i < 5; ++i) free(sem->references[i]);
    free(sem->words);
    free(sem);
}

MeleeSEM* melee_sem_open(const void* bytes, size_t size)
{
    if (!bytes || size < 20 || size > INT32_MAX || (size & 3)) return NULL;
    MeleeSEM* sem = calloc(1, sizeof(*sem));
    if (!sem) return NULL;
    sem->words = malloc(size);
    if (!sem->words) goto failed;
    sem->word_count = size / 4;
    const unsigned char* input = bytes;
    for (size_t i = 0; i < sem->word_count; ++i) {
        const unsigned char* p = input + i * 4;
        sem->words[i] = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
                        (uint32_t)p[2] << 8 | p[3];
    }
    size_t cursor = 0;
    for (unsigned table = 0; table < 5; ++table) {
        if (cursor == sem->word_count) goto failed;
        uint32_t count = sem->words[cursor++];
        if (count > INT32_MAX || count > sem->word_count - cursor) goto failed;
        sem->counts[table] = count;
        sem->values[table] = count ? sem->words + cursor : NULL;
        cursor += count;
    }
    sem->payload_word = cursor;
    for (unsigned table = 0; table < 5; ++table) {
        if (table == 0 || table == 2 || !sem->counts[table]) continue;
        sem->references[table] = calloc(sem->counts[table], sizeof(uint32_t*));
        if (!sem->references[table]) goto failed;
        for (uint32_t i = 0; i < sem->counts[table]; ++i) {
            uint32_t offset = sem->values[table][i];
            if ((offset & 3) || offset / 4 < cursor || offset >= size) goto failed;
            sem->references[table][i] = sem->words + offset / 4;
        }
    }
    for (uint32_t i = 0; i < sem->counts[2]; ++i) {
        uint32_t offset = sem->values[2][i];
        if (offset > sem->counts[3] || (i && offset < sem->values[2][i - 1])) goto failed;
    }
    return sem;
failed:
    melee_sem_close(sem);
    return NULL;
}

bool melee_sem_command_target(const MeleeSEM* sem, const uint32_t* current,
                               size_t backwards, uint32_t** target)
{
    if (!sem || !sem->words || !current || !target) return false;
    uintptr_t base = (uintptr_t) sem->words, address = (uintptr_t) current;
    if (address < base || (address - base) % sizeof(uint32_t)) return false;
    size_t index = (address - base) / sizeof(uint32_t);
    if (index < sem->payload_word || index >= sem->word_count ||
        backwards > index - sem->payload_word) return false;
    *target = sem->words + index - backwards;
    return true;
}
