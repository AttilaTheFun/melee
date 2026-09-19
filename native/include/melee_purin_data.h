#ifndef MELEE_NATIVE_PURIN_DATA_H
#define MELEE_NATIVE_PURIN_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleePurinData MeleePurinData;
/* Owns normal-match Jigglypuff ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleePurinData* melee_purin_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_purin_data_header(MeleePurinData*);
void melee_purin_data_free(MeleePurinData*);
#endif
