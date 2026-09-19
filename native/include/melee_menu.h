#ifndef MELEE_NATIVE_MENU_H
#define MELEE_NATIVE_MENU_H
#include "melee_archive.h"
#include <sysdolphin/baselib/archive.h>
/* Owned native archive adapter for MnMaAll and MnExtAll. Models decode lazily and retain
 * their descriptors across HSD scene-heap resets. Name-entry tables are not
 * yet supported. Remove borrowing objects before invoking native_destroy. */
HSD_Archive* melee_menu_decode(const MeleeArchive* archive);
/* Trophy model archives: optional animation bundles, owned lazy descriptors. */
HSD_Archive* melee_trophy_decode(const MeleeArchive* archive);
HSD_Archive* melee_trophy_files_decode(const MeleeArchive*);
MeleeHostBool melee_trophy_files_contains(HSD_Archive*,const char* filename);
#endif
