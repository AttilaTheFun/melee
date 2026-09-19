#include "melee_ax_depop.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    MeleeAXDepop state={0}; MeleeAXMixBuffers buffers;
    /* Channel mapping must follow PB field names, not their unusual order. */
    AXPBDPOP last={.aL=160,.aR=-320,.aS=480,
        .aAuxAL=-640,.aAuxAR=800,.aAuxAS=-960,
        .aAuxBL=1120,.aAuxBR=-1280,.aAuxBS=1440};
    AXPBDPOP preserved=last;
    assert(melee_ax_depop_add(&state,&last));
    assert(!memcmp(&last,&preserved,sizeof(last)));
    for(unsigned c=0;c<3;++c) {
        assert(state.main[c]==(c==1?-1:1)*(int)(c+1)*160);
        assert(state.auxA[c]==(c==1?1:-1)*(int)(c+4)*160);
        assert(state.auxB[c]==(c==1?-1:1)*(int)(c+7)*160);
    }
    assert(melee_ax_depop_add(&state,&last));
    assert(melee_ax_depop_begin_frame(&state,&buffers));
    for(unsigned c=0;c<3;++c) for(unsigned i=0;i<160;++i) {
        assert(buffers.main[c][i]==(c==1?-1:1)*(int)(c+1)*2*(160-(int)i));
        assert(buffers.auxA[c][i]==(c==1?1:-1)*(int)(c+4)*2*(160-(int)i));
        assert(buffers.auxB[c][i]==(c==1?-1:1)*(int)(c+7)*2*(160-(int)i));
    }
    MeleeAXDepop zero={0}; assert(!memcmp(&state,&zero,sizeof(zero)));
    /* Hand-selected truncation and slope-limit boundaries in both signs. */
    s32 initial[]={0,1,159,160,161,319,320,3199,3200,3359,3360,32767,INT32_MAX};
    s32 steps[]={0,0,0,1,1,1,2,19,20,20,20,20,20};
    for(unsigned j=0;j<sizeof(initial)/sizeof(initial[0]);++j)
    for(int sign=-1;sign<=1;sign+=2) {
        state=(MeleeAXDepop){0}; state.main[0]=initial[j]*sign;
        memset(&buffers,0xA7,sizeof(buffers));
        assert(melee_ax_depop_begin_frame(&state,&buffers));
        for(unsigned i=0;i<160;++i) {
            assert(buffers.main[0][i]==(steps[j]?sign*(initial[j]-(s32)i*steps[j]):0));
            for(unsigned c=0;c<3;++c) {
                assert(!buffers.auxA[c][i] && !buffers.auxB[c][i]);
                if(c) assert(!buffers.main[c][i]);
            }
        }
        assert(state.main[0]==(steps[j]?sign*(initial[j]-160*steps[j]):0));
    }
    state.main[0]=INT32_MIN;
    assert(melee_ax_depop_begin_frame(&state,&buffers));
    assert(buffers.main[0][0]==INT32_MIN && buffers.main[0][159]==INT32_MIN+3180);
    assert(state.main[0]==INT32_MIN+3200);
    /* 32767 fades by 3200 per frame, then by 640, discarding remainder 127. */
    state=(MeleeAXDepop){0}; state.main[0]=32767;
    for(unsigned frame=0;frame<10;++frame) {
        assert(melee_ax_depop_begin_frame(&state,&buffers));
        assert(buffers.main[0][0]==32767-(s32)frame*3200);
    }
    assert(state.main[0]==767);
    assert(melee_ax_depop_begin_frame(&state,&buffers));
    assert(buffers.main[0][159]==131 && state.main[0]==127);
    assert(melee_ax_depop_begin_frame(&state,&buffers));
    assert(!state.main[0] && !buffers.main[0][0]);
    /* Multiple retirement sums have explicit 32-bit wrap semantics. */
    state.main[0]=INT32_MAX; state.main[1]=INT32_MIN;
    last=(AXPBDPOP){.aL=1,.aR=-1};
    assert(melee_ax_depop_add(&state,&last));
    assert(state.main[0]==INT32_MIN && state.main[1]==INT32_MAX);
    MeleeAXDepop saved=state; MeleeAXMixBuffers snapshot=buffers;
    assert(!melee_ax_depop_add(&state,NULL)); assert(!melee_ax_depop_add(NULL,&last));
    assert(!melee_ax_depop_begin_frame(&state,NULL));
    assert(!melee_ax_depop_begin_frame(NULL,&buffers));
    assert(!memcmp(&saved,&state,sizeof(state)) && !memcmp(&snapshot,&buffers,sizeof(buffers)));
    puts("AX depop: nine-channel sums, signed fade boundaries, multi-frame decay, bus initialization and 32-bit wrapping passed");
}
