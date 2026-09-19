#ifndef MELEE_NATIVE_MARIO_DATA_H
#define MELEE_NATIVE_MARIO_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeMarioData MeleeMarioData;
/* Owns normal-match Mario/Dr. Mario ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeMarioData* melee_mario_data_decode(const MeleeArchive*,const void* aj,size_t aj_size,MeleeHostBool doctor);
ftData* melee_mario_data_header(MeleeMarioData*);
void melee_mario_data_free(MeleeMarioData*);
#endif
