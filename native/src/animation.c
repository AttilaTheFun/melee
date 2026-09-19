#include "melee_animation.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    HSD_FObjDesc desc;
    uint32_t offset;
} Track;
struct MeleeAnimationTracks {
    Track* tracks;
    size_t count;
};

static unsigned component_size(uint8_t format)
{
    if (!format) return 4;
    switch (format & 0xe0) {
    case HSD_A_FRAC_S16: case HSD_A_FRAC_U16: return 2;
    case HSD_A_FRAC_S8: case HSD_A_FRAC_U8: return 1;
    default: return 0;
    }
}

/* The bytecode packs low-order groups first, unlike the DAT descriptors. */
static MeleeHostBool variable(const uint8_t* bytes, size_t length, size_t* cursor,
                             uint32_t initial, unsigned shift, uint32_t limit,
                             uint32_t* result)
{
    uint32_t value = initial;
    for (;;) {
        if (*cursor >= length || shift >= 32) return false;
        unsigned byte = bytes[(*cursor)++], part = byte & 127;
        if (part > (limit - value) >> shift) return false;
        value += (uint32_t)part << shift;
        if (!(byte & 128)) { *result = value; return true; }
        shift += 7;
    }
}

MeleeHostBool melee_animation_stream_valid(const uint8_t* bytes, size_t length,
                                           uint8_t value_format, uint8_t slope_format)
{
    unsigned value_size = component_size(value_format);
    unsigned slope_size = component_size(slope_format);
    if ((!bytes && length) || !value_size || !slope_size) return false;
    size_t cursor = 0;
    while (cursor < length) {
        unsigned command = bytes[cursor++], op = command & 15;
        uint32_t count = ((command >> 4) & 7) + 1;
        if (op < HSD_A_OP_CON || op > HSD_A_OP_KEY) return false;
        if ((command & 128) && !variable(bytes, length, &cursor, count, 3,
                                        UINT16_MAX, &count)) return false;
        unsigned width = op == HSD_A_OP_SLP ? slope_size : value_size;
        if (op == HSD_A_OP_SPL) width += slope_size;
        for (uint32_t i = 0; i < count; ++i) {
            if (width > length - cursor) return false;
            cursor += width;
            if (op == HSD_A_OP_SLP) continue;
            /* The last value may terminate without a trailing duration. */
            if (cursor == length) return i + 1 == count;
            uint32_t wait;
            if (!variable(bytes, length, &cursor, 0, 0, UINT16_MAX, &wait)) return false;
        }
    }
    return true;
}


void melee_animation_tracks_free(MeleeAnimationTracks* owner)
{
    if (!owner) return;
    for (size_t i = 0; i < owner->count; ++i) free(owner->tracks[i].desc.ad);
    free(owner->tracks);
    free(owner);
}

HSD_FObjDesc* melee_animation_tracks_descriptors(MeleeAnimationTracks* owner)
{
    return owner && owner->count ? &owner->tracks[0].desc : NULL;
}

MeleeAnimationTracks* melee_animation_tracks_decode(const MeleeArchive* a, uint32_t offset)
{
    if (!a || !a->bytes) return NULL;
    MeleeAnimationTracks* owner = calloc(1, sizeof(*owner));
    if (!owner) return NULL;
    for (;;) {
        if ((offset & 3) || offset > a->data_size || a->data_size - offset < 20) goto fail;
        for (size_t i = 0; i < owner->count; ++i)
            if (owner->tracks[i].offset == offset) goto fail;
        uint32_t next, data, length;
        float frame;
        MeleeHostBool has_next, has_data;
        if (!melee_archive_pointer(a, offset, &next, &has_next) ||
            !melee_archive_pointer(a, offset + 16, &data, &has_data) ||
            !melee_archive_u32(a, offset + 4, &length) ||
            !melee_archive_f32(a, offset + 8, &frame) ||
            !isfinite(frame) || frame < INT16_MIN || frame > INT16_MAX ||
            (length && !has_data) || data > a->data_size || length > a->data_size - data) goto fail;
        const uint8_t* fields = a->bytes + 32 + offset + 12;
        const uint8_t* stream = a->bytes + 32 + data;
        if (!melee_animation_stream_valid(stream, length, fields[1], fields[2])) goto fail;
        Track* grown = realloc(owner->tracks, (owner->count + 1) * sizeof(*grown));
        if (!grown) goto fail;
        owner->tracks = grown;
        Track* track = &grown[owner->count++];
        *track = (Track){.offset = offset, .desc = {.length = length,
            .startframe = frame, .type = fields[0], .frac_value = fields[1],
            .frac_slope = fields[2], .dummy0 = fields[3]}};
        track->desc.ad = malloc(length ? length : 1);
        if (!track->desc.ad) goto fail;
        memcpy(track->desc.ad, stream, length);
        if (!has_next) break;
        offset = next;
    }
    for (size_t i = 1; i < owner->count; ++i)
        owner->tracks[i - 1].desc.next = &owner->tracks[i].desc;
    return owner;
fail:
    melee_animation_tracks_free(owner);
    return NULL;
}

