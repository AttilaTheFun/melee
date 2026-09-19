#include "melee_archive.h"
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static MeleeHostBool table_end(size_t start, uint32_t count, size_t stride,
                      size_t size, size_t *end)
{
    if (start > size || count > (size - start) / stride) return false;
    *end = start + (size_t)count * stride;
    return true;
}

static MeleeHostBool symbol(const MeleeArchive *a, size_t entry,
                   const char **name, uint32_t *offset)
{
    uint32_t data = be32(a->bytes + entry);
    uint32_t string = be32(a->bytes + entry + 4);
    if (data >= a->data_size || string >= a->size - a->strings_start) return false;
    const char *s = (const char *)a->bytes + a->strings_start + string;
    if (!memchr(s, 0, a->size - a->strings_start - string)) return false;
    *name = s;
    *offset = data;
    return true;
}

MeleeHostBool melee_archive_u32(const MeleeArchive *a, uint32_t offset, uint32_t *out)
{
    if (!a || !a->bytes || !out || offset > a->data_size ||
        a->data_size - offset < 4) return false;
    *out = be32(a->bytes + 32 + offset);
    return true;
}

MeleeHostBool melee_archive_f32(const MeleeArchive *a, uint32_t offset, float *out)
{
    uint32_t bits;
    if (!out || !melee_archive_u32(a, offset, &bits)) return false;
    _Static_assert(sizeof(float) == 4, "Requires IEEE binary32 floats");
    memcpy(out, &bits, sizeof(bits));
    return true;
}

MeleeHostBool melee_archive_relocation(const MeleeArchive *a, uint32_t index,
                              uint32_t *slot, uint32_t *target)
{
    if (!a || !a->bytes || !slot || !target || index >= a->reloc_count) return false;
    uint32_t s = be32(a->bytes + a->reloc_start + (size_t)index * 4), t;
    if (!melee_archive_u32(a, s, &t) || t > a->data_size) return false;
    *slot = s;
    *target = t;
    return true;
}

MeleeHostBool melee_archive_public(const MeleeArchive *a, uint32_t index,
                          const char **name, uint32_t *offset)
{
    if (!a || !a->bytes || !name || !offset || index >= a->public_count) return false;
    return symbol(a, a->public_start + (size_t)index * 8, name, offset);
}

MeleeHostBool melee_archive_find(const MeleeArchive *a, const char *name, uint32_t *offset)
{
    if (!a || !a->bytes || !name || !offset) return false;
    for (uint32_t i = 0; i < a->public_count; ++i) {
        const char *s;
        uint32_t o;
        if (!melee_archive_public(a, i, &s, &o)) return false;
        if (!strcmp(name, s)) { *offset = o; return true; }
    }
    return false;
}

MeleeHostBool melee_archive_open(MeleeArchive *out, const void *bytes, size_t size)
{
    if (!out) return false;
    memset(out, 0, sizeof(*out));
    if (!bytes || size < 32 || size > UINT32_MAX) return false;
    const uint8_t *p = bytes;
    if (be32(p) != size) return false;
    MeleeArchive a = {.bytes = p, .size = size, .data_size = be32(p + 4),
        .reloc_count = be32(p + 8), .public_count = be32(p + 12),
        .extern_count = be32(p + 16)};
    if (!table_end(32, a.data_size, 1, size, &a.reloc_start) ||
        !table_end(a.reloc_start, a.reloc_count, 4, size, &a.public_start) ||
        !table_end(a.public_start, a.public_count, 8, size, &a.extern_start) ||
        !table_end(a.extern_start, a.extern_count, 8, size, &a.strings_start)) return false;
    for (uint32_t i = 0; i < a.reloc_count; ++i) {
        uint32_t s, t;
        if (!melee_archive_relocation(&a, i, &s, &t)) return false;
    }
    for (uint32_t i = 0; i < a.public_count; ++i) {
        const char *name;
        uint32_t offset;
        if (!melee_archive_public(&a, i, &name, &offset)) return false;
    }
    for (uint32_t i = 0; i < a.extern_count; ++i) {
        const char *name;
        uint32_t offset;
        if (!symbol(&a, a.extern_start + (size_t)i * 8, &name, &offset)) return false;
        /* External references form a chain of big-endian slot offsets.
         * Bound its traversal so a cycle cannot hang asset import. */
        uint32_t steps = 0;
        while (offset != UINT32_MAX) {
            if (++steps > a.data_size / 4 || (offset & 3) ||
                !melee_archive_u32(&a, offset, &offset)) return false;
        }
    }
    *out = a;
    return true;
}

