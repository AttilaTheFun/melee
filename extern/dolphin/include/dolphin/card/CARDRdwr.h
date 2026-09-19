#ifndef _DOLPHIN_CARDRDWR_H_
#define _DOLPHIN_CARDRDWR_H_

#ifdef MELEE_NATIVE
s32 CARDGetXferredBytes(s32 chan);
#else
long CARDGetXferredBytes(long chan);
#endif

#endif // _DOLPHIN_CARDRDWR_H_
