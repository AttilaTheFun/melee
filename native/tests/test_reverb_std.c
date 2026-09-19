#include <dolphin/axfx.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma STDC FP_CONTRACT OFF
static unsigned char memory[0x400000];
static uint32_t ref_word(uint32_t at)
{
    assert(at <= sizeof(memory) - 4);
    return (uint32_t)memory[at] << 24 | (uint32_t)memory[at+1] << 16 |
        (uint32_t)memory[at+2] << 8 | memory[at+3];
}
static void ref_put_word(uint32_t at, uint32_t value)
{
    assert(at <= sizeof(memory) - 4);
    for (unsigned i=0; i<4; ++i) memory[at+i] = value >> (24-8*i);
}
static float ref_float(uint32_t at)
{ uint32_t bits=ref_word(at); float f; memcpy(&f,&bits,4); return f; }
static void ref_put_float(uint32_t at, float f)
{ uint32_t bits; memcpy(&bits,&f,4); ref_put_word(at,bits); }
static double ref_double(uint32_t at)
{
    uint64_t bits=(uint64_t)ref_word(at)<<32 | ref_word(at+4);
    double value; memcpy(&value,&bits,8); return value;
}
static int32_t ref_integer(double f)
{
    double value=trunc(f);
    if (isnan(value) || value < -2147483648.0) return INT32_MIN;
    if (value >= 2147483648.0) return INT32_MAX;
    return (int32_t)value;
}
#include "reverb_reference.inc"
static unsigned allocations, live, fail_at;
static void* allocate(unsigned long bytes)
{
    if (++allocations == fail_at) return NULL;
    void* p=malloc(bytes); assert(p); ++live; return p;
}
static void release(void* p) { assert(p && live); --live; free(p); }
/* Copy initialized native data into the console's original byte layout. The
 * generated reference then uses only PPC registers and virtual addresses. */
static void reference_init(const struct AXFX_REVSTD_WORK* work)
{
    memset(memory,0,sizeof(memory));
    uint32_t next=0x10000;
    for (unsigned i=0;i<12;++i) {
        const struct AXFX_REVSTD_DELAYLINE* line=i<6 ? &work->AP[i] : &work->C[i-6];
        uint32_t base=0x1000+i*20;
        ref_put_word(base,line->inPoint); ref_put_word(base+4,line->outPoint);
        ref_put_word(base+8,line->length); ref_put_word(base+12,next);
        ref_put_float(base+16,line->lastOutput);
        for (unsigned j=0;j<(unsigned)line->length/4;++j) ref_put_float(next+4*j,line->inputs[j]);
        next+=line->length;
    }
    ref_put_float(0x10f0,work->allPassCoeff);
    for(unsigned i=0;i<6;++i) ref_put_float(0x10f4+4*i,work->combCoef[i]);
    ref_put_float(0x1118,work->level); ref_put_float(0x111c,work->damping);
    ref_put_word(0x1120,work->preDelayTime);
    for(unsigned c=0;c<3;++c) {
        if(work->preDelayTime) {
            ref_put_word(0x1124+4*c,next); ref_put_word(0x1130+4*c,next);
            next+=work->preDelayTime*4;
        }
    }
}
static void compare_case(float predelay,float coloration,float mix)
{
    struct AXFX_REVERBSTD effect={.coloration=coloration,.mix=mix,.time=2.0f,
        .damping=0.5f,.preDelay=predelay};
    assert(AXFXReverbStdInit(&effect));
    assert(effect.rv.C[0].length==1791*4 && effect.rv.C[1].length==2001*4);
    assert(effect.rv.AP[0].length==435*4 && effect.rv.AP[1].length==151*4);
    reference_init(&effect.rv);
    s32 samples[3][176]; /* Deliberately noncontiguous 160-sample channels. */
    struct AXFX_BUFFERUPDATE buffers={samples[0],samples[1],samples[2]};
    uint32_t random=0x279BAC;
    unsigned nonzero=0;
    for(unsigned frame=0;frame<64;++frame) {
        memset(samples,0,sizeof(samples));
        for(unsigned c=0;c<3;++c) for(unsigned i=0;i<160;++i) {
            random=random*1664525u+1013904223u;
            s32 input=frame<4 ? (s32)(random>>8)-0x800000 : 0;
            samples[c][i]=input;
            ref_put_word(0x2000+(c*160+i)*4,(uint32_t)input);
        }
        reference_process();
        AXFXReverbStdCallback(&buffers,&effect);
        for(unsigned c=0;c<3;++c) {
            for(unsigned i=0;i<160;++i) {
                int32_t expected; uint32_t bits=ref_word(0x2000+(c*160+i)*4);
                memcpy(&expected,&bits,4);
                if(samples[c][i]!=expected) {
                    fprintf(stderr,"Reverb mismatch pre=%g color=%g mix=%g frame=%u channel=%u sample=%u: %d != %d\n",predelay,coloration,mix,frame,c,i,samples[c][i],expected);
                    abort();
                }
                if(frame>30 && samples[c][i]) ++nonzero;
            }
            for(unsigned i=160;i<176;++i) assert(samples[c][i]==0);
        }
    }
    if(mix>0) assert(nonzero);
    float* previous=effect.rv.C[0].inputs;
    effect.time=NAN;
    assert(!AXFXReverbStdSettings(&effect) && effect.rv.C[0].inputs==previous);
    effect.time=1;
    assert(AXFXReverbStdSettings(&effect));
    assert(AXFXReverbStdShutdown(&effect) && !live);
    assert(AXFXReverbStdShutdown(&effect) && !live);
}
int main(void)
{
    AXFXSetHooks(allocate,release);
    compare_case(0,0.5f,0.5f); compare_case(0.01f,0.5f,1);
    compare_case(0.1f,0,1); compare_case(0,1,0);
    for(unsigned failure=1;failure<=15;++failure) {
        struct AXFX_REVERBSTD effect={.coloration=0.5f,.mix=0.5f,.time=1,.damping=0.5f,.preDelay=0.1f};
        fail_at=allocations+failure;
        assert(!AXFXReverbStdInit(&effect) && !live && effect.tempDisableFX);
        assert(AXFXReverbStdShutdown(&effect));
    }
    fail_at=0;
    /* Tiny pre-delays must not inherit the original assembly's N=1 overrun. */
    for(unsigned n=0;n<3;++n) {
        struct AXFX_REVERBSTD effect={.time=1,.mix=1,.preDelay=(n+0.25f)/32000.0f};
        assert(AXFXReverbStdInit(&effect));
        s32 l[160]={1},r[160]={2},s[160]={3};
        struct AXFX_BUFFERUPDATE buffers={l,r,s};
        for(unsigned f=0;f<30;++f) AXFXReverbStdCallback(&buffers,&effect);
        AXFXReverbStdShutdown(&effect);
    }
    assert(!live);
    puts("Standard reverb: 122880 samples match original assembly reference; channel strides, tiny pre-delays, validation and all allocation failures passed");
}
