#ifndef MELEE_NATIVE_STAGE_PARAMS_H
#define MELEE_NATIVE_STAGE_PARAMS_H
#include "melee_archive.h"
#include <melee/gr/types.h>
GroundParam* melee_stage_params_decode(const MeleeArchive*);
void melee_stage_params_free(GroundParam*);
#endif
