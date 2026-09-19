#ifndef MELEE_NATIVE_KIRBY_DATA_H
#define MELEE_NATIVE_KIRBY_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeKirbyData MeleeKirbyData;
/* Owns normal-match Kirby ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeKirbyData* melee_kirby_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_kirby_data_header(MeleeKirbyData*);
void melee_kirby_data_free(MeleeKirbyData*);
#endif