struct MeleeFighterAnimation {
    FigaTree tree;
    size_t joint_count, track_count;
};
FigaTree* melee_fighter_animation_tree(MeleeFighterAnimation* a) { return a ? &a->tree : NULL; }
size_t melee_fighter_animation_joint_count(const MeleeFighterAnimation* a) { return a ? a->joint_count : 0; }
void melee_fighter_animation_free(MeleeFighterAnimation* a)
{
    if (!a) return;
    if (a->tree.tracks)
        for (size_t i = 0; i < a->track_count; ++i) free(a->tree.tracks[i].ad_head);
    free(a->tree.tracks); free(a->tree.nodes); free(a);
}
MeleeFighterAnimation* melee_fighter_animation_decode(const MeleeArchive* a, uint32_t offset)
{
    if (!a || !a->bytes || offset > a->data_size || a->data_size - offset < 20) return NULL;
    uint32_t type, flags, nodes, tracks;
    float frames;
    MeleeHostBool present_nodes, present_tracks;
    if (!melee_archive_u32(a, offset, &type) || !melee_archive_u32(a, offset + 4, &flags) ||
        !melee_archive_f32(a, offset + 8, &frames) || !isfinite(frames) || frames < 0 ||
        !melee_archive_pointer(a, offset + 12, &nodes, &present_nodes) || !present_nodes ||
        !melee_archive_pointer(a, offset + 16, &tracks, &present_tracks)) return NULL;
    MeleeFighterAnimation* owner = calloc(1, sizeof(*owner));
    if (!owner) return NULL;
    owner->tree.type = (int32_t)type; owner->tree.flags = flags; owner->tree.frames = frames;
    const uint8_t* data = a->bytes + 32;
    size_t end = nodes;
    while (end < a->data_size && data[end] != 255) {
        if (data[end] > 127) goto fail;
        owner->track_count += data[end++];
    }
    if (end == a->data_size || (owner->track_count && !present_tracks) || tracks > a->data_size ||
        owner->track_count > (a->data_size - tracks) / 12) goto fail;
    owner->joint_count = end - nodes;
    owner->tree.nodes = malloc(owner->joint_count + 1);
    if (!owner->tree.nodes) goto fail;
    memcpy(owner->tree.nodes, data + nodes, owner->joint_count + 1);
    owner->tree.tracks = calloc(owner->track_count ? owner->track_count : 1, sizeof(FigaTrack));
    if (!owner->tree.tracks) goto fail;
    for (size_t i = 0; i < owner->track_count; ++i) {
        uint32_t location = tracks + (uint32_t)i * 12, stream;
        MeleeHostBool present;
        const uint8_t* p = data + location;
        FigaTrack* t = &owner->tree.tracks[i];
        t->length = ((u16)p[0] << 8) | p[1];
        t->startframe = ((u16)p[2] << 8) | p[3];
        t->obj_type = p[4]; t->frac_value = p[5]; t->frac_slope = p[6];
        if (!melee_archive_pointer(a, location + 8, &stream, &present) || (t->length && !present) ||
            stream > a->data_size || t->length > a->data_size - stream ||
            !melee_animation_stream_valid(data + stream, t->length, t->frac_value, t->frac_slope)) goto fail;
        t->ad_head = malloc(t->length ? t->length : 1);
        if (!t->ad_head) goto fail;
        memcpy(t->ad_head, data + stream, t->length);
    }
    return owner;
fail:
    melee_fighter_animation_free(owner);
    return NULL;
}
