#ifndef MELEE_NATIVE_KOOPA_DATA_H
#define MELEE_NATIVE_KOOPA_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeKoopaData MeleeKoopaData;
/* Owns normal-match Bowser/Giga Bowser ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeKoopaData* melee_koopa_data_decode(const MeleeArchive*,const void* aj,size_t aj_size,MeleeHostBool giga);
ftData* melee_koopa_data_header(MeleeKoopaData*);
void melee_koopa_data_free(MeleeKoopaData*);
#endif
