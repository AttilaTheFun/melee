#ifndef MELEE_NATIVE_TEXTURE_H
#define MELEE_NATIVE_TEXTURE_H
#include "melee_host_types.h"
#include <stddef.h>
#include <stdint.h>
/* GX format numbers match the SDK. Returns zero for invalid dimensions/format.
 * Sizes include complete GX tiles, even when the last tile is only partly used. */
size_t melee_texture_level_size(uint16_t width, uint16_t height, uint32_t format);
/* Decode one mip level to tightly packed, straight-alpha RGBA8. Input remains
 * in its original tiled, big-endian disk representation. Palette entries are
 * big-endian 16-bit GX TLUT values (format 0=IA8, 1=RGB565, 2=RGB5A3).
 * Source, palette and output must not overlap. Output is unspecified on error. */
MeleeHostBool melee_texture_decode(const void* data, size_t data_size,
    uint16_t width, uint16_t height, uint32_t format,
    const void* palette, size_t palette_size, uint32_t palette_format,
    void* rgba, size_t rgba_size);
#endif
