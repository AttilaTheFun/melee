#include "melee_fighter_parts.h"
#include <melee/ft/ftparts.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FighterPartsTable** ftPartsTable;
struct Fighter_804D6540_t** Fighter_804D6540;
static u32 ref(const MeleeArchive* a, u32 slot)
{
    u32 value; MeleeHostBool present;
    assert(melee_archive_pointer(a, slot, &value, &present) && present);
    return value;
}
static void word(u8* p, u32 value)
{
    p[0]=value>>24; p[1]=value>>16; p[2]=value>>8; p[3]=value;
}
int main(int argc, char** argv)
{
    assert(argc == 2);
    FILE* file = fopen(argv[1], "rb");
    assert(file && !fseek(file, 0, SEEK_END));
    long size = ftell(file); assert(size > 0); rewind(file);
    u8* bytes = malloc(size);
    assert(bytes && fread(bytes, 1, size, file) == size); fclose(file);
    MeleeArchive a; assert(melee_archive_open(&a, bytes, size));
    MeleeFighterParts* owner = melee_fighter_parts_decode(&a); assert(owner);
    ftPartsTable = melee_fighter_parts_tables(owner);
    Fighter_804D6540 = melee_fighter_parts_accessories(owner);
    u32 root; assert(melee_archive_find(&a, "ftLoadCommonData", &root));
    u32 maps = ref(&a, root + 16), extras = ref(&a, root + 20);
    u8 forward[MELEE_FIGHTER_PART_TABLE_COUNT][140] = {{0}}, inverse[MELEE_FIGHTER_PART_TABLE_COUNT][54];
    unsigned total = 0, accessory_count = 0;
    for (unsigned k = 0; k < MELEE_FIGHTER_PART_TABLE_COUNT; ++k) {
        u32 at = ref(&a, maps + 4*k), n;
        assert(melee_archive_u32(&a, at+8, &n));
        assert(ftPartsTable[k]->parts_num == n);
        memcpy(forward[k], bytes+32+ref(&a, at), n);
        memcpy(inverse[k], bytes+32+ref(&a, at+4), 54);
        assert(!memcmp(ftPartsTable[k]->joint_to_part, forward[k], n));
        assert(!memcmp(ftPartsTable[k]->part_to_joint, inverse[k], 54));
        if (Fighter_804D6540[k]) {
            u32 extra = ref(&a, extras+4*k), count;
            assert(melee_archive_u32(&a, extra+4, &count));
            assert(Fighter_804D6540[k]->x4 == count);
            assert(!memcmp(Fighter_804D6540[k]->x0, bytes+32+ref(&a, extra), count*4));
            accessory_count += count;
        }
        total += n;
    }
    assert(accessory_count == 16);
    /* Invalid serialized indexes/counts must not produce a usable owner. */
    u32 first = ref(&a, maps), at = ref(&a, first), count;
    assert(melee_archive_u32(&a, first+8, &count));
    u8 saved = bytes[32+at]; bytes[32+at] = 54;
    assert(!melee_fighter_parts_decode(&a)); bytes[32+at] = saved;
    word(bytes+32+first+8, 141); assert(!melee_fighter_parts_decode(&a));
    word(bytes+32+first+8, count);
    MeleeArchive truncated = a; truncated.data_size = maps + MELEE_FIGHTER_PART_TABLE_COUNT*4 - 1;
    assert(!melee_fighter_parts_decode(&truncated));
    memset(bytes, 0xa5, size); free(bytes);
    /* Execute original consumers after all serialized input is gone. */
    Fighter fighter = {0}; unsigned remaps = 0;
    for (unsigned from = 0; from < MELEE_FIGHTER_PART_TABLE_COUNT; ++from) {
        fighter.kind = from;
        for (unsigned part = 0; part < 54; ++part)
            assert(ftParts_GetBoneIndex(&fighter, part) == inverse[from][part]);
        for (unsigned joint = 0; joint < ftPartsTable[from]->parts_num; ++joint) {
            unsigned expected_mask = 0;
            if (Fighter_804D6540[from])
                for (unsigned i = 0; i < Fighter_804D6540[from]->x4; ++i)
                    if (Fighter_804D6540[from]->x0[i].x0 == joint)
                        expected_mask = 1u << i;
            assert(ftParts_8007506C(from, joint) == expected_mask);
            for (unsigned to = 0; to < MELEE_FIGHTER_PART_TABLE_COUNT; ++to) {
                u8 part = forward[from][joint];
                int expected = part == 255 ? 255 : inverse[to][part];
                assert(ftPartsRemap(to, from, joint) == expected); ++remaps;
            }
        }
        assert(ftPartsRemap(0, from, ftPartsTable[from]->parts_num) == 255);
    }
    melee_fighter_parts_free(owner); ftPartsTable = NULL; Fighter_804D6540 = NULL;
    printf("Fighter parts: 34 maps, %u joints, 16 accessories, %u original remaps and source disposal passed\n", total, remaps);
}
