#ifndef SYSDOLPHIN_BASELIB_PSDISPTEV_H
#define SYSDOLPHIN_BASELIB_PSDISPTEV_H

#include <Runtime/platform.h>

void psSetupTevCommon(void);
void psSetupTevInvalidState(void);
#ifdef MELEE_NATIVE
#include "psstructs.h"
void psSetupTev(HSD_Particle*);
#else
void psSetupTev(u32*);
#endif

#endif
