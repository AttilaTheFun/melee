#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef __assert
#include "../../src/sysdolphin/baselib/axdriver.c"

/* Only callback registration is captured: the test calls the exact callbacks
 * registered by the real driver. All four native effects are selected. */
static void (*callbacks[2])(void*,void*);
static void* contexts[2];
void AXRegisterAuxACallback(void (*callback)(void*,void*),void* context)
{ callbacks[0]=callback; contexts[0]=context; }
void AXRegisterAuxBCallback(void (*callback)(void*,void*),void* context)
{ callbacks[1]=callback; contexts[1]=context; }
union Effect {
    struct AXFX_REVERBHI hi;
    struct AXFX_REVERBSTD standard;
    struct AXFX_DELAY delay;
    struct AXFX_CHORUS chorus;
};
static void test_effect(AXDriverAuxType type)
{
    union Effect parameters={0};
    assert(AXDriver_8038E37C(type,&parameters));
    s32 required=HSD_AudioGetAuxHeapSize(type,&parameters);
    assert(required>0);
    unsigned char* storage[2];
    for(unsigned channel=0;channel<2;++channel) {
        assert(!posix_memalign((void**)&storage[channel],32,(size_t)required+64));
        assert((uintptr_t)storage[channel]>UINT32_MAX);
        memset(storage[channel],0xA7,(size_t)required+64);
        assert(AXDriver_8038E30C(channel,type,&parameters,storage[channel]+32,required));
        assert(callbacks[channel] && contexts[channel]);
        assert(axfxallocsize==(u32)required); /* Exact fit, not a padded workspace. */
        assert(!AXDriverAlloc(1) && !AXDriverAlloc(SIZE_MAX));
        assert(axfxallocsize==(u32)required);
        void (*saved)(void*,void*)=callbacks[channel];
        assert(!AXDriver_8038E30C(channel,type,&parameters,storage[channel]+32,required-1));
        assert(!AXDriver_8038E30C(channel,type,&parameters,NULL,required));
        assert(!AXDriver_8038E30C(channel,type,&parameters,storage[channel]+33,required));
        assert(!AXDriver_8038E30C(channel,type,&parameters,storage[channel]+32,(size_t)UINT32_MAX+1));
        assert(callbacks[channel]==saved);
    }
    unsigned nonzero[2]={0};
    for(unsigned frame=0;frame<120;++frame) {
        for(unsigned channel=0;channel<2;++channel) {
            s32 left[160]={0},right[160]={0},surround[160]={0};
            struct AXFX_BUFFERUPDATE buffers={left,right,surround};
            if(frame==0) left[0]=right[0]=surround[0]=0x100000*(channel+1);
            callbacks[channel](&buffers,contexts[channel]);
            for(unsigned i=0;i<160;++i)
                if(left[i] || right[i] || surround[i]) ++nonzero[channel];
        }
    }
    assert(nonzero[0] && nonzero[1]);
    assert(AXDriver_8038E30C(0,AXDRIVER_AUX_OFF,NULL,NULL,0));
    assert(!callbacks[0] && !contexts[0] && callbacks[1]);
    s32 l[160]={1},r[160]={2},s[160]={3};
    struct AXFX_BUFFERUPDATE buffers={l,r,s};
    callbacks[1](&buffers,contexts[1]);
    assert(AXDriver_8038E30C(1,AXDRIVER_AUX_OFF,NULL,NULL,0));
    assert(!callbacks[1] && !contexts[1]);
    for(unsigned channel=0;channel<2;++channel) {
        for(unsigned i=0;i<32;++i) {
            assert(storage[channel][i]==0xA7);
            assert(storage[channel][32+required+i]==0xA7);
        }
        free(storage[channel]);
    }
}
int main(void)
{
    AXFXSetHooks(AXDriverAlloc,AXDriverFree);
    assert(!AXDriverAlloc(1));
    test_effect(AXDRIVER_AUX_CHORUS);
    test_effect(AXDRIVER_AUX_DELAY);
    test_effect(AXDRIVER_AUX_REVERB_STD);
    test_effect(AXDRIVER_AUX_REVERB_HI);
    struct AXFX_DELAY delay={.delay={UINT32_MAX,6,6}};
    assert(!HSD_AudioGetAuxHeapSize(AXDRIVER_AUX_DELAY,&delay));
    delay.delay[0]=5;
    assert(!HSD_AudioGetAuxHeapSize(AXDRIVER_AUX_DELAY,&delay));
    struct AXFX_REVERBSTD standard={.preDelay=NAN};
    assert(!HSD_AudioGetAuxHeapSize(AXDRIVER_AUX_REVERB_STD,&standard));
    standard.preDelay=INFINITY;
    assert(!HSD_AudioGetAuxHeapSize(AXDRIVER_AUX_REVERB_STD,&standard));
    assert(!AXDriver_8038E30C(-1,AXDRIVER_AUX_OFF,NULL,NULL,0));
    assert(!AXDriver_8038E30C(2,AXDRIVER_AUX_OFF,NULL,NULL,0));
    puts("Original AX driver: exact-fit native workspaces, four real effects on both auxiliary channels, typed callbacks, bounds rejection and canaries passed");
}
