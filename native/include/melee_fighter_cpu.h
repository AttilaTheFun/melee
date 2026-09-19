#ifndef MELEE_NATIVE_FIGHTER_CPU_H
#define MELEE_NATIVE_FIGHTER_CPU_H
#include "melee_archive.h"
#include <melee/ft/fighter.h>
typedef struct MeleeFighterCpu MeleeFighterCpu;
MeleeFighterCpu* melee_fighter_cpu_decode(const MeleeArchive*);
struct Fighter_804D64FC_t* melee_fighter_cpu_header(MeleeFighterCpu*);
void melee_fighter_cpu_free(MeleeFighterCpu*);
#endif
