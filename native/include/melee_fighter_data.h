#ifndef MELEE_NATIVE_FIGHTER_DATA_H
#define MELEE_NATIVE_FIGHTER_DATA_H
#include "melee_archive.h"
typedef struct MeleeFighterData MeleeFighterData;
MeleeFighterData* melee_fighter_data_decode(const MeleeArchive*);
void** melee_fighter_data_entries(MeleeFighterData*);
void melee_fighter_data_free(MeleeFighterData*);
#endif
