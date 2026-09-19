#ifndef MELEE_NATIVE_WIREFRAME_DATA_H
#define MELEE_NATIVE_WIREFRAME_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeWireframeData MeleeWireframeData;
/* Owns normal-match male/female wireframe ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeWireframeData* melee_wireframe_data_decode(const MeleeArchive*,const void* aj,size_t aj_size,MeleeHostBool girl);
ftData* melee_wireframe_data_header(MeleeWireframeData*);
void melee_wireframe_data_free(MeleeWireframeData*);
#endif
