#ifndef MELEE_NATIVE_ONETT_STAGE_H
#define MELEE_NATIVE_ONETT_STAGE_H
#include "melee_archive.h"
#include <sysdolphin/baselib/archive.h>
HSD_Archive* melee_onett_stage_decode(const MeleeArchive*);
void melee_onett_stage_register_particles(HSD_Archive*,int bank);
#endif
