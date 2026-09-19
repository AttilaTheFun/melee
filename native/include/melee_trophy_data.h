#ifndef MELEE_NATIVE_TROPHY_DATA_H
#define MELEE_NATIVE_TROPHY_DATA_H
#include "melee_archive.h"
typedef struct MeleeTrophyData MeleeTrophyData;
typedef enum {
    MELEE_TROPHY_INIT, MELEE_TROPHY_INIT_DIFFERENT, MELEE_TROPHY_NO_US,
    MELEE_TROPHY_EXP_DIFFERENT, MELEE_TROPHY_SORT, MELEE_TROPHY_DISPLAY,
    MELEE_TROPHY_DISPLAY_US, MELEE_TROPHY_TABLE_COUNT
} MeleeTrophyTable;
MeleeTrophyData* melee_trophy_data_decode(const MeleeArchive* archive);
void* melee_trophy_data_table(MeleeTrophyData* data,MeleeTrophyTable table);
size_t melee_trophy_data_count(const MeleeTrophyData* data,MeleeTrophyTable table);
void melee_trophy_data_free(MeleeTrophyData* data);
#endif
