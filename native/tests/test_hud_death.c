#include <melee/if/types.h>
#include "melee_hud.h"
#include <sysdolphin/baselib/jobj.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    IfDamageState state = {0};
    HSD_JObj joints[HUD_PLACE_MAX] = {0};
    state.damage_percent = 123;
    state.player_slot = 2;
    state.flags.explode_animation = 1;
    state.flags.randomize_velocity = 1;
    for (int i = 0; i < HUD_PLACE_MAX; ++i) {
        state.jobjs[i] = &joints[i];
        state.translation_x[i] = 20 + i;
        state.translation_y[i] = 30 + i;
    }
    melee_hud_death_animation(&state);
    assert(!state.flags.randomize_velocity && state.flags.explode_animation);
    float vx[HUD_PLACE_MAX], vy0[HUD_PLACE_MAX];
    for (int i = 0; i < HUD_PLACE_MAX; ++i) {
        vx[i] = state.velocity_x[i]; vy0[i] = state.velocity_y[i];
        assert(fabsf(vx[i]) >= 0.3041f && fabsf(vx[i]) <= 0.9124f);
        assert(vy0[i] >= 1.2165f && vy0[i] <= 2.0275f);
    }
    for (int frame = 0; frame < 5; ++frame) melee_hud_death_animation(&state);
    for (int i = 0; i < HUD_PLACE_MAX; ++i) {
        assert(fabsf(joints[i].translate.x - 5 * vx[i]) < 0.0001f);
        assert(fabsf(joints[i].translate.y - (5 * vy0[i] - 10 * 0.2028f)) < 0.0001f);
        assert(state.jobjs[i] == &joints[i]);
        assert(state.translation_x[i] == 20 + i && state.translation_y[i] == 30 + i);
    }
    joints[0].translate.x = 100;
    joints[0].translate.y = -100;
    float vy = state.velocity_y[0];
    melee_hud_death_animation(&state);
    assert(joints[0].translate.x == 100 && joints[0].translate.y == -100);
    assert(state.velocity_y[0] == vy && state.damage_percent == 123 && state.player_slot == 2);
    puts("Native HUD death: digit pointers, velocity initialization, gravity and bounds passed");
}
