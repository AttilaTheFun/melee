#include "melee_ax_voice.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
extern u32 __AXClMode;
static void expected_abort(int signal_number) { (void)signal_number; _exit(86); }
static void invalid_update(AXVPB* voice,unsigned variant)
{
    switch(variant) {
    case 0: AXSetVoiceSrcRatio(voice,NAN); break;
    case 1: AXSetVoiceSrcRatio(voice,-1); break;
    case 2: AXSetVoiceUpdateWrite(voice,0,0); break;
    case 3: AXSetVoiceUpdateIncrement(voice); break;
    case 4: AXSetVoiceState((AXVPB*)1,1); break;
    }
}
int main(void)
{
    _Static_assert(sizeof(AXPB)==192,"DSP parameter widths stay fixed");
    melee_ax_voice_pool_init();
    AXVPB* voice=AXAcquireVoice(1,NULL,0);
    assert(voice);
    static const u16 flags[2][18]={
        {0,8,0,8,1,8,1,8,2,8,2,8,4,8,4,8,4,8},
        {0,8,0,8,1,8,1,8,16,8,16,8,0,0,0,0,0,8}};
    for(unsigned mode=0;mode<2;++mode) for(unsigned field=0;field<18;++field) {
        u16 words[18]={0}; words[field]=0x8123;
        AXPBMIX mix; memcpy(&mix,words,sizeof(mix));
        __AXClMode=mode?4:0;
        voice->sync=0;
        AXSetVoiceMix(voice,&mix);
        assert(!memcmp(&mix,&voice->pb.mix,sizeof(mix)));
        assert(voice->pb.mixerCtrl==flags[mode][field]);
        assert(voice->sync==(AX_SYNC_FLAG_COPYAXPBMIX|AX_SYNC_FLAG_COPYMXRCTRL));
    }
    for(unsigned type=0;type<5;++type) {
        AXSetVoiceSrcType(voice,type);
        assert(voice->pb.srcSelect==(type<2?2-type:0));
        if(type>=2) assert(voice->pb.coefSelect==type-2);
    }
    float ratios[]={0,0.5f,1,1.5f,4,FLT_MAX};
    u32 fixed[]={0,0x8000,0x10000,0x18000,0x40000,0x40000};
    for(unsigned i=0;i<6;++i) {
        AXSetVoiceSrcRatio(voice,ratios[i]);
        assert(((u32)voice->pb.src.ratioHi<<16|voice->pb.src.ratioLo)==fixed[i]);
    }
    AXPBSRC src={.ratioHi=1,.ratioLo=0x1234,.currentAddressFrac=0xabcd,.last_samples={1,0xffff,3,4}};
    AXSetVoiceSrc(voice,&src);
    assert(!memcmp(&src,&voice->pb.src,sizeof(src)));
    assert(!(voice->sync&AX_SYNC_FLAG_COPYRATIO) && (voice->sync&AX_SYNC_FLAG_COPYSRC));
    for(unsigned format=10;format<=25;format+=15) {
        AXPBADDR address={.loopFlag=1,.format=format,.loopAddressHi=0x1234,.loopAddressLo=0x5678,
            .endAddressHi=0x8765,.endAddressLo=0x4321,.currentAddressHi=0xABC,.currentAddressLo=0xDEF0};
        _Alignas(4) unsigned char bytes[sizeof(address)+2];
        memcpy(bytes+2,&address,sizeof(address));
        memset(&voice->pb.adpcm,0xA7,sizeof(voice->pb.adpcm));
        AXSetVoiceAddr(voice,(AXPBADDR*)(bytes+2));
        assert(!memcmp(&address,&voice->pb.addr,sizeof(address)));
        AXPBADPCM expected={.gain=format==10?0x0800:0x0100};
        assert(!memcmp(&expected,&voice->pb.adpcm,sizeof(expected)));
    }
    AXPBADPCM adpcm={0};
    u16 words[20]; for(unsigned i=0;i<20;++i) words[i]=0x8000+i*71;
    memcpy(&adpcm,words,sizeof(adpcm)); AXSetVoiceAdpcm(voice,&adpcm);
    AXPBADDR adpcm_address={.format=0,.loopAddressLo=2,.endAddressLo=15,.currentAddressLo=3};
    AXSetVoiceAddr(voice,&adpcm_address);
    assert(!memcmp(&adpcm,&voice->pb.adpcm,sizeof(adpcm)));
    AXSetVoiceLoop(voice,1);
    AXSetVoiceLoopAddr(voice,0x12345672); AXSetVoiceEndAddr(voice,0x7654321F);
    AXSetVoiceCurrentAddr(voice,0xFEDCBA93);
    assert(voice->pb.addr.loopAddressHi==0x1234 && voice->pb.addr.loopAddressLo==0x5672);
    assert(voice->pb.addr.endAddressHi==0x7654 && voice->pb.addr.endAddressLo==0x321F);
    assert(voice->pb.addr.currentAddressHi==0xFEDC && voice->pb.addr.currentAddressLo==0xBA93);
    AXPBADPCMLOOP loop={0x18,0xff00,0x1234}; AXSetVoiceAdpcmLoop(voice,&loop);
    assert(!memcmp(&loop,&voice->pb.adpcmLoop,sizeof(loop)));
    AXPBVE ve={0x7123,-17}; AXSetVoiceVe(voice,&ve);
    AXSetVoiceVeDelta(voice,-29);
    assert(voice->pb.ve.currentVolume==ve.currentVolume && voice->pb.ve.currentDelta==-29);
    AXSetVoiceItdTarget(voice,12,23); AXSetVoiceItdOn(voice);
    assert(voice->pb.itd.flag==1 && !voice->pb.itd.targetShiftL && !voice->pb.itd.targetShiftR);
    assert(!(voice->sync&AX_SYNC_FLAG_COPYTSHIFT));
    AXSetVoiceItdTarget(voice,4,5);
    assert(voice->pb.itd.targetShiftL==4 && voice->pb.itd.targetShiftR==5);
    AXSetVoiceType(voice,1); assert(voice->pb.type==1);
    AXPBFIR fir={3,0x1234,0x5678}; AXSetVoiceFir(voice,&fir);
    assert(!memcmp(&fir,&voice->pb.fir,sizeof(fir)));
    AXPBDPOP dpop={.aL=-123,.aR=234,.aS=-345,.aAuxAL=456};
    AXSetVoiceDpop(voice,&dpop);
    assert(!memcmp(&dpop,&voice->pb.dpop,sizeof(dpop)));
    AXSetVoiceState(voice,1); assert(voice->pb.state==1);
    AXSetVoiceState(voice,0); assert(voice->pb.state==0 && voice->depop==1);
    for(unsigned i=0;i<64;++i) AXSetVoiceUpdateWrite(voice,i,0x8000+i);
    for(unsigned i=0;i<64;++i) assert(voice->updateData[2*i]==i && voice->updateData[2*i+1]==0x8000+i);
    for(unsigned i=0;i<4;++i) AXSetVoiceUpdateIncrement(voice);
    for(unsigned variant=0;variant<5;++variant) {
        pid_t child=fork(); assert(child>=0);
        if(!child) { signal(SIGABRT,expected_abort); invalid_update(voice,variant); _exit(99); }
        int status; assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==86);
    }
    AXFreeVoice(voice);
    puts("AX parameters: mixer flags, typed unaligned copies, PCM gains, ADPCM addresses, SRC ratios, volume/ITD, update limits and invalid input rejection passed");
}
