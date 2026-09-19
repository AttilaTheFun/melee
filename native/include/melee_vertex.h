#ifndef MELEE_NATIVE_VERTEX_H
#define MELEE_NATIVE_VERTEX_H
#include "melee_host_types.h"
#include <stddef.h>
#include <stdint.h>
/* Values are the GX enum values. Array bytes and display lists are big-endian.
 * Array bounds must cover the referenced data, not merely one element. */
typedef struct {
    uint32_t attribute, mode, components, format;
    uint8_t fraction;
    size_t stride;
    const uint8_t* array;
    size_t array_size;
} MeleeVertexAttribute;
typedef struct {
    float position[3], normal[3][3], texcoord[8][2];
    uint8_t color[2][4], matrix_index[9];
} MeleeVertex;
typedef struct {
    uint8_t primitive, vat;
    size_t first, count;
} MeleeDraw;
typedef struct MeleeVertexBatch MeleeVertexBatch;
/* Decode fixed-format draw commands and NOP padding. State commands, nested
 * display lists and position sentinel indices fail; none are silently skipped.
 * Returned vertices/draws own all values. Source arrays may then be released. */
MeleeVertexBatch* melee_vertex_decode(const uint8_t* bytes, size_t size,
    const MeleeVertexAttribute* attributes, size_t count, uint8_t vat);
void melee_vertex_free(MeleeVertexBatch* batch);
size_t melee_vertex_count(const MeleeVertexBatch* batch);
size_t melee_draw_count(const MeleeVertexBatch* batch);
/* Canonical GX attribute bits 0..20; NBT uses the normal bit (10). */
/* Minimum indexed-array prefix read by this display list; zero for direct or
 * unused attributes. NBT maps to the normal array. */
size_t melee_vertex_array_bytes(const MeleeVertexBatch* batch,uint32_t attribute);
uint32_t melee_vertex_attribute_mask(const MeleeVertexBatch* batch);
const MeleeVertex* melee_vertex_data(const MeleeVertexBatch* batch);
const MeleeDraw* melee_draw_data(const MeleeVertexBatch* batch);
#endif
