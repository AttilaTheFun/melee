// Bridge SDK configuration to the pinned Aurora shadow state. The original
// hardware-only dummy-primitive flush is already adapted inside Aurora.
#include "lib/dolphin/gx/__gx.h"
#include <cstdio>
#include <cstdlib>

extern "C" void GXSetMisc(GXMiscToken token, u32 value) {
  switch (token) {
  case GX_MT_XF_FLUSH:
    __gx->vNum = static_cast<u16>(value);
    __gx->bpSent = 1;
    if (__gx->vNum != 0) __gx->dirtyState |= 8;
    break;
  case GX_MT_DL_SAVE_CONTEXT:
    if (__gx->inDispList) {
      std::fputs("GXSetMisc: cannot change context preservation inside a display list\n", stderr);
      std::abort();
    }
    __gx->dlSaveContext = value > 0;
    break;
  case GX_MT_NULL:
    break;
  default:
    std::fputs("GXSetMisc: unsupported token\n", stderr);
    std::abort();
  }
}

// GALE01 rev2 at 0x80340518 contains only 0x4e800020 (blr). This operation
// has no effect on the original hardware release, not an unimplemented bridge.
extern "C" void GXSetTevClampMode(int stage, int mode) {
  (void)stage;
  (void)mode;
}
