#ifndef MELEE_NATIVE_YOSHI_DATA_H
#define MELEE_NATIVE_YOSHI_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeYoshiData MeleeYoshiData;
/* Owns normal-match Yoshi ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeYoshiData* melee_yoshi_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_yoshi_data_header(MeleeYoshiData*);
void melee_yoshi_data_free(MeleeYoshiData*);
#endif
