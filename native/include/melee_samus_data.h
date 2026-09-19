#ifndef MELEE_NATIVE_SAMUS_DATA_H
#define MELEE_NATIVE_SAMUS_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeSamusData MeleeSamusData;
/* Owns normal-match Samus ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeSamusData* melee_samus_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_samus_data_header(MeleeSamusData*);
void melee_samus_data_free(MeleeSamusData*);
#endif
