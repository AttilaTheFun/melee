#ifndef MELEE_NATIVE_ITEM_SPECIAL_H
#define MELEE_NATIVE_ITEM_SPECIAL_H
#include "melee_archive.h"
/* Scalar special attributes plus the event egg's verified null-pointer record.
 * Pointer-rich kinds use separate owners; unsupported kinds return zero/NULL.
 * Returned storage is owned by the caller and freed with free(). */
size_t melee_item_special_size(unsigned kind);
void* melee_item_special_decode(const MeleeArchive*,unsigned kind,uint32_t offset);
#endif
