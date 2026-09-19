#ifndef MELEE_NATIVE_PIKACHU_DATA_H
#define MELEE_NATIVE_PIKACHU_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleePikachuData MeleePikachuData;
/* Owns normal-match Pikachu/Pichu ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleePikachuData* melee_pikachu_data_decode(const MeleeArchive*,const void* aj,size_t aj_size,MeleeHostBool pichu);
ftData* melee_pikachu_data_header(MeleePikachuData*);
void melee_pikachu_data_free(MeleePikachuData*);
#endif
