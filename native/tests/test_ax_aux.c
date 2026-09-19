#include "melee_ax_aux.h"
#include "../../extern/dolphin/src/dolphin/ax/__ax.h"
#include <dolphin/axfx.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Context { unsigned calls; int gain; s32 received[3][16]; };
static void amplify(void* opaque,void* user)
{
    struct AX_AUX_DATA* channels=opaque;
    struct Context* context=user;
    assert((uintptr_t)channels->l>UINT32_MAX && !((uintptr_t)channels->l&31));
    assert(channels->r==channels->l+160 && channels->s==channels->l+320);
    s32* values[]={channels->l,channels->r,channels->s};
    for(unsigned c=0;c<3;++c) {
        if(context->calls<16) context->received[c][context->calls]=values[c][0];
        for(unsigned i=0;i<160;++i) values[c][i]*=context->gain;
    }
    ++context->calls;
    MeleeAXMixBuffers sentinel; memset(&sentinel,0xA5,sizeof(sentinel));
    MeleeAXMixBuffers saved=sentinel;
    assert(!melee_ax_aux_process_frame(&sentinel));
    assert(!melee_ax_aux_init());
    assert(!memcmp(&sentinel,&saved,sizeof(sentinel)));
}
static void delay_callback(void* opaque,void* effect)
{
    struct AXFX_BUFFERUPDATE channels;
    memcpy(&channels,opaque,sizeof(channels));
    AXFXDelayCallback(&channels,effect);
}
static s32 wrapped(int64_t value)
{
    if(value>INT32_MAX) value-=INT64_C(4294967296);
    if(value<INT32_MIN) value+=INT64_C(4294967296);
    return value;
}
int main(void)
{
    MeleeAXMixBuffers buffers={0};
    assert(!melee_ax_aux_process_frame(&buffers));
    assert(!melee_ax_aux_process_frame(NULL));
    assert(melee_ax_aux_init());
    struct Context a={.gain=2},b={.gain=3};
    AXRegisterAuxACallback(amplify,&a); AXRegisterAuxBCallback(amplify,&b);
    AXAuxAddress input,output;
    __AXGetAuxAInput(&input); __AXGetAuxAOutput(&output);
    assert(input>UINT32_MAX && output>UINT32_MAX && input!=output);
    for(unsigned frame=0;frame<10;++frame) {
        for(unsigned c=0;c<3;++c) for(unsigned i=0;i<160;++i) {
            buffers.main[c][i]=i%2?INT32_MIN+5:INT32_MAX-5;
            buffers.auxA[c][i]=(frame+1)*100+c*10+i%7;
            buffers.auxB[c][i]=-((int)(frame+1)*50+(int)c*3+(int)(i%5));
        }
        assert(melee_ax_aux_process_frame(&buffers));
        for(unsigned c=0;c<3;++c) for(unsigned i=0;i<160;++i) {
            int64_t expected=i%2?INT32_MIN+5:INT32_MAX-5;
            if(frame>=2) {
                expected+=2*((frame-1)*100+c*10+i%7);
                expected-=3*((frame-1)*50+c*3+i%5);
            }
            assert(buffers.main[c][i]==wrapped(expected));
        }
        assert(a.calls==frame+1 && b.calls==frame+1);
        for(unsigned c=0;c<3;++c) {
            assert(a.received[c][frame]==(frame?(s32)(frame*100+c*10):0));
            assert(b.received[c][frame]==(frame?-(s32)(frame*50+c*3):0));
        }
    }
    /* Preserve the original B transfer gate: B alone gets no new DSP input. */
    assert(melee_ax_aux_init()); memset(&b,0,sizeof(b)); b.gain=3;
    AXRegisterAuxBCallback(amplify,&b); __AXGetAuxBInput(&input); assert(!input);
    for(unsigned frame=0;frame<5;++frame) {
        memset(&buffers,0,sizeof(buffers)); buffers.auxB[0][0]=1234;
        assert(melee_ax_aux_process_frame(&buffers)); assert(!buffers.main[0][0]);
    }
    for(unsigned i=0;i<5;++i) assert(!b.received[0][i]);
    /* A alone still transfers unprocessed B, as the original command list does. */
    assert(melee_ax_aux_init()); memset(&a,0,sizeof(a)); a.gain=2;
    AXRegisterAuxACallback(amplify,&a);
    for(unsigned frame=0;frame<5;++frame) {
        memset(&buffers,0,sizeof(buffers)); buffers.auxB[0][0]=1234;
        assert(melee_ax_aux_process_frame(&buffers));
        assert(buffers.main[0][0]==(frame>=2?1234:0));
    }
    /* Real SDK delay: one effect block plus two auxiliary ring blocks. */
    assert(melee_ax_aux_init());
    AXFXSetHooks(malloc,free);
    struct AXFX_DELAY effect={.delay={6,6,6},.output={100,100,100}};
    assert(AXFXDelayInit(&effect)); AXRegisterAuxACallback(delay_callback,&effect);
    for(unsigned frame=0;frame<8;++frame) {
        memset(&buffers,0,sizeof(buffers));
        if(!frame) for(unsigned c=0;c<3;++c) buffers.auxA[c][0]=(c+1)*1000;
        assert(melee_ax_aux_process_frame(&buffers));
        for(unsigned c=0;c<3;++c) for(unsigned i=0;i<160;++i)
            assert(buffers.main[c][i]==(frame==3 && i==0?(s32)((c+1)*1000):0));
    }
    AXRegisterAuxACallback(NULL,NULL); AXFXDelayShutdown(&effect);
    /* A mode failure must not rotate the ring or call an effect. */
    assert(melee_ax_aux_init()); memset(&a,0,sizeof(a)); a.gain=1;
    AXRegisterAuxACallback(amplify,&a);
    __AXGetAuxAInput(&input); MeleeAXMixBuffers saved=buffers;
    __AXClMode=4; assert(!melee_ax_aux_process_frame(&buffers)); __AXClMode=0;
    __AXGetAuxAInput(&output); assert(input==output && !a.calls);
    assert(!memcmp(&buffers,&saved,sizeof(buffers)));
    /* Reset after nonzero traffic clears every ring slot, not just slot zero. */
    for(unsigned frame=0;frame<3;++frame) {
        memset(&buffers,0x11,sizeof(buffers)); assert(melee_ax_aux_process_frame(&buffers));
    }
    assert(melee_ax_aux_init()); AXRegisterAuxACallback(amplify,&a);
    for(unsigned frame=0;frame<4;++frame) {
        memset(&buffers,0,sizeof(buffers)); assert(melee_ax_aux_process_frame(&buffers));
        for(unsigned c=0;c<3;++c) for(unsigned i=0;i<160;++i) assert(!buffers.main[c][i]);
    }
    puts("AX auxiliary integration: native pointers, three-slot timing, original A/B gates, real delay return, reset and reentrancy passed");
}
