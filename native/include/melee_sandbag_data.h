#ifndef MELEE_NATIVE_SANDBAG_DATA_H
#define MELEE_NATIVE_SANDBAG_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeSandbagData MeleeSandbagData;
/* Owns Sandbag ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeSandbagData* melee_sandbag_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_sandbag_data_header(MeleeSandbagData*);
void melee_sandbag_data_free(MeleeSandbagData*);
#endif
