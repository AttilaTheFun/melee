#ifndef MELEE_SEM_H
#define MELEE_SEM_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* Owned host-endian SEM words and full-width relocated pointer tables.
 * Tables 0/2 are values; tables 1/3/4 contain file-relative word pointers.
 * Command interpretation remains in the original AX driver. */
typedef struct MeleeSEM {
    uint32_t* words;
    size_t word_count;
    size_t payload_word;
    uint32_t counts[5];
    uint32_t* values[5];
    uint32_t** references[5];
} MeleeSEM;
MeleeSEM* melee_sem_open(const void* bytes, size_t size);
void melee_sem_close(MeleeSEM* sem);
/* Validate a command address and optional backward word displacement without
 * performing arithmetic on an unvalidated native pointer. Failure preserves
 * the output pointer. Targets cannot enter the SEM metadata tables. */
bool melee_sem_command_target(const MeleeSEM* sem, const uint32_t* current,
                               size_t backwards, uint32_t** target);
#endif
