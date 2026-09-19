#ifndef _DOLPHIN_CARDWRITE_H_
#define _DOLPHIN_CARDWRITE_H_

#ifdef MELEE_NATIVE
s32 CARDWriteAsync(struct CARDFileInfo * fileInfo, void * buf, s32 length, s32 offset, CARDCallback callback);
#else
long CARDWriteAsync(struct CARDFileInfo * fileInfo, void * buf, long length, long offset, void (* callback)(long, long));
#endif
#ifdef MELEE_NATIVE
s32 CARDWrite(struct CARDFileInfo * fileInfo, void * buf, s32 length, s32 offset);
#else
long CARDWrite(struct CARDFileInfo * fileInfo, void * buf, long length, long offset);
#endif

#endif // _DOLPHIN_CARDWRITE_H_
