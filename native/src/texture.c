#include "melee_texture.h"
#include <string.h>

typedef struct { unsigned width, height, bytes; } Tile;
static Tile tile(uint32_t format)
{
    switch (format) {
    case 0: case 8: case 14: return (Tile){8, 8, 32};
    case 1: case 2: case 9: return (Tile){8, 4, 32};
    case 3: case 4: case 5: case 10: return (Tile){4, 4, 32};
    case 6: return (Tile){4, 4, 64};
    default: return (Tile){0};
    }
}
size_t melee_texture_level_size(uint16_t width, uint16_t height, uint32_t format)
{
    Tile t = tile(format);
    if (!width || !height || !t.bytes) return 0;
    size_t columns = ((size_t)width + t.width - 1) / t.width;
    size_t rows = ((size_t)height + t.height - 1) / t.height;
    if (columns > SIZE_MAX / rows / t.bytes) return 0;
    return columns * rows * t.bytes;
}
static uint16_t be16(const uint8_t* p) { return ((uint16_t)p[0] << 8) | p[1]; }
static uint8_t bits5(unsigned v) { return (v << 3) | (v >> 2); }
static uint8_t bits6(unsigned v) { return (v << 2) | (v >> 4); }
static void rgb565(uint16_t word, uint8_t* p)
{
    p[0] = bits5(word >> 11); p[1] = bits6((word >> 5) & 63);
    p[2] = bits5(word & 31); p[3] = 255;
}
static void rgb5a3(uint16_t word, uint8_t* p)
{
    if (word & 0x8000) {
        p[0] = bits5((word >> 10) & 31); p[1] = bits5((word >> 5) & 31);
        p[2] = bits5(word & 31); p[3] = 255;
    } else {
        unsigned alpha = word >> 12;
        p[0] = ((word >> 8) & 15) * 17; p[1] = ((word >> 4) & 15) * 17;
        p[2] = (word & 15) * 17; p[3] = (alpha << 5) | (alpha << 2) | (alpha >> 1);
    }
}
static void cmpr(const uint8_t* block, unsigned x, unsigned y, uint8_t* p)
{
    const uint8_t* sub = block + ((y / 4) * 2 + x / 4) * 8;
    uint16_t a = be16(sub), b = be16(sub + 2);
    unsigned index = (sub[4 + y % 4] >> (6 - (x % 4) * 2)) & 3;
    uint8_t first[4], second[4]; rgb565(a, first); rgb565(b, second);
    if (index < 2) memcpy(p, index ? second : first, 4);
    else {
        for (unsigned c = 0; c < 3; ++c) {
            /* GX uses 5/8 and 3/8, rather than desktop BC1's thirds. */
            if (a > b) p[c] = index == 2 ? (5 * first[c] + 3 * second[c]) / 8 :
                                                        (3 * first[c] + 5 * second[c]) / 8;
            else p[c] = (first[c] + second[c]) / 2;
        }
        /* Transparent GX CMPR retains the averaged RGB components. */
        p[3] = a <= b && index == 3 ? 0 : 255;
    }
}
MeleeHostBool melee_texture_decode(const void* data, size_t data_size,
    uint16_t width, uint16_t height, uint32_t format,
    const void* palette, size_t palette_size, uint32_t palette_format,
    void* rgba, size_t rgba_size)
{
    size_t needed = melee_texture_level_size(width, height, format);
    if (!data || !rgba || !needed || data_size < needed ||
        (size_t)width > SIZE_MAX / height / 4 || rgba_size < (size_t)width * height * 4) return false;
    MeleeHostBool indexed = format == 8 || format == 9 || format == 10;
    if (indexed && (!palette || palette_size < 2 || (palette_size & 1) || palette_format > 2)) return false;
    Tile t = tile(format);
    size_t columns = ((size_t)width + t.width - 1) / t.width;
    for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) {
        unsigned local_x = x % t.width, local_y = y % t.height;
        unsigned pixel = local_y * t.width + local_x;
        const uint8_t* block = (const uint8_t*)data + ((y / t.height) * columns + x / t.width) * t.bytes;
        uint8_t* p = (uint8_t*)rgba + ((size_t)y * width + x) * 4;
        if (indexed) {
            unsigned index = format == 8 ? ((block[pixel / 2] >> (pixel % 2 ? 0 : 4)) & 15) :
                             format == 9 ? block[pixel] : be16(block + pixel * 2) & 16383;
            if (index >= palette_size / 2) return false;
            const uint8_t* entry = (const uint8_t*)palette + index * 2;
            if (palette_format == 0) { p[0] = p[1] = p[2] = entry[1]; p[3] = entry[0]; }
            else if (palette_format == 1) rgb565(be16(entry), p);
            else rgb5a3(be16(entry), p);
            continue;
        }
        switch (format) {
        case 0: memset(p, ((block[pixel / 2] >> (pixel % 2 ? 0 : 4)) & 15) * 17, 4); break;
        case 1: memset(p, block[pixel], 4); break;
        case 2: p[0] = p[1] = p[2] = (block[pixel] & 15) * 17; p[3] = (block[pixel] >> 4) * 17; break;
        case 3: p[0] = p[1] = p[2] = block[pixel * 2 + 1]; p[3] = block[pixel * 2]; break;
        case 4: rgb565(be16(block + pixel * 2), p); break;
        case 5: rgb5a3(be16(block + pixel * 2), p); break;
        case 6: p[0] = block[pixel * 2 + 1]; p[1] = block[32 + pixel * 2];
                p[2] = block[33 + pixel * 2]; p[3] = block[pixel * 2]; break;
        case 14: cmpr(block, local_x, local_y, p); break;
        }
    }
    return true;
}
