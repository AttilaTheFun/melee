#include "../../src/melee/gm/gm_1601.c"
#include <assert.h>
#include <stdio.h>

static CharacterKind selected_character;
static u32 selected_costume;
static FighterKind selected_kind;
CharacterKind Player_GetPlayerCharacter(int slot) { assert(slot == 2); return selected_character; }
u32 Player_GetCostumeId(int slot) { assert(slot == 2); return selected_costume; }
FighterKind Player_80036394(s32 slot) { assert(slot == 2); return selected_kind; }

int main(void)
{
    /* Independent expected atlas slots from retail gm_80168B34 (US 1.02). */
    static const int slots[] = {
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,
        18,18,19,20,21,22,23,24,28,26,26,58,27,59,14
    };
    for (unsigned character = 0; character < sizeof(slots)/sizeof(slots[0]); ++character) {
        for (unsigned costume = 0; costume < 6; ++costume) {
            selected_character = (CharacterKind) character;
            selected_costume = costume;
            selected_kind = (FighterKind) 0;
            float expected = slots[character] + (character >= 26 && character <= 31 ? 0 : 30 * costume);
            assert(gm_80168B34(selected_character, 0, costume) == expected);
            assert(gm_80168BF8(2) == expected);
        }
    }
    selected_character = CKind_Seak;
    selected_costume = 3;
    selected_kind = (FighterKind) 7;
    assert(gm_80168BF8(2) == 115.0f);
    assert(gm_80168B34(CKind_Zelda, 7, 1) == 55.0f);
    puts("HUD atlas frames: 33 characters, six costumes and transformed Zelda/Sheik passed");
}
