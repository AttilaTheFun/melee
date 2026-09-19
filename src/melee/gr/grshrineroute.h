#ifndef GALE01_2087B0
#define GALE01_2087B0

#include <melee/gr/forward.h>
#include <melee/gr/types.h>

struct grShrineRoute_YakumonoParam {
#ifdef MELEE_NATIVE
    union ColorOverlay_x8_t* x0;
    union ColorOverlay_x8_t* x4;
    union ColorOverlay_x8_t* x8;
    union ColorOverlay_x8_t* xC;
    lbColl_80008D30_arg1* x10;
#else
    int x0, x4, x8, xC, x10;
#endif
    f32 x14, x18, x1C, x20;
    int x24;
    grZakoGenerator_SpawnDesc spawn_desc;
#ifdef MELEE_NATIVE
    grZakoGenerator_SpawnDesc native_spawn_tail[79];
#endif
};

/* 3E5988 */ extern StageData grSh_Route_StageData;

#endif
