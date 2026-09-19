#include "melee_fighter_parts.h"
#include <melee/ft/ftparts.h>
#include <stdlib.h>
#include <string.h>

/* Includes the unnamed 0x35 part requested by ftParts_80074E58.
 * Accessory masks use a signed 1 << i in the original consumer. */
enum { CANONICAL_PARTS = 54, MAX_ACCESSORIES = 31 };
_Static_assert(sizeof(struct Fighter_804D6540_x0_t) == 4, "Accessory byte record");

struct MeleeFighterParts {
    FighterPartsTable maps[MELEE_FIGHTER_PART_TABLE_COUNT];
    FighterPartsTable* tables[MELEE_FIGHTER_PART_TABLE_COUNT];
    u8 joint_to_part[MELEE_FIGHTER_PART_TABLE_COUNT][MAX_FT_PARTS];
    u8 part_to_joint[MELEE_FIGHTER_PART_TABLE_COUNT][CANONICAL_PARTS];
    struct Fighter_804D6540_t extras[MELEE_FIGHTER_PART_TABLE_COUNT];
    struct Fighter_804D6540_t* accessories[MELEE_FIGHTER_PART_TABLE_COUNT];
    struct Fighter_804D6540_x0_t records[MELEE_FIGHTER_PART_TABLE_COUNT][MAX_ACCESSORIES];
};

void melee_fighter_parts_free(MeleeFighterParts* owner) { free(owner); }
FighterPartsTable** melee_fighter_parts_tables(MeleeFighterParts* owner)
{
    return owner ? owner->tables : NULL;
}
struct Fighter_804D6540_t** melee_fighter_parts_accessories(MeleeFighterParts* owner)
{
    return owner ? owner->accessories : NULL;
}

static int required(const MeleeArchive* a, u32 at, u32* target)
{
    MeleeHostBool present;
    return melee_archive_pointer(a, at, target, &present) && present;
}
static int copy_bytes(const MeleeArchive* a, u32 at, void* out, size_t size)
{
    if (at > a->data_size || size > a->data_size - at)
        return 0;
    memcpy(out, a->bytes + 32 + at, size);
    return 1;
}

MeleeFighterParts* melee_fighter_parts_decode(const MeleeArchive* a)
{
    u32 root, maps, extras;
    if (!a || !melee_archive_find(a, "ftLoadCommonData", &root) ||
        !required(a, root + 16, &maps) || !required(a, root + 20, &extras))
        return NULL;
    MeleeFighterParts* owner = calloc(1, sizeof(*owner));
    if (!owner)
        return NULL;
    for (unsigned kind = 0; kind < MELEE_FIGHTER_PART_TABLE_COUNT; ++kind) {
        u32 at, forward, inverse, count;
        if (!required(a, maps + 4 * kind, &at) ||
            !required(a, at, &forward) || !required(a, at + 4, &inverse) ||
            !melee_archive_u32(a, at + 8, &count) || !count || count > MAX_FT_PARTS ||
            !copy_bytes(a, forward, owner->joint_to_part[kind], count) ||
            !copy_bytes(a, inverse, owner->part_to_joint[kind], CANONICAL_PARTS))
            goto fail;
        for (unsigned i = 0; i < count; ++i) {
            u8 part = owner->joint_to_part[kind][i];
            if (part != FTPART_INVALID && part >= CANONICAL_PARTS)
                goto fail;
        }
        for (unsigned i = 0; i < CANONICAL_PARTS; ++i) {
            u8 joint = owner->part_to_joint[kind][i];
            if (joint != FTPART_INVALID && joint >= count)
                goto fail;
        }
        owner->maps[kind] = (FighterPartsTable){owner->joint_to_part[kind],
                                              owner->part_to_joint[kind], count};
        owner->tables[kind] = &owner->maps[kind];
        MeleeHostBool present;
        if (!melee_archive_pointer(a, extras + 4 * kind, &at, &present))
            goto fail;
        if (!present)
            continue;
        u32 records, n;
        if (!required(a, at, &records) || !melee_archive_u32(a, at + 4, &n) ||
            !n || n > MAX_ACCESSORIES ||
            !copy_bytes(a, records, owner->records[kind], n * 4))
            goto fail;
        for (unsigned i = 0; i < n; ++i) {
            struct Fighter_804D6540_x0_t* r = &owner->records[kind][i];
            if (r->x0 >= count || r->x1 >= count || r->x2 > 3)
                goto fail;
        }
        owner->extras[kind].x0 = owner->records[kind];
        owner->extras[kind].x4 = n;
        owner->accessories[kind] = &owner->extras[kind];
    }
    return owner;
fail:
    melee_fighter_parts_free(owner);
    return NULL;
}
