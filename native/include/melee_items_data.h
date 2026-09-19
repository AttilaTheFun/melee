#ifndef MELEE_NATIVE_ITEMS_DATA_H
#define MELEE_NATIVE_ITEMS_DATA_H
#include "melee_archive.h"
#include <melee/it/it_3F14.h>
typedef struct MeleeItemsData MeleeItemsData;
MeleeItemsData* melee_items_data_decode(const MeleeArchive*);
it_804D6D20_t* melee_items_data_header(MeleeItemsData*);
/* Returns a converted/registered article, or NULL for a deferred kind. */
Article* melee_items_data_article(MeleeItemsData*,unsigned kind);
void melee_items_data_free(MeleeItemsData*);
#endif
