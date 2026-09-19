#include "melee_ax_mix.h"
#include "melee_ax_voice.h"
#include <dolphin/ar.h>
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition=PTHREAD_COND_INITIALIZER;
static int completed;
static _Alignas(32) unsigned char encoded[320];
static const s16 waveform[8]={-32768,-12345,-1,0,1,8192,23456,32767};
static void done(ARQRequest* unused)
{
    (void)unused; pthread_mutex_lock(&mutex); completed=1;
    pthread_cond_signal(&condition); pthread_mutex_unlock(&mutex);
}
static int scaled(int sample,int volume)
{
    int64_t product=(int64_t)sample*volume;
    int64_t result=product/32768;
    if(product<0 && product%32768) --result;
    if(result< -32768) result= -32768;
    if(result>32767) result=32767;
    return (int)result;
}
static int signed16(unsigned value) { return value<32768?(int)value:(int)value-65536; }
static int32_t add32(int32_t a,int b)
{
    int64_t sum=(int64_t)a+b;
    if(sum>INT32_MAX) sum-=INT64_C(4294967296);
    if(sum<INT32_MIN) sum+=INT64_C(4294967296);
    return (int32_t)sum;
}
static s32* bus(MeleeAXMixBuffers* buffers,unsigned channel)
{
    if(channel<3) return buffers->main[channel];
    if(channel<6) return buffers->auxA[channel-3];
    return buffers->auxB[channel-6];
}
static void fails(AXPB* pb,MeleeAXMixBuffers* buffers,unsigned ms)
{
    AXPB before=*pb; MeleeAXMixBuffers snapshot=*buffers;
    assert(!melee_ax_mix_voice_ms(pb,buffers,ms));
    assert(!memcmp(&before,pb,sizeof(before)));
    assert(!memcmp(&snapshot,buffers,sizeof(snapshot)));
}
int main(void)
{
    u32 stack[1]; ARInit(stack,1); u32 base=ARAlloc(sizeof(encoded));
    for(unsigned i=0;i<160;++i) {
        u16 value=(u16)waveform[i%8]; encoded[2*i]=value>>8; encoded[2*i+1]=value;
    }
    ARRegisterDMACallback(done);
    ARStartDMA(ARAM_DIR_MRAM_TO_ARAM,(ARAddress)encoded,base,sizeof(encoded));
    struct timespec deadline; clock_gettime(CLOCK_REALTIME,&deadline); deadline.tv_sec+=10;
    pthread_mutex_lock(&mutex);
    while(!completed) assert(!pthread_cond_timedwait(&condition,&mutex,&deadline));
    pthread_mutex_unlock(&mutex);
    melee_ax_voice_pool_init(); AXVPB* voice=AXAcquireVoice(1,NULL,0); assert(voice);
    AXPBADDR address={.format=10,.loopFlag=1};
    address.currentAddressHi=address.loopAddressHi=(base/2)>>16;
    address.currentAddressLo=address.loopAddressLo=base/2;
    address.endAddressHi=(base/2+159)>>16; address.endAddressLo=base/2+159;
    AXSetVoiceAddr(voice,&address); AXSetVoiceSrcType(voice,AX_SRC_TYPE_NONE);
    AXSetVoiceState(voice,1);
    AXPBMIX mix={.vL=0x8000,.vR=0xFFFF,.vS=0x3210,
        .vAuxAL=0x4321,.vAuxAR=0x5432,.vAuxAS=0x6543,
        .vAuxBL=0x7654,.vAuxBR=0x8765,.vAuxBS=0x9876,
        .vDeltaL=3,.vDeltaR=0xFFFD,.vDeltaS=0xB001,
        .vDeltaAuxAL=0xFF00,.vDeltaAuxAR=0x1234,.vDeltaAuxAS=0x7000,
        .vDeltaAuxBL=0xFFFF,.vDeltaAuxBR=1,.vDeltaAuxBS=0xFFFF};
    AXSetVoiceMix(voice,&mix); assert(voice->pb.mixerCtrl==15);
    AXPB initial=voice->pb;
    /* Reference layout indices come from the original PB, not the mixer's
     * routing implementation. All channel volumes/deltas are distinct. */
    unsigned volume_word[9]={0,2,14,4,6,16,8,10,12};
    unsigned depop_word[9]={0,3,6,1,4,7,2,5,8};
    unsigned required_flags[9]={0,0,4,1,1,5,2,2,6};
    u16 envelope_start[]={0x7FFF,0x8000,0xFFFF,0,0xF000,0x1000};
    s16 envelope_delta[]={0,0,-1,1,0x1234,-0x2345};
    for(unsigned flags=0;flags<16;++flags) for(unsigned ramp=0;ramp<6;++ramp) {
        AXPB pb=initial; pb.mixerCtrl=flags;
        pb.ve.currentVolume=envelope_start[ramp]; pb.ve.currentDelta=envelope_delta[ramp];
        memset(&pb.dpop,0x55,sizeof(pb.dpop));
        MeleeAXMixBuffers actual,expected;
        for(unsigned c=0;c<9;++c) for(unsigned i=0;i<160;++i)
            bus(&actual,c)[i]=(i%2)?INT32_MIN+17:INT32_MAX-17;
        expected=actual;
        u16 mix_words[18]; memcpy(mix_words,&pb.mix,sizeof(mix_words));
        s16 last_words[9]; memcpy(last_words,&pb.dpop,sizeof(last_words));
        unsigned envelope=pb.ve.currentVolume;
        for(unsigned ms=0;ms<5;++ms) {
            for(unsigned i=ms*32;i<(ms+1)*32;++i) {
                int input=scaled(waveform[i%8],signed16(envelope));
                envelope=(envelope+(unsigned)envelope_delta[ramp])&65535;
                for(unsigned c=0;c<9;++c) if((flags&required_flags[c])==required_flags[c]) {
                    unsigned index=volume_word[c];
                    int contribution=scaled(input,mix_words[index]);
                    bus(&expected,c)[i]=add32(bus(&expected,c)[i],contribution);
                    if(flags&8) mix_words[index]=(u16)(mix_words[index]+mix_words[index+1]);
                    last_words[depop_word[c]]=contribution;
                }
            }
            assert(melee_ax_mix_voice_ms(&pb,&actual,ms));
            assert(!memcmp(&actual,&expected,sizeof(actual)));
            assert(pb.ve.currentVolume==envelope);
            assert(!memcmp(&pb.mix,mix_words,sizeof(mix_words)));
            assert(!memcmp(&pb.dpop,last_words,sizeof(last_words)));
        }
        assert(pb.state==1 && pb.addr.currentAddressHi==address.loopAddressHi &&
               pb.addr.currentAddressLo==address.loopAddressLo);
    }
    /* Two voices accumulate, rather than replacing each other. */
    MeleeAXMixBuffers buffers={0}; AXPB pb=initial;
    pb.mixerCtrl=0; memset(&pb.mix,0,sizeof(pb.mix));
    pb.mix.vL=0x8000; pb.mix.vR=0x4000; pb.ve.currentVolume=0x4000;
    AXPB second=pb;
    assert(melee_ax_mix_voice_ms(&pb,&buffers,2));
    assert(melee_ax_mix_voice_ms(&second,&buffers,2));
    assert(buffers.main[0][64]==-32768 && buffers.main[1][64]==-16384);
    assert(buffers.main[0][66]==-2 && buffers.main[1][66]==-2);
    assert(buffers.main[0][71]==32766 && buffers.main[1][71]==16382);
    for(unsigned i=0;i<160;++i) if(i<64 || i>=96)
        assert(!buffers.main[0][i] && !buffers.main[1][i]);
    /* Finishing a source completes this quantum, then later quanta leave all
     * envelope, ramp and bus state unchanged. */
    pb=initial; pb.addr.loopFlag=0; pb.addr.endAddressHi=(base/2+2)>>16;
    pb.addr.endAddressLo=base/2+2; pb.ve.currentVolume=0x7FFF; pb.ve.currentDelta=-5;
    assert(melee_ax_mix_voice_ms(&pb,&buffers,0)); assert(!pb.state);
    AXPB stopped=pb; MeleeAXMixBuffers saved=buffers;
    assert(melee_ax_mix_voice_ms(&pb,&buffers,1));
    assert(!memcmp(&pb,&stopped,sizeof(pb)) && !memcmp(&buffers,&saved,sizeof(buffers)));
    for(unsigned variant=0;variant<7;++variant) {
        pb=initial; pb.ve.currentVolume=0x7FFF;
        if(variant==0) pb.itd.flag=1;
        if(variant==1) pb.fir.numCoefs=1;
        if(variant==2) pb.mixerCtrl=0x10;
        if(variant==3) pb.state=2;
        if(variant==4) pb.addr.format=99;
        if(variant==5) {
            u32 position=ARGetSize()/2-1;
            pb.addr.currentAddressHi=position>>16; pb.addr.currentAddressLo=position;
            pb.addr.endAddressHi=pb.addr.endAddressLo=0xFFFF;
        }
        fails(&pb,&buffers,variant==6?5:0);
    }
    assert(!melee_ax_mix_voice_ms(NULL,&buffers,0));
    assert(!melee_ax_mix_voice_ms(&pb,NULL,0));
    /* Scheduled start, gain change, stop and restart happen on sample
     * boundaries 0, 32, 64 and 128, including updates to an inactive voice. */
    memset(&buffers,0,sizeof(buffers)); pb=initial; pb.state=0;
    pb.mixerCtrl=0; memset(&pb.mix,0,sizeof(pb.mix));
    pb.mix.vL=0x8000; pb.ve.currentVolume=0x4000;
    u16 commands[]={7,1, 50,0x2000, 7,0, 7,1};
    pb.update.updNum[0]=pb.update.updNum[1]=pb.update.updNum[2]=pb.update.updNum[4]=1;
    assert(melee_ax_mix_voice_frame(&pb,commands,8,&buffers));
    for(unsigned i=0;i<160;++i) {
        int expected=i<32?scaled(waveform[i%8],0x4000):
            (i<64 || i>=128)?scaled(waveform[i%8],0x2000):0;
        assert(buffers.main[0][i]==expected && !buffers.main[1][i]);
    }
    assert(pb.state==1 && pb.addr.currentAddressLo==(u16)(base/2+96));
    /* A failure in the last quantum must not leak any earlier mixed samples. */
    for(unsigned variant=0;variant<5;++variant) {
        pb=initial; pb.ve.currentVolume=0x7FFF; pb.update.updNum[4]=1;
        u16 invalid[]={variant==0?96:variant==1?6:variant==2?52:7,
                       variant==1?0x10:variant==2?1:0};
        if(variant==3) pb.update.updNum[4]=2;
        if(variant==4) pb.update.updNum[0]=65535;
        AXPB before=pb; saved=buffers;
        assert(!melee_ax_mix_voice_frame(&pb,invalid,2,&buffers));
        assert(!memcmp(&before,&pb,sizeof(pb)) && !memcmp(&saved,&buffers,sizeof(buffers)));
    }
    pb=initial; saved=buffers;
    assert(!melee_ax_mix_voice_frame(&pb,commands,129,&buffers));
    assert(!melee_ax_mix_voice_frame(&pb,commands,1,&buffers));
    assert(!melee_ax_mix_voice_frame(&pb,NULL,2,&buffers));
    assert(!memcmp(&saved,&buffers,sizeof(buffers)));
    AXFreeVoice(voice); ARReset();
    puts("AX voice mixing: all 16 routing/ramp modes, signed envelopes, channel clipping, 32-bit accumulation, depop state and transactional failures passed");
}
