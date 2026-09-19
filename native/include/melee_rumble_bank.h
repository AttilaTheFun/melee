#ifndef MELEE_NATIVE_RUMBLE_BANK_H
#define MELEE_NATIVE_RUMBLE_BANK_H
#include "melee_archive.h"
#ifdef __cplusplus
extern "C" {
#endif
struct Fighter_804D653C_t;
typedef struct MeleeRumbleBank MeleeRumbleBank;
/* Owned commands and native table. No pointers into the borrowed DAT survive. */
MeleeRumbleBank* melee_rumble_bank_create(const MeleeArchive* archive);
void melee_rumble_bank_free(MeleeRumbleBank* bank);
size_t melee_rumble_bank_count(const MeleeRumbleBank* bank);
struct Fighter_804D653C_t* melee_rumble_bank_entries(MeleeRumbleBank* bank);
#ifdef __cplusplus
}
#endif
#endif
