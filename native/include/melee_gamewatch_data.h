#ifndef MELEE_NATIVE_GAMEWATCH_DATA_H
#define MELEE_NATIVE_GAMEWATCH_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeGameWatchData MeleeGameWatchData;
/* Owns normal-match GameWatch ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeGameWatchData* melee_gamewatch_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_gamewatch_data_header(MeleeGameWatchData*);
void melee_gamewatch_data_free(MeleeGameWatchData*);
#endif
