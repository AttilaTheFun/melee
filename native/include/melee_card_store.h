#ifndef MELEE_CARD_STORE_H
#define MELEE_CARD_STORE_H
#include <stdbool.h>
#include <dolphin/card.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Private native container, not a raw GameCube memory-card image. 251 usable
 * 8 KiB blocks and 127 directory slots. SDK callbacks are supplied separately. */
typedef struct MeleeCardStore MeleeCardStore;
MeleeCardStore* melee_card_store_open(const char* path, bool create, s32* error);
void melee_card_store_close(MeleeCardStore* store); /* No concurrent users. */
s32 melee_card_store_find(MeleeCardStore* store, const char* name);
s32 melee_card_store_create(MeleeCardStore* store, const char* name, u32 length, u32 time, s32* slot);
s32 melee_card_store_delete(MeleeCardStore* store, s32 slot);
s32 melee_card_store_rename(MeleeCardStore* store, s32 slot, const char* name);
s32 melee_card_store_stat(MeleeCardStore* store, s32 slot, CARDStat* out);
s32 melee_card_store_set_stat(MeleeCardStore* store, s32 slot, const CARDStat* in, u32 time);
s32 melee_card_store_read(MeleeCardStore* store, s32 slot, void* bytes, s32 length, s32 offset);
s32 melee_card_store_write(MeleeCardStore* store, s32 slot, const void* bytes, s32 length, s32 offset);
s32 melee_card_store_free(MeleeCardStore* store, s32* bytes, s32* slots);
s32 melee_card_store_check(MeleeCardStore* store);
s32 melee_card_store_write_timed(MeleeCardStore* store, s32 slot, const void* bytes, s32 length, s32 offset, u32 time);
s32 melee_card_store_format(MeleeCardStore* store);
#ifdef __cplusplus
}
#endif
#endif
