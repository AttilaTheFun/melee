#ifndef MELEE_NATIVE_INPUT_H
#define MELEE_NATIVE_INPUT_H
#include "melee_host_types.h"
#include <stdint.h>
#include <dolphin/pad.h>

/* Apple virtual key codes on macOS. Multiple keys can bind one action. */
typedef enum {
    MELEE_LEFT, MELEE_RIGHT, MELEE_DOWN, MELEE_UP,
    MELEE_C_LEFT, MELEE_C_RIGHT, MELEE_C_DOWN, MELEE_C_UP,
    MELEE_A, MELEE_B, MELEE_X, MELEE_Y, MELEE_Z,
    MELEE_L, MELEE_R, MELEE_START, MELEE_ACTION_COUNT
} MeleeAction;

typedef struct {
    MeleeHostBool keys[128];
    uint32_t bindings[128];
} MeleeKeyboard;

void melee_keyboard_defaults(MeleeKeyboard *keyboard);
MeleeHostBool melee_keyboard_bind(MeleeKeyboard *keyboard, unsigned key, uint32_t actions);
void melee_keyboard_event(MeleeKeyboard *keyboard, unsigned key, MeleeHostBool pressed);
/* Call on focus loss or when entering key binding capture. */
void melee_keyboard_clear(MeleeKeyboard *keyboard);
PADStatus melee_keyboard_read(const MeleeKeyboard *keyboard);
/* Shared conversion for touch and Apple's extended gamepad profile.
 * Axes range -1...1; triggers 0...1. Nonfinite input is neutralized. */
PADStatus melee_analog_pad(uint16_t buttons, float x, float y, float cx,
                           float cy, float left, float right);
PADStatus melee_pad_merge(PADStatus a, PADStatus b);
#endif
