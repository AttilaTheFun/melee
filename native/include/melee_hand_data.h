#ifndef MELEE_NATIVE_HAND_DATA_H
#define MELEE_NATIVE_HAND_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeHandData MeleeHandData;
/* Owns boss fighter data and copied AJ motions; requires initialized HSD pools. */
MeleeHandData* melee_hand_data_decode(const MeleeArchive*,const void*,size_t,MeleeHostBool crazy);
ftData* melee_hand_data_header(MeleeHandData*);
void melee_hand_data_free(MeleeHandData*);
#endif