/* A zero word is null only when its slot has no relocation. Offset zero can
 * itself be a valid referenced object. Unresolved external pointers fail. */
MeleeHostBool melee_archive_pointer(const MeleeArchive* a, uint32_t slot,
                            uint32_t* target, MeleeHostBool* present)
{
    if (!target || !present || !melee_archive_u32(a, slot, target)) return false;
    *present = false;
    for (uint32_t i = 0; i < a->extern_count; ++i) {
        const uint8_t* entry = a->bytes + a->extern_start + (size_t)i * 8;
        uint32_t external = ((uint32_t)entry[0] << 24) | ((uint32_t)entry[1] << 16) |
                            ((uint32_t)entry[2] << 8) | entry[3];
        uint32_t steps = 0;
        while (external != UINT32_MAX) {
            if (external == slot || ++steps > a->data_size / 4 ||
                !melee_archive_u32(a, external, &external)) return false;
        }
    }
    for (uint32_t i = 0; i < a->reloc_count; ++i) {
        uint32_t s, t;
        if (!melee_archive_relocation(a, i, &s, &t)) return false;
        if (s == slot) { *target = t; *present = true; return true; }
    }
    return *target == 0;
}


uint8_t* melee_archive_copy_null_externals(const MeleeArchive* source,size_t* size)
{
    MeleeArchive a;
    if(!source||!size||!melee_archive_open(&a,source->bytes,source->size))return NULL;
    size_t removed=(size_t)a.extern_count*8,new_size=a.size-removed;
    uint8_t* bytes=malloc(new_size);if(!bytes)return NULL;
    memcpy(bytes,a.bytes,a.extern_start);
    memcpy(bytes+a.extern_start,a.bytes+a.strings_start,a.size-a.strings_start);
    for(unsigned i=0;i<4;i++)bytes[i]=(uint8_t)(new_size>>(24-8*i));
    memset(bytes+16,0,4);
    for(uint32_t i=0;i<a.extern_count;i++){
        uint32_t at=be32(a.bytes+a.extern_start+(size_t)i*8),next;
        while(at!=UINT32_MAX){
            if(!melee_archive_u32(&a,at,&next)){free(bytes);return NULL;}
            memset(bytes+32+at,0,4);at=next;
        }
    }
    MeleeArchive checked;if(!melee_archive_open(&checked,bytes,new_size)){free(bytes);return NULL;}
    *size=new_size;return bytes;
}

struct MeleeArchiveStorage { _Atomic size_t references; uint8_t* bytes; size_t size; };
MeleeHostBool melee_archive_adopt(MeleeArchive* out,uint8_t* bytes,size_t size)
{
    if(!melee_archive_open(out,bytes,size))return false;
    struct MeleeArchiveStorage* storage=malloc(sizeof(*storage));
    if(!storage){memset(out,0,sizeof(*out));return false;}
    atomic_init(&storage->references,1);storage->bytes=bytes;storage->size=size;
    out->storage=storage;return true;
}
MeleeHostBool melee_archive_acquire(MeleeArchive* out,const MeleeArchive* input)
{
    if(!out||!input||out==input)return false;
    if(input->storage&&input->bytes==input->storage->bytes&&input->size==input->storage->size){
        if(!melee_archive_open(out,input->bytes,input->size))return false;
        atomic_fetch_add_explicit(&input->storage->references,1,memory_order_relaxed);
        out->storage=input->storage;return true;
    }
    memset(out,0,sizeof(*out));
    if(!input->bytes||!input->size)return false;
    uint8_t* copy=malloc(input->size);if(!copy)return false;
    memcpy(copy,input->bytes,input->size);
    if(!melee_archive_adopt(out,copy,input->size)){free(copy);return false;}
    return true;
}
void melee_archive_release(MeleeArchive* owned)
{
    if(!owned)return;
    struct MeleeArchiveStorage* storage=owned->storage;
    memset(owned,0,sizeof(*owned));
    if(storage&&atomic_fetch_sub_explicit(&storage->references,1,memory_order_acq_rel)==1){
        free(storage->bytes);free(storage);
    }
}
