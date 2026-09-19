#ifndef MELEE_NATIVE_ARCHIVE_H
#define MELEE_NATIVE_ARCHIVE_H
#include "melee_host_types.h"
#include <stddef.h>
#include <stdint.h>

/* A borrowed, immutable view of an HSD DAT file. All offsets remain disk
 * offsets; never cast its data into the original pointer-bearing structs.
 * The caller must keep the supplied bytes alive and unchanged. */
typedef struct {
    const uint8_t *bytes;
    size_t size;
    uint32_t data_size, reloc_count, public_count, extern_count;
    size_t reloc_start, public_start, extern_start, strings_start;
    struct MeleeArchiveStorage* storage; /* Borrowed token; see owned APIs below. */
} MeleeArchive;

MeleeHostBool melee_archive_open(MeleeArchive *out, const void *bytes, size_t size);
MeleeHostBool melee_archive_public(const MeleeArchive *archive, uint32_t index,
                          const char **name, uint32_t *offset);
MeleeHostBool melee_archive_find(const MeleeArchive *archive, const char *name,
                        uint32_t *offset);
MeleeHostBool melee_archive_u32(const MeleeArchive *archive, uint32_t offset, uint32_t *out);
MeleeHostBool melee_archive_f32(const MeleeArchive *archive, uint32_t offset, float *out);
/* Resolve a possibly unaligned relocation slot to a data offset. Targets may
 * equal data_size (one-past-end); scalar readers still require actual bytes.
 * Never cast a slot to a host pointer or dereference an end target. */
MeleeHostBool melee_archive_relocation(const MeleeArchive *archive, uint32_t index,
                              uint32_t *slot, uint32_t *target);
/* Read a nullable pointer slot without casting disk data. Returns false for
 * unresolved external references or a nonzero word lacking relocation. */
MeleeHostBool melee_archive_pointer(const MeleeArchive* archive, uint32_t slot,
                                  uint32_t* target, MeleeHostBool* present);
/* Owned serialized copy with all external symbols resolved to null, matching
 * lbArchive_InitializeDAT. Free the returned bytes after all views are gone. */
uint8_t* melee_archive_copy_null_externals(const MeleeArchive*,size_t* size);
/* Explicit owned views: adopt takes malloc storage only on success; acquire
 * copies a borrowed input or shares an existing immutable owned allocation.
 * Each successful call requires one release. Plain struct copies are borrowed
 * and must not be released. Never mutate owned bytes or reuse a live output. */
MeleeHostBool melee_archive_adopt(MeleeArchive* out, uint8_t* bytes, size_t size);
MeleeHostBool melee_archive_acquire(MeleeArchive* out, const MeleeArchive* input);
void melee_archive_release(MeleeArchive* owned);
#endif
