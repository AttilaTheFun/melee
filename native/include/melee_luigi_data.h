#ifndef MELEE_NATIVE_LUIGI_DATA_H
#define MELEE_NATIVE_LUIGI_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeLuigiData MeleeLuigiData;
/* Owns normal-match Luigi ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeLuigiData* melee_luigi_data_decode(const MeleeArchive*,const void* aj,size_t aj_size);
ftData* melee_luigi_data_header(MeleeLuigiData*);
void melee_luigi_data_free(MeleeLuigiData*);
#endif
