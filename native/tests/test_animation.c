#include "melee_animation.h"
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void be32(uint8_t* p, uint32_t v)
{ p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
static void lefloat(uint8_t* p, float v)
{
    uint32_t bits; memcpy(&bits, &v, 4);
    p[0] = bits; p[1] = bits >> 8; p[2] = bits >> 16; p[3] = bits >> 24;
}
static void update(void* object, int type, HSD_ObjData* value)
{ ((float*)object)[type] = value->fv; }
static uint8_t dat[128];
static void fixture(void)
{
    memset(dat, 0, sizeof(dat));
    be32(dat, sizeof(dat)); be32(dat + 4, 80); be32(dat + 8, 4);
    uint8_t* d = dat + 32;
    be32(d, 20); be32(d + 4, 11); d[12] = 0; be32(d + 16, 40);
    be32(d + 24, 7); d[32] = 1; d[33] = HSD_A_FRAC_S16 | 1; be32(d + 36, 64);
    d[40] = 0x12; lefloat(d + 41, -2); d[45] = 10;
    lefloat(d + 46, -12); d[50] = 10;
    d[64] = 0x12; d[65] = 0xf8; d[66] = 0xff; d[67] = 10;
    d[68] = 8; d[69] = 0; d[70] = 10;
    be32(d + 80, 0); be32(d + 84, 16); be32(d + 88, 36);
    /* Fourth relocation is a duplicate, legal to the archive reader. */
    be32(d + 92, 36);
}
static void check_format(uint8_t format, const uint8_t* encoded, unsigned width, float expected)
{
    uint8_t stream[11] = {0x11};
    memcpy(stream + 1, encoded, width); stream[1 + width] = 1;
    memcpy(stream + 2 + width, encoded, width); stream[2 + 2 * width] = 1;
    HSD_FObjDesc desc = {.length = 3 + 2 * width, .frac_value = format, .ad = stream};
    assert(melee_animation_stream_valid(stream, desc.length, format, 0));
    HSD_FObj* track = HSD_FObjLoadDesc(&desc);
    float result = 123;
    HSD_FObjReqAnimAll(track, 0);
    HSD_FObjInterpretAnim(track, &result, update, 0);
    assert(result == expected);
    HSD_FObjRemoveAll(track);
}

static void test_fighter_tree(void)
{
    uint8_t file[108] = {0};
    be32(file, sizeof(file)); be32(file + 4, 64); be32(file + 8, 3);
    uint8_t* d = file + 32;
    be32(d, 1); be32(d + 8, 0x41200000); be32(d + 12, 20); be32(d + 16, 24);
    d[20] = 1; d[21] = 255;
    d[25] = 5; d[28] = 1; d[29] = HSD_A_FRAC_U8; be32(d + 32, 36);
    memcpy(d + 36, (uint8_t[]){0x12, 0, 10, 10, 10}, 5);
    be32(d + 64, 12); be32(d + 68, 16); be32(d + 72, 32);
    MeleeArchive archive;
    assert(melee_archive_open(&archive, file, sizeof(file)));
    MeleeFighterAnimation* owner = melee_fighter_animation_decode(&archive, 0);
    assert(owner && melee_fighter_animation_joint_count(owner) == 1);
    FigaTree* tree = melee_fighter_animation_tree(owner);
    assert(tree->type == 1 && tree->frames == 10 && tree->nodes[0] == 1 && tree->nodes[1] == -1);
    assert(tree->tracks[0].ad_head != d + 36 && tree->tracks[0].length == 5);
    HSD_FObjDesc desc = {.length = tree->tracks[0].length, .type = tree->tracks[0].obj_type,
                        .frac_value = tree->tracks[0].frac_value, .ad = tree->tracks[0].ad_head};
    HSD_FObj* track = HSD_FObjLoadDesc(&desc);
    float values[2] = {0};
    HSD_FObjReqAnimAll(track, 0); HSD_FObjInterpretAnim(track, values, update, 0);
    HSD_FObjInterpretAnim(track, values, update, 5);
    assert(values[1] == 5);
    HSD_FObjRemoveAll(track); melee_fighter_animation_free(owner);
    d[20] = 128; assert(!melee_fighter_animation_decode(&archive, 0));
    d[20] = 127; assert(!melee_fighter_animation_decode(&archive, 0));
    d[20] = 1; d[25] = 100; assert(!melee_fighter_animation_decode(&archive, 0));
    d[25] = 5; be32(d + 72, 12); assert(!melee_fighter_animation_decode(&archive, 0));
}

int main(void)
{
    static _Alignas(32) uint8_t arena[65536];
    assert(OSInitAlloc(arena, arena + sizeof(arena), 1));
    HSD_SetHeap(OSCreateHeap(arena, arena + sizeof(arena)));
    HSD_FObjInitAllocData();
    test_fighter_tree();
    check_format(HSD_A_FRAC_S8 | 2, (uint8_t[]){0xf0}, 1, -4);
    check_format(HSD_A_FRAC_U8 | 1, (uint8_t[]){250}, 1, 125);
    check_format(HSD_A_FRAC_U16 | 3, (uint8_t[]){0x00, 0x80}, 2, 4096);
    check_format(HSD_A_FRAC_S16, (uint8_t[]){0x00, 0x80}, 2, -32768);
    /* Preserve the original signed divisor even for fraction exponent 31. */
    check_format(HSD_A_FRAC_U8 | 31, (uint8_t[]){1}, 1, -0x1p-31f);
    uint8_t empty_byte = 0;
    HSD_FObjDesc empty = {.ad = &empty_byte};
    HSD_FObj* empty_track = HSD_FObjLoadDesc(&empty);
    float untouched = 123;
    HSD_FObjReqAnimAll(empty_track, 0);
    HSD_FObjInterpretAnim(empty_track, &untouched, update, 0);
    assert(untouched == 123);
    HSD_FObjRemoveAll(empty_track);
    uint8_t cubic[] = {0x14, 0, 0, 10, 10, 0, 10};
    HSD_FObjDesc spline = {.length = sizeof(cubic), .frac_value = HSD_A_FRAC_U8,
                          .frac_slope = HSD_A_FRAC_S8, .ad = cubic};
    assert(melee_animation_stream_valid(cubic, sizeof(cubic), spline.frac_value, spline.frac_slope));
    HSD_FObj* spline_track = HSD_FObjLoadDesc(&spline);
    float interpolated = 0;
    HSD_FObjReqAnimAll(spline_track, 0);
    HSD_FObjInterpretAnim(spline_track, &interpolated, update, 0);
    HSD_FObjInterpretAnim(spline_track, &interpolated, update, 2.5f);
    assert(fabsf(interpolated - 1.5625f) < 1e-6f);
    HSD_FObjRemoveAll(spline_track);
    fixture();
    MeleeArchive archive;
    assert(melee_archive_open(&archive, dat, sizeof(dat)));
    MeleeAnimationTracks* owner = melee_animation_tracks_decode(&archive, 0);
    assert(owner);
    HSD_FObjDesc* desc = melee_animation_tracks_descriptors(owner);
    assert(desc && desc->next && !desc->next->next);
    assert(desc->ad != dat + 72 && desc->length == 11);
    HSD_FObj* tracks = HSD_FObjLoadDesc(desc);
    memset(dat, 0, sizeof(dat)); /* decoded tracks own their bytecode */
    float values[2] = {99, 99};
    HSD_FObjReqAnimAll(tracks, 0);
    HSD_FObjInterpretAnimAll(tracks, values, update, 0);
    assert(values[0] == -2 && values[1] == -4);
    HSD_FObjInterpretAnimAll(tracks, values, update, 5);
    assert(fabsf(values[0] + 7) < 1e-6f && fabsf(values[1]) < 1e-6f);
    HSD_FObjInterpretAnimAll(tracks, values, update, 5);
    assert(values[0] == -12 && values[1] == 4);
    HSD_FObjReqAnimAll(tracks, 0);
    HSD_FObjInterpretAnimAll(tracks, values, update, 0);
    assert(values[0] == -2 && values[1] == -4);
    HSD_FObjRemoveAll(tracks);
    melee_animation_tracks_free(owner);
    assert(HSD_FObjGetAllocData()->used == 0);
    fixture();
    assert(melee_archive_open(&archive, dat, sizeof(dat)));
    be32(dat + 32 + 92, 20); /* relocated zero points back to first descriptor */
    assert(!melee_animation_tracks_decode(&archive, 0));
    fixture(); be32(dat + 32 + 4, 100);
    assert(!melee_animation_tracks_decode(&archive, 0));
    fixture(); be32(dat + 32 + 8, 0x7fc00000);
    assert(!melee_animation_tracks_decode(&archive, 0));
    fixture(); dat[32 + 40] = 15;
    assert(!melee_animation_tracks_decode(&archive, 0));
    fixture(); be32(dat + 32 + 84, 36); /* data pointer without relocation */
    assert(!melee_animation_tracks_decode(&archive, 0));
    fixture();
    uint8_t external[160] = {0};
    memcpy(external, dat, 112);
    be32(external, sizeof(external)); be32(external + 8, 2); be32(external + 16, 1);
    be32(external + 112, 16); be32(external + 116, 36);
    be32(external + 120, 20); memcpy(external + 128, "external", 9);
    be32(external + 32, UINT32_MAX); /* external chain: slot 20 -> slot 0 -> end */
    assert(melee_archive_open(&archive, external, sizeof(external)));
    assert(!melee_animation_tracks_decode(&archive, 20));
    uint8_t packed[20] = {0x82, 1}; /* nine one-byte linear values/waits */
    for (int i = 0; i < 9; ++i) { packed[2 + i * 2] = i; packed[3 + i * 2] = 1; }
    assert(melee_animation_stream_valid(packed, sizeof(packed), HSD_A_FRAC_U8, 0));
    assert(!melee_animation_stream_valid((uint8_t[]){2, 0}, 2, 0, 0));
    assert(!melee_animation_stream_valid((uint8_t[]){2, 1, 128}, 3, HSD_A_FRAC_U8, 0));
    assert(!melee_animation_stream_valid((uint8_t[]){0x12, 1}, 2, HSD_A_FRAC_U8, 0));
    assert(!melee_animation_stream_valid((uint8_t[]){0x82, 128, 128, 128}, 4, HSD_A_FRAC_U8, 0));
    assert(melee_animation_stream_valid((uint8_t[]){4, 1, 2, 5, 5, 3}, 6, HSD_A_FRAC_U8, HSD_A_FRAC_S8));
    puts("DAT animation descriptors and original ARM track interpolation passed");
}
