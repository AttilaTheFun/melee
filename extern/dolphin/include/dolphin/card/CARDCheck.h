#ifndef _DOLPHIN_CARDCHECK_H_
#define _DOLPHIN_CARDCHECK_H_

s32 CARDCheckAsync(s32 chan, CARDCallback callback);
#ifdef MELEE_NATIVE
s32 CARDCheck(s32 chan);
#else
long CARDCheck(long chan);
#endif

#endif // _DOLPHIN_CARDCHECK_H_
