#ifndef MELEE_NATIVE_NESS_DATA_H
#define MELEE_NATIVE_NESS_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeNessData MeleeNessData;
/* Owns normal-match Ness ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeNessData* melee_ness_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_ness_data_header(MeleeNessData*);
void melee_ness_data_free(MeleeNessData*);
#endif
