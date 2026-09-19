#include "melee_fighter_modifiers.h"
#include <math.h>
#include <string.h>

_Static_assert(ftCo_MS_HeavyThrowLw4 - ftCo_MS_LightThrowF + 1 == 26,
               "Shared throw motion range");
_Static_assert(sizeof(struct Fighter_804D6524_t) == 156, "Scale modifier schema");
_Static_assert(sizeof(struct Fighter_804D6520_t) == 60, "Bunny modifier schema");
_Static_assert(sizeof(struct Fighter_804D651C_t) == 36, "Metal modifier schema");
_Static_assert(sizeof(struct Fighter_804D6518_t) == 8, "Gravity modifier schema");

MeleeHostBool melee_fighter_modifiers_decode(const MeleeArchive* a,
                                            MeleeFighterModifiers* out)
{
    if (!a || !out)
        return false;
    u32 root;
    if (!melee_archive_find(a, "ftLoadCommonData", &root))
        return false;
    MeleeFighterModifiers value;
    struct Field { unsigned entry; void* destination; size_t size; } fields[] = {
        {1, value.throws, sizeof(value.throws)},
        {2, value.swing, sizeof(value.swing)},
        {3, value.staling, sizeof(value.staling)},
        {12, &value.scale, sizeof(value.scale)},
        {13, &value.bunny, sizeof(value.bunny)},
        {14, &value.metal, sizeof(value.metal)},
        {15, &value.gravity, sizeof(value.gravity)},
    };
    for (unsigned i = 0; i < sizeof(fields)/sizeof(*fields); ++i) {
        u32 data;
        MeleeHostBool present;
        struct Field* field = &fields[i];
        if (!melee_archive_pointer(a, root + field->entry*4, &data, &present) ||
            !present || data > a->data_size || field->size > a->data_size-data)
            return false;
        for (unsigned j = 0; j < field->size; j += 4) {
            float number;
            if (!melee_archive_f32(a, data+j, &number) || !isfinite(number))
                return false;
            memcpy((u8*)field->destination+j, &number, 4);
        }
    }
    *out = value;
    return true;
}
