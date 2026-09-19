#ifndef MELEE_NATIVE_SHEIK_DATA_H
#define MELEE_NATIVE_SHEIK_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeSheikData MeleeSheikData;
/* Owns normal-match Sheik ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeSheikData* melee_sheik_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_sheik_data_header(MeleeSheikData*);
void melee_sheik_data_free(MeleeSheikData*);
#endif
