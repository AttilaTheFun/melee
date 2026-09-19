#ifndef MELEE_NATIVE_DONKEY_DATA_H
#define MELEE_NATIVE_DONKEY_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeDonkeyData MeleeDonkeyData;
/* Owns normal-match Donkey Kong ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeDonkeyData* melee_donkey_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_donkey_data_header(MeleeDonkeyData*);
void melee_donkey_data_free(MeleeDonkeyData*);
#endif
