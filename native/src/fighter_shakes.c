#include "melee_fighter_shakes.h"
#include <math.h>
#include <stdlib.h>

struct MeleeFighterShakes {
    struct Fighter_ShakeTable_t tables[5];
};
void melee_fighter_shakes_free(MeleeFighterShakes* owner)
{
    if (owner) {
        for (unsigned i = 0; i < 5; ++i)
            free(owner->tables[i].x0);
        free(owner);
    }
}
struct Fighter_ShakeTable_t* melee_fighter_shakes_tables(MeleeFighterShakes* owner)
{
    return owner ? owner->tables : NULL;
}
static int required(const MeleeArchive* a, u32 at, u32* target)
{
    MeleeHostBool present;
    return melee_archive_pointer(a, at, target, &present) && present;
}
MeleeFighterShakes* melee_fighter_shakes_decode(const MeleeArchive* a)
{
    u32 root, damage, grab, smash;
    if (!a || !melee_archive_find(a, "ftLoadCommonData", &root) ||
        !required(a, root + 36, &damage) || !required(a, root + 40, &grab) ||
        !required(a, root + 44, &smash))
        return NULL;
    u32 records[5] = {damage, damage + 8, damage + 16, grab, smash};
    MeleeFighterShakes* owner = calloc(1, sizeof(*owner));
    if (!owner)
        return NULL;
    for (unsigned i = 0; i < 5; ++i) {
        u32 data, count;
        /* Original consumers store the count in one byte. */
        if (!required(a, records[i], &data) ||
            !melee_archive_u32(a, records[i] + 4, &count) || !count || count > 255 ||
            data > a->data_size || count * 8 > a->data_size - data)
            goto fail;
        Vec2* values = calloc(count, sizeof(*values));
        if (!values)
            goto fail;
        owner->tables[i].x0 = values;
        owner->tables[i].x4 = count;
        for (unsigned j = 0; j < count; ++j) {
            if (!melee_archive_f32(a, data + j*8, &values[j].x) ||
                !melee_archive_f32(a, data + j*8 + 4, &values[j].y) ||
                !isfinite(values[j].x) || !isfinite(values[j].y))
                goto fail;
        }
    }
    return owner;
fail:
    melee_fighter_shakes_free(owner);
    return NULL;
}
