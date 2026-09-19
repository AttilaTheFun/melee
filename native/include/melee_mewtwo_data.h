#ifndef MELEE_NATIVE_MEWTWO_DATA_H
#define MELEE_NATIVE_MEWTWO_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeMewtwoData MeleeMewtwoData;
/* Owns normal-match Mewtwo ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeMewtwoData* melee_mewtwo_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_mewtwo_data_header(MeleeMewtwoData*);
void melee_mewtwo_data_free(MeleeMewtwoData*);
#endif
