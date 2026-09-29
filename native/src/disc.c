#include "melee_disc.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

struct MeleeDisc {
    int fd;
    MeleeDiscReader reader;
    void* reader_context;
    uint64_t size;
    uint8_t id[32];
    uint8_t* fst;
    MeleeDiscEntry* entries;
    uint32_t count;
    uint32_t block_size;
    uint32_t* block_map;
};
static uint32_t be32(const uint8_t* p)
{ return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
static MeleeHostBool read_at(const MeleeDisc* disc, void* out, size_t length, uint64_t offset)
{
    if (disc->reader) return disc->reader(disc->reader_context, out, length, offset);
    int fd = disc->fd;
    uint8_t* p = out;
    while (length) {
        size_t chunk = length > INT_MAX ? INT_MAX : length;
        ssize_t n = pread(fd, p, chunk, (off_t)offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        length -= (size_t)n; p += n; offset += (size_t)n;
    }
    return true;
}
MeleeHostBool melee_disc_read_at(const MeleeDisc* d, void* output, size_t length, uint64_t offset)
{
    if (!d || (!output && length) || offset > d->size || length > d->size - offset) return false;
    if (!d->block_size) return read_at(d, output, length, offset);
    uint8_t* out = output;
    while (length) {
        uint32_t block = (uint32_t)(offset / d->block_size);
        uint32_t within = offset % d->block_size;
        size_t amount = d->block_size - within;
        if (amount > length) amount = length;
        uint32_t stored = d->block_map[block];
        if (stored == UINT32_MAX) memset(out, 0, amount);
        else if (!read_at(d, out, amount, 32768 + (uint64_t)stored * d->block_size + within)) return false;
        out += amount; offset += amount; length -= amount;
    }
    return true;
}
void melee_disc_close(MeleeDisc* disc)
{
    if (!disc) return;
    if (disc->fd >= 0) close(disc->fd); free(disc->block_map); free(disc->fst); free(disc->entries); free(disc);
}
static MeleeDisc* parse_disc(MeleeDisc* d, uint64_t physical_size)
{
    uint8_t boot[0x440];
    d->size = physical_size;
    if (physical_size < sizeof(boot) || !read_at(d, boot, 8, 0)) goto fail;
    if (!memcmp(boot, "CISO", 4)) {
        /* GameCube CISO: little-endian block size, 32760 presence bytes,
         * then contiguous stored blocks. Absent logical blocks read as zero. */
        uint8_t header[32768];
        if (!read_at(d, header, sizeof(header), 0)) goto fail;
        d->block_size = header[4] | ((uint32_t)header[5] << 8) |
                        ((uint32_t)header[6] << 16) | ((uint32_t)header[7] << 24);
        if (!d->block_size) goto fail;
        d->block_map = malloc(32760 * sizeof(*d->block_map));
        if (!d->block_map) goto fail;
        uint32_t stored = 0;
        for (uint32_t i = 0; i < 32760; ++i) {
            if (header[8 + i] > 1) goto fail;
            d->block_map[i] = header[8 + i] ? stored++ : UINT32_MAX;
        }
        if (32768 + (uint64_t)stored * d->block_size > physical_size) goto fail;
        d->size = (uint64_t)32760 * d->block_size;
    }
    if (!melee_disc_read_at(d, boot, sizeof(boot), 0) || be32(boot + 0x1c) != 0xc2339f3d) goto fail;
    memcpy(d->id, boot, 32);
    uint32_t offset = be32(boot + 0x424), size = be32(boot + 0x428);
    if (size < 12 || offset < sizeof(boot) || offset > d->size || size > d->size - offset) goto fail;
    d->fst = malloc(size);
    if (!d->fst || !melee_disc_read_at(d, d->fst, size, offset)) goto fail;
    d->count = be32(d->fst + 8);
    if (d->fst[0] != 1 || be32(d->fst + 4) || !d->count ||
        d->count > INT32_MAX || d->count > size / 12) goto fail;
    d->entries = calloc(d->count, sizeof(*d->entries));
    if (!d->entries) goto fail;
    d->entries[0] = (MeleeDiscEntry){.directory = true, .next = d->count, .name = ""};
    uint32_t parent = 0;
    size_t strings = (size_t)d->count * 12;
    for (uint32_t i = 1; i < d->count; ++i) {
        while (i >= d->entries[parent].next && parent) parent = d->entries[parent].parent;
        const uint8_t* p = d->fst + (size_t)i * 12;
        uint32_t name = be32(p) & 0xffffff;
        if (p[0] > 1 || name >= size - strings) goto fail;
        const char* text = (const char*)d->fst + strings + name;
        if (!memchr(text, 0, size - strings - name) || !*text ||
            strchr(text, '/') || !strcmp(text, ".") || !strcmp(text, "..")) goto fail;
        MeleeDiscEntry* e = &d->entries[i];
        *e = (MeleeDiscEntry){.parent = parent, .name = text, .directory = p[0] == 1,
                             .offset = be32(p + 4), .length = be32(p + 8), .next = i + 1};
        if (e->directory) {
            e->next = e->length;
            if (e->offset != parent || e->next <= i || e->next > d->entries[parent].next) goto fail;
            e->offset = e->length = 0;
            parent = i;
        } else if (e->offset > d->size || e->length > d->size - e->offset) goto fail;
    }
    return d;
fail:
    melee_disc_close(d);
    return NULL;
}
MeleeDisc* melee_disc_open(const char* path)
{
    if (!path) return NULL;
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return NULL;
    struct stat st;
    if (fstat(fd, &st) || !S_ISREG(st.st_mode) || st.st_size < 0) { close(fd); return NULL; }
    MeleeDisc* d = calloc(1, sizeof(*d));
    if (!d) { close(fd); return NULL; }
    d->fd = fd;
    return parse_disc(d, (uint64_t)st.st_size);
}
MeleeDisc* melee_disc_open_reader(uint64_t size, MeleeDiscReader reader, void* context)
{
    if (!reader) return NULL;
    MeleeDisc* d = calloc(1, sizeof(*d));
    if (!d) return NULL;
    d->fd = -1; d->reader = reader; d->reader_context = context;
    return parse_disc(d, size);
}
uint32_t melee_disc_entry_count(const MeleeDisc* d) { return d ? d->count : 0; }
const MeleeDiscEntry* melee_disc_entry(const MeleeDisc* d, uint32_t n)
{ return d && n < d->count ? &d->entries[n] : NULL; }
const uint8_t* melee_disc_id(const MeleeDisc* d) { return d ? d->id : NULL; }
int32_t melee_disc_find(const MeleeDisc* d, uint32_t directory, const char* path)
{
    if (!d || !path || directory >= d->count || !d->entries[directory].directory) return -1;
    uint32_t current = *path == '/' ? 0 : directory;
    while (*path) {
        while (*path == '/') ++path;
        if (!*path) break;
        const char* end = strchr(path, '/');
        size_t length = end ? (size_t)(end - path) : strlen(path);
        if (!d->entries[current].directory) return -1;
        if (length == 1 && path[0] == '.') {}
        else if (length == 2 && path[0] == '.' && path[1] == '.') current = d->entries[current].parent;
        else {
            uint32_t candidate = current + 1;
            for (; candidate < d->entries[current].next; candidate = d->entries[candidate].next) {
                const char* name = d->entries[candidate].name;
                if (strlen(name) == length && !strncasecmp(name, path, length)) break;
            }
            if (candidate >= d->entries[current].next) return -1;
            current = candidate;
        }
        if (end && !d->entries[current].directory) return -1;
        path += length;
    }
    return (int32_t)current;
}
MeleeHostBool melee_disc_read(const MeleeDisc* d, uint32_t entry, void* out, size_t length, uint32_t offset)
{
    const MeleeDiscEntry* e = melee_disc_entry(d, entry);
    if (!e || e->directory || (!out && length) || offset > e->length ||
        length > (uint64_t)e->length + 31 - offset) return false;
    uint64_t absolute = (uint64_t)e->offset + offset;
    if (absolute > d->size || length > d->size - absolute) return false;
    return melee_disc_read_at(d, out, length, absolute);
}
