#ifndef MELEE_NATIVE_DISC_H
#define MELEE_NATIVE_DISC_H
#include "melee_host_types.h"
#include <stdint.h>
#include <stddef.h>
typedef struct MeleeDisc MeleeDisc;
typedef struct {
    uint32_t parent, next, offset, length;
    MeleeHostBool directory;
    const char* name;
} MeleeDiscEntry;
/* Owns a read-only file descriptor and a validated, host-endian filesystem.
 * Entry numbers stay identical to the disc. Returned names live until close. */
MeleeDisc* melee_disc_open(const char* image_path);
/* Reader supplies exact physical image ranges. Context is borrowed through close;
 * caller serializes access if its reader requires it. Parsing is shared with files. */
typedef MeleeHostBool (*MeleeDiscReader)(void* context, void* output, size_t length, uint64_t offset);
MeleeDisc* melee_disc_open_reader(uint64_t size, MeleeDiscReader reader, void* context);
void melee_disc_close(MeleeDisc* disc);
uint32_t melee_disc_entry_count(const MeleeDisc* disc);
const MeleeDiscEntry* melee_disc_entry(const MeleeDisc* disc, uint32_t number);
int32_t melee_disc_find(const MeleeDisc* disc, uint32_t directory, const char* path);
MeleeHostBool melee_disc_read(const MeleeDisc* disc, uint32_t entry, void* output,
                             size_t length, uint32_t offset);
MeleeHostBool melee_disc_read_at(const MeleeDisc* disc, void* output, size_t length, uint64_t offset);
const uint8_t* melee_disc_id(const MeleeDisc* disc);
#endif
