#ifndef _DOLPHIN_CARDCREATE_H_
#define _DOLPHIN_CARDCREATE_H_

s32 CARDCreateAsync(s32 chan, const char* fileName, u32 size, CARDFileInfo* fileInfo, CARDCallback callback);
#ifdef MELEE_NATIVE
s32 CARDCreate(s32 chan, char * fileName, u32 size, struct CARDFileInfo * fileInfo);
#else
long CARDCreate(long chan, char * fileName, unsigned long size, struct CARDFileInfo * fileInfo);
#endif

#endif // _DOLPHIN_CARDCREATE_H_
