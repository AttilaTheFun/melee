#ifndef MELEE_NATIVE_EVENT_DATA_H
#define MELEE_NATIVE_EVENT_DATA_H
#include "melee_archive.h"
#include <melee/gm/gmevent_data.h>
typedef struct MeleeEventData MeleeEventData;
/* Owned 51-entry retail event table, including rules, players, stages and
 * event-specific auxiliary data. No archive bytes are borrowed. */
MeleeEventData* melee_event_data_decode(const MeleeArchive* archive);
struct gm_804D6900_t** melee_event_data_table(MeleeEventData* data);
void melee_event_data_free(MeleeEventData* data);
#endif
