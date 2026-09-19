#include "melee_fighter_common.h"
#include <math.h>
#include <string.h>

_Static_assert(sizeof(ftCommonData) == 0x818, "PlCo scalar parameter size");
_Static_assert(offsetof(ftCommonData, x380) == 0x380, "PlCo hit capsule offset");
_Static_assert(offsetof(ftCommonData, x6DC_colorsByPlayer) == 0x6dc, "PlCo colors offset");
_Static_assert(offsetof(ftCommonData, x814) == 0x814, "PlCo last parameter offset");

static int raw_word(unsigned offset)
{
    return (offset >= 0x6dc && offset < 0x6f0) || offset == 0x7d8;
}

MeleeHostBool melee_fighter_common_decode(const MeleeArchive* a, ftCommonData* out)
{
    u32 root, data;
    MeleeHostBool present;
    if (!a || !out || !melee_archive_find(a, "ftLoadCommonData", &root) ||
        !melee_archive_pointer(a, root, &data, &present) || !present ||
        data > a->data_size || a->data_size - data < sizeof(*out))
        return false;
    /* This block has no pointers. Reject a different serialized schema. */
    for (u32 i = 0; i < a->reloc_count; ++i) {
        u32 slot, target;
        if (!melee_archive_relocation(a, i, &slot, &target) ||
            (slot >= data && slot - data < sizeof(*out)))
            return false;
    }
    /* Integer parameters, including the embedded hit-capsule descriptor.
     * Every other non-byte word is a float (including Vec2/Vec3 fields). */
    static const u16 integer_offsets[] = {
        0x1c, 0x40, 0x74, 0x8c, 0xe4, 0xfc, 0x130, 0x134, 0x18c, 0x1b8,
        0x1dc, 0x214, 0x224, 0x23c, 0x274, 0x2a0, 0x2b8, 0x318, 0x320, 0x324,
        0x334, 0x348, 0x380, 0x384, 0x388, 0x38c, 0x390, 0x394, 0x398, 0x39c,
        0x3a0, 0x3c0, 0x3cc, 0x3f4, 0x3f8, 0x3fc, 0x410, 0x414, 0x418, 0x41c,
        0x428, 0x488, 0x498, 0x49c, 0x4b4, 0x4c4, 0x4c8, 0x4cc, 0x4d8, 0x4f8,
        0x4fc, 0x500, 0x504, 0x508, 0x50c, 0x518, 0x520, 0x524, 0x528, 0x52c,
        0x530, 0x534, 0x538, 0x544, 0x57c, 0x580, 0x584, 0x588, 0x5a4, 0x5b4,
        0x5bc, 0x5c4, 0x5c8, 0x5d0, 0x5d4, 0x5d8, 0x5dc, 0x5e0, 0x5e4, 0x5ec,
        0x5f0, 0x5f4, 0x620, 0x648, 0x688, 0x68c, 0x690, 0x6ac, 0x6b0, 0x6b4,
        0x6b8, 0x6bc, 0x6c0, 0x6c8, 0x6cc, 0x6d4, 0x6d8, 0x6f4, 0x6f8, 0x6fc,
        0x700, 0x73c, 0x760, 0x764, 0x774, 0x7ac, 0x7b0, 0x7b4, 0x7cc, 0x7d0,
        0x7dc, 0x7e0, 0x7e8, 0x7ec, 0x7f0, 0x814,
    };
    ftCommonData value;
    memcpy(&value, a->bytes + 32 + data, sizeof(value));
    unsigned integer_index = 0;
    for (unsigned offset = 0; offset < sizeof(value); offset += 4) {
        if (raw_word(offset))
            continue;
        u32 bits;
        if (!melee_archive_u32(a, data + offset, &bits))
            return false;
        memcpy((u8*)&value + offset, &bits, 4);
        if (integer_index < sizeof(integer_offsets) / sizeof(*integer_offsets) &&
            offset == integer_offsets[integer_index]) {
            ++integer_index;
        } else {
            float number;
            memcpy(&number, &bits, 4);
            if (!isfinite(number))
                return false;
        }
    }
    *out = value;
    return true;
}
