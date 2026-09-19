#ifndef MELEE_NATIVE_CARD_ICONS_H
#define MELEE_NATIVE_CARD_ICONS_H
#include "melee_archive.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MeleeCardIcons MeleeCardIcons;
/* Own the three banner payloads and icon payload from MemCardIconData.
 * Packed pixel/palette bytes retain disc byte order for card serialization.
 * No source DAT pointers survive. */
MeleeCardIcons* melee_card_icons_create(const MeleeArchive* archive);
/* Snapshot cards have one banner and one icon, exposed at indices 0 and 1. */
MeleeCardIcons* melee_card_snapshot_icons_create(const MeleeArchive* archive);
const void* melee_card_icons_data(const MeleeCardIcons* icons, unsigned index);
size_t melee_card_icons_size(const MeleeCardIcons* icons, unsigned index);
void melee_card_icons_free(MeleeCardIcons* icons);
#ifdef __cplusplus
}
#endif
#endif
