/* Exercise the real item-table builder without assuming adjacent globals. */
#include "../../src/melee/it/itspawn.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* HSD_MemAlloc(ssize_t size)
{
    void* result = malloc((size_t) size);
    assert(result);
    return result;
}

int main(void)
{
    s32 counts[It_Kind_L_Gun_Ray] = {0};
    unsigned char spawner_before[sizeof(it_804A0E30)];
    unsigned char monster_before[sizeof(it_804A0E60)];
    memset(&it_804A0E30, 0x5A, sizeof(it_804A0E30));
    memset(&it_804A0E60, 0xA5, sizeof(it_804A0E60));
    memcpy(spawner_before, &it_804A0E30, sizeof(spawner_before));
    memcpy(monster_before, &it_804A0E60, sizeof(monster_before));
    counts[It_Kind_BombHei] = 2;
    counts[It_Kind_BombHei + 1] = 99; /* Excluded by the item mask. */
    counts[It_Kind_BombHei + 2] = 5;
    it_804A0E50.x8 = 7;
    it_8026CD50(counts, 5, 1.0f);
    assert(it_804A0E50.size == 2 && it_804A0E50.x8 == 7);
    assert(it_804A0E50.x4[0] == It_Kind_BombHei);
    assert(it_804A0E50.x4[1] == It_Kind_BombHei + 2);
    assert(it_804A0E50.xC[0] == 0 && it_804A0E50.xC[1] == 2);
    assert(!memcmp(spawner_before, &it_804A0E30, sizeof(spawner_before)));
    assert(!memcmp(monster_before, &it_804A0E60, sizeof(monster_before)));
    free(it_804A0E50.x4);
    free(it_804A0E50.xC);
    puts("PASS item weights and kinds use their explicit table; unrelated globals preserved");
    return 0;
}
