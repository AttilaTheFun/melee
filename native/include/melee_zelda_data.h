#ifndef MELEE_NATIVE_ZELDA_DATA_H
#define MELEE_NATIVE_ZELDA_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeZeldaData MeleeZeldaData;
/* Owns normal-match Zelda ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeZeldaData* melee_zelda_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_zelda_data_header(MeleeZeldaData*);
void melee_zelda_data_free(MeleeZeldaData*);
#endif
