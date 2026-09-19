#include <melee/gm/types.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

_Static_assert(sizeof(struct lbl_8046B6A0_24C_t) == sizeof(MatchEnd), "Match result views must have the same extent");
_Static_assert(offsetof(struct lbl_8046B6A0_24C_t, x58) == offsetof(MatchEnd, player_standings), "Player records must overlay correctly");
_Static_assert(offsetof(struct lbl_8046B6A0_24C_t, x44C) == offsetof(MatchEnd, x44C), "Bonus tables must overlay correctly");

int main(void)
{
    MatchEnd result = {0};
    result.x0 = 12345;
    result.outcome = 2;
    for (int i = 0; i < 6; ++i) result.player_standings[i].pkind = i < 2 ? 0 : 3;
    struct lbl_8046B6A0_24C_t view;
    memcpy(&view, &result, sizeof(view));
    assert(view.x0 == 12345 && view.x4 == 2);
    for (int i = 0; i < 6; ++i) assert(view.x58[i].x0 == (i < 2 ? 0 : 3));
    puts("Native match results: timer, outcome, active slots and bonus-table layout passed");
}
