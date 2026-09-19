#include "melee_archive.h"
#include "melee_texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static MeleeArchive archive;
static uint8_t* visited;
static unsigned count, formats[15];
static _Noreturn void fail(void) { fprintf(stderr, "Invalid or unsupported texture graph\n"); exit(1); }
static uint32_t word(uint32_t offset)
{ uint32_t value; if (!melee_archive_u32(&archive, offset, &value)) fail(); return value; }
static uint32_t pointer(uint32_t offset)
{
    uint32_t value = word(offset);
    for (uint32_t i = 0; i < archive.reloc_count; ++i) {
        uint32_t slot, target;
        if (!melee_archive_relocation(&archive, i, &slot, &target)) fail();
        if (slot == offset) return target;
    }
    if (value) fail();
    return UINT32_MAX;
}
static int seen(uint32_t offset, unsigned kind, unsigned bytes)
{
    if (offset == UINT32_MAX) return 1;
    if ((offset & 3) || offset > archive.data_size || bytes > archive.data_size - offset) fail();
    uint8_t mask = (uint8_t)(1 << kind);
    if (visited[offset / 4] & mask) return 1;
    visited[offset / 4] |= mask;
    return 0;
}
static void texture(uint32_t offset)
{
    while (!seen(offset, 2, 92)) {
        uint32_t desc = pointer(offset + 76), paldesc = pointer(offset + 80);
        if (desc == UINT32_MAX || desc > archive.data_size || archive.data_size - desc < 24) fail();
        uint32_t data = pointer(desc), dimensions = word(desc + 4), format = word(desc + 8);
        uint16_t width = dimensions >> 16, height = dimensions;
        const uint8_t* palette = NULL; size_t palette_size = 0; uint32_t palette_format = 0;
        if (paldesc != UINT32_MAX) {
            if (paldesc > archive.data_size || archive.data_size - paldesc < 16) fail();
            uint32_t pal = pointer(paldesc);
            palette_format = word(paldesc + 4); palette_size = (word(paldesc + 12) >> 16) * 2;
            if (pal > archive.data_size || palette_size > archive.data_size - pal) fail();
            palette = archive.bytes + 32 + pal;
        }
        if (data > archive.data_size) fail();
        unsigned levels = 1;
        if (word(desc + 12)) {
            float lod;
            if (!melee_archive_f32(&archive, desc + 20, &lod) || !isfinite(lod) || lod < 0 || lod > 16) fail();
            levels += (unsigned)lod;
        }
        for (unsigned level = 0; level < levels; ++level) {
            size_t size = melee_texture_level_size(width, height, format);
            size_t rgba_size = (size_t)width * height * 4;
            uint8_t* rgba = malloc(rgba_size);
            if (!rgba || !size || size > archive.data_size - data ||
                !melee_texture_decode(archive.bytes + 32 + data, size, width, height, format,
                    palette, palette_size, palette_format, rgba, rgba_size)) fail();
            free(rgba); data += size;
            if (width > 1) width /= 2;
            if (height > 1) height /= 2;
        }
        ++count; ++formats[format];
        offset = pointer(offset + 4);
    }
}
static void drawable(uint32_t offset)
{
    while (!seen(offset, 1, 16)) {
        uint32_t material = pointer(offset + 8);
        if (material != UINT32_MAX) {
            if (material > archive.data_size || archive.data_size - material < 24) fail();
            texture(pointer(material + 8));
        }
        offset = pointer(offset + 4);
    }
}
static void joint(uint32_t offset)
{
    while (!seen(offset, 0, 64)) {
        uint32_t flags = word(offset + 4);
        if (!(flags & ((1 << 5) | (1 << 14)))) drawable(pointer(offset + 16));
        joint(pointer(offset + 8));
        offset = pointer(offset + 12);
    }
}
int main(int argc, char** argv)
{
    if (argc != 3) { fprintf(stderr, "Usage: verify-textures ARCHIVE JOINT_SYMBOL\n"); return 2; }
    FILE* f = fopen(argv[1], "rb"); if (!f) return 1;
    if (fseek(f, 0, SEEK_END)) return 1;
    long size = ftell(f); rewind(f);
    uint8_t* bytes = size > 0 ? malloc(size) : NULL;
    if (!bytes || fread(bytes, 1, size, f) != (size_t)size) return 1;
    fclose(f);
    uint32_t root;
    if (!melee_archive_open(&archive, bytes, size) || !melee_archive_find(&archive, argv[2], &root)) fail();
    visited = calloc((size_t)archive.data_size / 4 + 1, 1); if (!visited) fail();
    joint(root);
    printf("Decoded %u material textures including their mip levels\n", count);
    for (unsigned i = 0; i < 15; ++i) if (formats[i]) printf("GX format %u: %u textures\n", i, formats[i]);
    free(visited); free(bytes); return 0;
}
