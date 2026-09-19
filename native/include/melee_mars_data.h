#ifndef MELEE_NATIVE_MARS_DATA_H
#define MELEE_NATIVE_MARS_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeMarsData MeleeMarsData;
/* Owns normal-match Marth/Roy ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeMarsData* melee_mars_data_decode(const MeleeArchive*,const void* aj,size_t aj_size,MeleeHostBool roy);
ftData* melee_mars_data_header(MeleeMarsData*);
void melee_mars_data_free(MeleeMarsData*);
#endif
