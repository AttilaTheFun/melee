#include <dolphin/gx.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
extern void aurora_gx_shadow(u32 state[5]);

int main(void)
{
    u32 before[5], after[5];
    aurora_gx_shadow(before);
    GXSetMisc(GX_MT_NULL, 123);
    GXSetTevClampMode(0, 0);
    aurora_gx_shadow(after);
    assert(!memcmp(before,after,sizeof(before)));
    GXSetMisc(GX_MT_XF_FLUSH,8);
    aurora_gx_shadow(after);
    assert(after[0]==8 && after[1]==1 && (after[2]&8));
    GXSetMisc(GX_MT_XF_FLUSH,0);
    aurora_gx_shadow(after);
    assert(after[0]==0 && after[1]==1);
    unsigned char list[8192] __attribute__((aligned(32)));
    GXSetMisc(GX_MT_DL_SAVE_CONTEXT,1);
    GXBeginDisplayList(list,sizeof(list));
    aurora_gx_shadow(before);
    GXSetCullMode(GX_CULL_FRONT);
    assert(GXEndDisplayList()>0);
    aurora_gx_shadow(after);
    assert(after[3]==1 && after[4]==before[4]);
    GXSetMisc(GX_MT_DL_SAVE_CONTEXT,0);
    GXBeginDisplayList(list,sizeof(list));
    GXSetCullMode(GX_CULL_BACK);
    assert(GXEndDisplayList()>0);
    aurora_gx_shadow(after);
    assert(after[3]==0 && after[4]!=before[4]);
    assert(GXNtsc480Prog.viTVmode==2 && GXNtsc480Prog.fbWidth==640);
    assert(GXNtsc480Prog.efbHeight==480 && GXNtsc480Prog.xfbHeight==480);
    assert(GXNtsc480Prog.viXOrigin==40 && GXNtsc480Prog.viYOrigin==0);
    assert(GXNtsc480Prog.viWidth==640 && GXNtsc480Prog.viHeight==480);
    assert(!GXNtsc480Prog.xFBmode && !GXNtsc480Prog.field_rendering && !GXNtsc480Prog.aa);
    for (unsigned i=0;i<12;i++) for(unsigned j=0;j<2;j++) assert(GXNtsc480Prog.sample_pattern[i][j]==6);
    const unsigned char filter[7]={0,0,21,22,21,0,0};
    assert(!memcmp(filter,GXNtsc480Prog.vfilter,7));
    GXSetDispCopySrc(0,0,640,240);
    assert(GXSetDispCopyYScale(2.0f)==480);
    GXSetDispCopySrc(0,0,640,480);
    assert(GXSetDispCopyYScale(1.5f)==722); /* SDK's reciprocal is quantized to 170/256. */
    assert(GXSetDispCopyYScale(1.0f)==480);
    GXSetDispCopyDst(640,480);
    GXSetCopyClamp((GXFBClamp)(GX_CLAMP_TOP|GX_CLAMP_BOTTOM));
    puts("Aurora GX misc: original display-list context preservation and progressive mode passed without a window/GPU");
    return 0;
}
