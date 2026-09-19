#ifndef MELEE_NATIVE_LINK_DATA_H
#define MELEE_NATIVE_LINK_DATA_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeLinkData MeleeLinkData;
/* Owns normal-match Link ftData and AJ data. Demo motions are installed
 * separately by the runtime results loader; this decoder leaves x14 null. Requires initialized HSD pools. */
MeleeLinkData* melee_link_data_decode(const MeleeArchive*,const void* aj,size_t aj_size,int young);
ftData* melee_link_data_header(MeleeLinkData*);
void melee_link_data_free(MeleeLinkData*);
#endif
