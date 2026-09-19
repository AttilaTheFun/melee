#ifndef __GALE01_1D5238
#define __GALE01_1D5238

#include <Runtime/platform.h>

#include <melee/gr/types.h>
#include "grkongo.h"

extern GrJoint grKg_803E16E0[6];
extern StageCallbacks grKg_StageCallbacks[12];

/// A dat file index and the index of a spline within that file's
/// spline table, as taken by #Ground_801C247C.
typedef struct grKg_SplineChoice {
    s32 file;
    s32 spline;
} grKg_SplineChoice;

typedef struct grKg_SplineChoiceList {
    grKg_SplineChoice pairs[3];
    s32 terminator;
} grKg_SplineChoiceList;



extern const grKg_SplineChoiceList grKg_803B7FD4;

static struct grKongo_YakumonoParam* yakumono_param;

#endif
