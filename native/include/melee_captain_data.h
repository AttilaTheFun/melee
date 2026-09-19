#ifndef MELEE_NATIVE_CAPTAIN_DATA_H
#define MELEE_NATIVE_CAPTAIN_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeCaptainData MeleeCaptainData;
/* Owns normal-match Captain Falcon/Ganondorf ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeCaptainData* melee_captain_data_decode(const MeleeArchive*,const void* aj,size_t aj_size,MeleeHostBool ganon);
ftData* melee_captain_data_header(MeleeCaptainData*);
void melee_captain_data_free(MeleeCaptainData*);
#endif
