#include "melee_input.h"
#include <math.h>
#include <string.h>

void melee_keyboard_clear(MeleeKeyboard *k) { memset(k->keys, 0, sizeof(k->keys)); }

MeleeHostBool melee_keyboard_bind(MeleeKeyboard *k, unsigned key, uint32_t actions)
{
    if (key >= 128 || actions >= (1u << MELEE_ACTION_COUNT)) return false;
    k->bindings[key] = actions;
    return true;
}

void melee_keyboard_event(MeleeKeyboard *k, unsigned key, MeleeHostBool pressed)
{
    if (key < 128) k->keys[key] = pressed;
}

void melee_keyboard_defaults(MeleeKeyboard *k)
{
    memset(k, 0, sizeof(*k));
    const unsigned keys[] = {0, 2, 1, 13, 123, 124, 125, 126,
                             38, 40, 32, 34, 12, 14, 49, 36,
                             4, 37, 46, 31};
    const MeleeAction actions[] = {MELEE_LEFT, MELEE_RIGHT, MELEE_DOWN, MELEE_UP,
        MELEE_LEFT, MELEE_RIGHT, MELEE_DOWN, MELEE_UP,
        MELEE_A, MELEE_B, MELEE_X, MELEE_Y, MELEE_L, MELEE_R, MELEE_Z, MELEE_START,
        MELEE_C_LEFT, MELEE_C_RIGHT, MELEE_C_DOWN, MELEE_C_UP};
    for (unsigned i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i)
        k->bindings[keys[i]] = 1u << actions[i];
}

static float clamp(float value, float low, float high)
{
    return isfinite(value) ? fminf(high, fmaxf(low, value)) : 0;
}

static void stick(float x, float y, s8 *ox, s8 *oy)
{
    x = clamp(x, -1, 1); y = clamp(y, -1, 1);
    float length = sqrtf(x*x + y*y);
    if (length > 1) { x /= length; y /= length; }
    *ox = (s8)lroundf(x * 80);
    *oy = (s8)lroundf(y * 80);
}

PADStatus melee_analog_pad(uint16_t buttons, float x, float y, float cx,
                           float cy, float left, float right)
{
    PADStatus p = {.button = buttons, .err = PAD_ERR_NONE};
    stick(x, y, &p.stickX, &p.stickY);
    stick(cx, cy, &p.substickX, &p.substickY);
    /* Match Melee's calibrated range in gmMain_8015FD24. Mapping host
     * 0...1 to 255 would reach full analog shield at only 55% travel. */
    p.triggerLeft = (u8)lroundf(clamp(left, 0, 1) * 140);
    p.triggerRight = (u8)lroundf(clamp(right, 0, 1) * 140);
    p.analogA = (buttons & PAD_BUTTON_A) ? 255 : 0;
    p.analogB = (buttons & PAD_BUTTON_B) ? 255 : 0;
    return p;
}

PADStatus melee_keyboard_read(const MeleeKeyboard *k)
{
    uint32_t actions = 0;
    for (unsigned key = 0; key < 128; ++key)
        if (k->keys[key]) actions |= k->bindings[key];
    const uint16_t buttons[] = {PAD_BUTTON_A, PAD_BUTTON_B, PAD_BUTTON_X,
        PAD_BUTTON_Y, PAD_TRIGGER_Z, PAD_TRIGGER_L, PAD_TRIGGER_R, PAD_BUTTON_START};
    uint16_t mask = 0;
    for (unsigned i = MELEE_A; i <= MELEE_START; ++i)
        if (actions & (1u << i)) mask |= buttons[i - MELEE_A];
#define ACTIVE(a) ((actions & (1u << (a))) != 0)
    return melee_analog_pad(mask, ACTIVE(MELEE_RIGHT) - ACTIVE(MELEE_LEFT),
        ACTIVE(MELEE_UP) - ACTIVE(MELEE_DOWN), ACTIVE(MELEE_C_RIGHT) - ACTIVE(MELEE_C_LEFT),
        ACTIVE(MELEE_C_UP) - ACTIVE(MELEE_C_DOWN), ACTIVE(MELEE_L), ACTIVE(MELEE_R));
#undef ACTIVE
}

static unsigned magnitude(s8 x, s8 y) { return x*x + y*y; }

PADStatus melee_pad_merge(PADStatus a, PADStatus b)
{
    if (a.err != PAD_ERR_NONE) return b;
    if (b.err != PAD_ERR_NONE) return a;
    a.button |= b.button;
    /* Choose a whole stick, avoiding synthetic diagonals from two devices. */
    if (magnitude(b.stickX, b.stickY) > magnitude(a.stickX, a.stickY)) {
        a.stickX = b.stickX; a.stickY = b.stickY;
    }
    if (magnitude(b.substickX, b.substickY) > magnitude(a.substickX, a.substickY)) {
        a.substickX = b.substickX; a.substickY = b.substickY;
    }
#define MAX_FIELD(field) if (b.field > a.field) a.field = b.field
    MAX_FIELD(triggerLeft); MAX_FIELD(triggerRight); MAX_FIELD(analogA); MAX_FIELD(analogB);
#undef MAX_FIELD
    return a;
}
