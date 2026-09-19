#ifndef MELEE_CARD_BACKEND_H
#define MELEE_CARD_BACKEND_H
#include "melee_card_store.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Host lifecycle, serialized outside SDK callbacks. Slots start empty.
 * Invalid files are rejected without changing the current slot. Eject/shutdown
 * return BUSY while I/O or completion callbacks are outstanding. */
s32 melee_card_insert(s32 channel, const char* path, bool create);
s32 melee_card_eject(s32 channel);
s32 melee_card_shutdown(void);
#ifdef __cplusplus
}
#endif
#endif
