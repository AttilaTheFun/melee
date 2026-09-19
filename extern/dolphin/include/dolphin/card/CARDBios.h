#ifndef _DOLPHIN_CARDBIOS_H_
#define _DOLPHIN_CARDBIOS_H_

void CARDInit(void);
s32 CARDGetResultCode(s32 chan);
s32 CARDFreeBlocks(s32 chan, s32 *byteNotUsed, s32 *filesNotUsed);
#ifdef MELEE_NATIVE
s32 CARDGetEncoding(s32 chan, unsigned short * encode);
#else
long CARDGetEncoding(long chan, unsigned short * encode);
#endif
#ifdef MELEE_NATIVE
s32 CARDGetMemSize(s32 chan, unsigned short * size);
#else
long CARDGetMemSize(long chan, unsigned short * size);
#endif
s32 CARDGetSectorSize(s32 chan, u32 *size);

#endif // _DOLPHIN_CARDBIOS_H_
