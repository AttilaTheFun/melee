#ifndef MELEE_NATIVE_PEACH_DATA_H
#define MELEE_NATIVE_PEACH_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleePeachData MeleePeachData;
/* Owns normal-match Peach ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleePeachData* melee_peach_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_peach_data_header(MeleePeachData*);
void melee_peach_data_free(MeleePeachData*);
#endif
