#ifndef MELEE_NATIVE_ICECLIMBERS_DATA_H
#define MELEE_NATIVE_ICECLIMBERS_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeIceClimbersData MeleeIceClimbersData;
/* Owns normal-match Popo or Nana ftData and AJ data.
 * Nana leaves x48_items null: Popo registers the shared item kinds.
 * Nana motion fallback requires Popo ftData to remain loaded.
 * Demo motions are installed separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeIceClimbersData* melee_iceclimbers_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_iceclimbers_data_header(MeleeIceClimbersData*);
void melee_iceclimbers_data_free(MeleeIceClimbersData*);
#endif
