#include <dolphin/gx.h>
#include <cstddef>
extern "C" void aurora_abi_sizes(size_t* sizes) {
    sizes[0]=sizeof(GXTexObj);sizes[1]=sizeof(GXTlutObj);sizes[2]=sizeof(GXLightObj);
    sizes[3]=sizeof(GXColor);sizes[4]=sizeof(GXRenderModeObj);sizes[5]=sizeof(GXVtxAttrFmtList);sizes[6]=sizeof(GXFogAdjTable);
}

#include "lib/gx/fifo.hpp"
extern "C" size_t aurora_fifo_copy(unsigned char* out,size_t capacity) {
    using namespace aurora::gx::fifo::detail;
    if(capacity<sBufferSize)return sBufferSize;
    std::memcpy(out,sBufferData,sBufferSize);return sBufferSize;
}

#include "lib/dolphin/gx/__gx.h"
extern "C" void aurora_gx_shadow(u32 state[5]) {
    state[0]=__gx->vNum;state[1]=__gx->bpSent;state[2]=__gx->dirtyState;
    state[3]=__gx->dlSaveContext;state[4]=__gx->genMode;
}
