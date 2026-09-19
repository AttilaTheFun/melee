/* Deliberately include game headers first: host ABI must not depend on order. */
#include <Runtime/platform.h>
#include "melee_input.h"
#include "melee_archive.h"
#include "melee_pad_backend.h"
#include <dolphin/gx.h>
#include <dolphin/card.h>
#include <assert.h>
#include <stdio.h>

struct GameFlags { u32 id; bool ready; u32 next; };
_Static_assert(sizeof(bool) == 4, "MSL game bool is a 32-bit integer");
_Static_assert(sizeof(struct GameFlags) == 12, "Preserve game boolean layout");
_Static_assert(offsetof(struct GameFlags, next) == 8, "Preserve following fields");
_Static_assert(sizeof(MeleeHostBool) == 1, "Apple API bool stays native");
_Static_assert(sizeof(GXTexObj) == 32, "Default SDK texture ABI is unchanged");
_Static_assert(sizeof(GXTlutObj) == 12, "Default SDK palette ABI is unchanged");
_Static_assert(sizeof(MeleeKeyboard) == 640, "Game headers must not widen host keys");
_Static_assert(__builtin_types_compatible_p(bool, int), "Preserve original callback ABI");
_Static_assert(__builtin_types_compatible_p(__typeof__(&melee_keyboard_bind),
    MeleeHostBool (*)(MeleeKeyboard*, unsigned, uint32_t)), "Host API signature");

/* PPC long was a 32-bit game word; Darwin ARM64 long is 64 bits. All
 * public CARD entry points must agree with the asynchronous callback ABI. */
#define CARD_SIGNATURE(name, type) _Static_assert(__builtin_types_compatible_p(__typeof__(&name), type), #name " native ABI")
CARD_SIGNATURE(CARDWriteAsync, s32 (*)(CARDFileInfo*, void*, s32, s32, CARDCallback));
CARD_SIGNATURE(CARDWrite, s32 (*)(CARDFileInfo*, void*, s32, s32));
CARD_SIGNATURE(CARDRead, s32 (*)(CARDFileInfo*, void*, s32, s32));
CARD_SIGNATURE(CARDCreate, s32 (*)(s32, char*, u32, CARDFileInfo*));
CARD_SIGNATURE(CARDSetStatus, s32 (*)(s32, s32, CARDStat*));
CARD_SIGNATURE(CARDGetXferredBytes, s32 (*)(s32));
CARD_SIGNATURE(CARDGetEncoding, s32 (*)(s32, unsigned short*));
CARD_SIGNATURE(CARDGetMemSize, s32 (*)(s32, unsigned short*));
CARD_SIGNATURE(CARDFormat, s32 (*)(s32));
CARD_SIGNATURE(CARDCheck, s32 (*)(s32));
CARD_SIGNATURE(CARDProbe, int (*)(s32));
CARD_SIGNATURE(CARDFastDelete, s32 (*)(s32, s32));
_Static_assert(sizeof(CARDFileInfo)==20 && sizeof(CARDStat)==108,"CARD scalar layouts");
#undef CARD_SIGNATURE

static bool game_callback(int value) { return value; }
int main(void)
{
    int (*callback)(int) = game_callback;
    assert(callback(-1) == -1); /* MSL bool does not canonicalize to 0/1. */
    MeleeKeyboard k;
    melee_keyboard_defaults(&k);
    assert(melee_keyboard_bind(&k, 3, 1u << MELEE_B));
    melee_keyboard_event(&k, 3, true);
    assert(melee_keyboard_read(&k).button & PAD_BUTTON_B);
    melee_keyboard_event(&k, 3, false);
    assert(melee_keyboard_read(&k).button == 0);
    puts("32-bit game booleans and separate Apple boolean ABI passed.");
    return 0;
}
