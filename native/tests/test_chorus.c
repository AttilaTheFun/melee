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
#undef __assert
#include "../../extern/dolphin/src/dolphin/axfx/chorus.c"
#include "chorus_src1_reference.inc"
#include "chorus_src2_reference.inc"
static unsigned live;
static int fail_allocation;
static void* allocate(unsigned long bytes)
{ if(fail_allocation) return NULL; void* p=malloc(bytes); assert(p); ++live; return p; }
static void release(void* p) { assert(p && live); --live; free(p); }
static void compare_src(unsigned whole, uint32_t increment)
{
    s32 samples[480], old[4]={INT32_MAX,INT32_MIN,0,0}, output[160];
    uint32_t random=0x427ABC;
    for(unsigned i=0;i<480;++i) {
        random=random*1664525u+1013904223u;
        memcpy(&samples[i],&random,4);
    }
    struct AXFX_CHORUS_SRCINFO src={.dest=output,.smpBase=samples,.old=old,
        .posLo=0xFEDCBA98,.posHi=479,.pitchLo=increment,.pitchHi=whole,.trigger=480,.target=0};
    memset(memory,0,sizeof(memory));
    ref_put_word(0x1000,0x2000); ref_put_word(0x1004,0x10000); ref_put_word(0x1008,0x3000);
    ref_put_word(0x100c,src.posLo); ref_put_word(0x1010,src.posHi);
    ref_put_word(0x1014,increment); ref_put_word(0x1018,whole);
    ref_put_word(0x101c,480); ref_put_word(0x1020,0);
    for(unsigned i=0;i<4;++i) ref_put_word(0x3000+i*4,(uint32_t)old[i]);
    for(unsigned i=0;i<480;++i) ref_put_word(0x10000+i*4,(uint32_t)samples[i]);
    for(unsigned i=0;i<512;++i) ref_put_float(0x4000+i*4,rsmpTab12khz[i]);
    for(unsigned frame=0;frame<60;++frame) {
        if(whole) { reference_src2(); do_src2(&src); }
        else { reference_src1(); do_src1(&src); }
        for(unsigned i=0;i<160;++i) {
            uint32_t actual; memcpy(&actual,&output[i],4);
            if(actual!=ref_word(0x2000+i*4)) {
                fprintf(stderr,"Chorus SRC mismatch step=%u increment=%x frame=%u sample=%u\n",whole,increment,frame,i);
                abort();
            }
        }
        assert(src.posLo==ref_word(0x100c) && src.posHi==ref_word(0x1010));
        for(unsigned i=0;i<3;++i) assert((uint32_t)old[i]==ref_word(0x3000+4*i));
    }
}
int main(void)
{
    uint32_t increments[]={0,1,0x7FFFFFFF,0x80000000,0xFFFFFFFF,0x12345678};
    for(unsigned whole=0;whole<2;++whole)
        for(unsigned i=0;i<6;++i) compare_src(whole,increments[i]);
    AXFXSetHooks(allocate,release);
    struct AXFX_CHORUS chorus={.baseDelay=15,.variation=1,.period=20};
    assert(AXFXChorusInit(&chorus) && live==1);
    unsigned negative=0,positive=0,nonzero=0;
    for(unsigned frame=0;frame<100;++frame) {
        s32 l[160]={0},r[160]={0},s[160]={0};
        struct AXFX_BUFFERUPDATE buffers={l,r,s};
        l[0]=1000000; r[1]=-500000; s[2]=200000;
        AXFXChorusCallback(&buffers,&chorus);
        if(chorus.work.pitchOffset<0) ++negative; else ++positive;
        for(unsigned i=0;i<160;++i) if(l[i] || r[i] || s[i]) ++nonzero;
    }
    assert(negative && positive && nonzero);
    s32* allocation=chorus.work.lastLeft[0];
    chorus.period=0;
    assert(!AXFXChorusSettings(&chorus) && chorus.work.lastLeft[0]==allocation);
    assert(AXFXChorusShutdown(&chorus) && !live);
    assert(AXFXChorusShutdown(&chorus) && !live);
    chorus.period=20; fail_allocation=1;
    assert(!AXFXChorusInit(&chorus) && !live);
    fail_allocation=0;
    chorus.variation=UINT32_MAX;
    assert(!AXFXChorusInit(&chorus) && !live);
    puts("Chorus: 115200 samples, retained history and phase match both assembly paths; modulation, invalid settings and allocation failure passed");
}
