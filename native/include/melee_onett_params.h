#ifndef MELEE_NATIVE_ONETT_PARAMS_H
#define MELEE_NATIVE_ONETT_PARAMS_H
#include "melee_archive.h"
#include <melee/gr/gronett.h>
/* Only Onett's yakumono_param schema; other stages have different layouts. */
MeleeHostBool melee_onett_params_decode(const MeleeArchive*,struct grOnett_StageParam*);
#endif
