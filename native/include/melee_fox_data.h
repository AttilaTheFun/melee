#ifndef MELEE_NATIVE_FOX_DATA_H
#define MELEE_NATIVE_FOX_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeFoxData MeleeFoxData;
/* Owns normal-match Fox/Falco ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeFoxData* melee_fox_data_decode(const MeleeArchive*,const void* aj,size_t aj_size,MeleeHostBool falco);
ftData* melee_fox_data_header(MeleeFoxData*);
void melee_fox_data_free(MeleeFoxData*);
#endif
