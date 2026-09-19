#include "melee_ax_decode.h"
#include "melee_ax_voice.h"
#include <dolphin/ar.h>
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition=PTHREAD_COND_INITIALIZER;
static int completed;
static _Alignas(32) unsigned char encoded[96];
static void done(ARQRequest* unused)
{ (void)unused; pthread_mutex_lock(&mutex); completed=1; pthread_cond_signal(&condition); pthread_mutex_unlock(&mutex); }
static void put_address(AXPB* pb,u32 current,u32 end,u32 loop)
{
    pb->addr.currentAddressHi=current>>16; pb->addr.currentAddressLo=current;
    pb->addr.endAddressHi=end>>16; pb->addr.endAddressLo=end;
    pb->addr.loopAddressHi=loop>>16; pb->addr.loopAddressLo=loop;
}
static void expect(AXPB* pb,s16 expected)
{ s16 sample=1234; assert(melee_ax_decode_sample(pb,&sample)); assert(sample==expected); }
static void failure(AXPB* pb)
{
    AXPB before=*pb; s16 sentinel=0x1234;
    assert(!melee_ax_decode_sample(pb,&sentinel));
    assert(sentinel==0x1234 && !memcmp(pb,&before,sizeof(before)));
}
int main(void)
{
    AXPB missing={.state=1}; missing.addr.format=10; failure(&missing);
    u32 stack[1]; ARInit(stack,1); u32 base=ARAlloc(sizeof(encoded));
    unsigned char pcm16[]={0x80,0x00,0xff,0xff,0x00,0x01,0x7f,0xff};
    memcpy(encoded,pcm16,sizeof(pcm16));
    unsigned char pcm8[]={0x80,0xff,0,0x7f}; memcpy(encoded+16,pcm8,4);
    unsigned char adpcm[]={0,0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,
                          1,0x11,0x11,0x11,0x11,0x11,0x11,0x11};
    memcpy(encoded+32,adpcm,sizeof(adpcm));
    ARRegisterDMACallback(done);
    ARStartDMA(ARAM_DIR_MRAM_TO_ARAM,(ARAddress)encoded,base,sizeof(encoded));
    struct timespec deadline; clock_gettime(CLOCK_REALTIME,&deadline); deadline.tv_sec+=10;
    pthread_mutex_lock(&mutex);
    while(!completed) assert(!pthread_cond_timedwait(&condition,&mutex,&deadline));
    pthread_mutex_unlock(&mutex);
    /* The actual allocator and SDK setters feed the native decoder. */
    melee_ax_voice_pool_init(); AXVPB* voice=AXAcquireVoice(1,NULL,0); assert(voice);
    AXPBADDR address={.format=10};
    address.currentAddressHi=(base/2)>>16; address.currentAddressLo=base/2;
    address.loopAddressHi=address.currentAddressHi; address.loopAddressLo=address.currentAddressLo;
    address.endAddressHi=((base/2)+3)>>16; address.endAddressLo=(base/2)+3;
    AXSetVoiceAddr(voice,&address); AXSetVoiceState(voice,1);
    s16 pcm_expected[]={-32768,-1,1,32767};
    for(unsigned i=0;i<4;++i) expect(&voice->pb,pcm_expected[i]);
    assert(!voice->pb.state); expect(&voice->pb,0);
    AXPB pcm={.state=1}; pcm.addr.format=25; pcm.adpcm.gain=0x100;
    put_address(&pcm,base+16,base+19,base+16);
    s16 pcm8_expected[]={-32768,-256,0,32512};
    for(unsigned i=0;i<4;++i) expect(&pcm,pcm8_expected[i]);
    assert(!pcm.state);
    pcm=voice->pb; pcm.state=1; pcm.addr.loopFlag=1;
    put_address(&pcm,base/2,base/2+1,base/2);
    for(unsigned i=0;i<6;++i) expect(&pcm,pcm_expected[i%2]);
    assert(pcm.state==1);
    AXPB ad={.state=1};
    u32 first=(base+32)*2+2, last=(base+47)*2+1;
    put_address(&ad,first,last,first);
    s16 residuals[]={1,2,3,4,5,6,7,-8,-7,-6,-5,-4,-3,-2};
    for(unsigned i=0;i<14;++i) expect(&ad,residuals[i]);
    assert(ad.adpcm.pred_scale==1);
    for(unsigned i=0;i<14;++i) expect(&ad,2);
    assert(!ad.state); expect(&ad,0);
    memset(&ad,0,sizeof(ad)); ad.state=1; ad.adpcm.a[0][0]=2048;
    put_address(&ad,first,last,first);
    s16 predicted[]={1,3,6,10,15,21,28,20,13,7,2,-2,-5,-7};
    for(unsigned i=0;i<14;++i) expect(&ad,predicted[i]);
    for(unsigned i=0;i<14;++i) expect(&ad,-5+2*i);
    memset(&ad,0,sizeof(ad)); ad.state=1; ad.adpcm.a[0][0]=2048;
    ad.adpcm.a[0][1]=(u16)-1024; ad.adpcm.yn1=1000; ad.adpcm.yn2=(u16)-2000;
    put_address(&ad,first,last,first); expect(&ad,2001);
    ad.adpcm.a[0][1]=0; ad.adpcm.yn1=32767;
    put_address(&ad,first,last,first); expect(&ad,32767);
    ad.adpcm.yn1=(u16)-32768;
    put_address(&ad,first+7,last,first); expect(&ad,-32768);
    for(unsigned streaming=0;streaming<2;++streaming) {
        memset(&ad,0,sizeof(ad)); ad.state=1; ad.type=streaming; ad.addr.loopFlag=1;
        ad.adpcm.a[0][0]=ad.adpcm.a[1][0]=2048; ad.adpcm.yn1=10;
        ad.adpcmLoop.loop_yn1=10; ad.adpcmLoop.loop_pred_scale=0x10;
        put_address(&ad,first,first+1,first);
        expect(&ad,11); expect(&ad,13); assert(ad.adpcm.pred_scale==0x10);
        expect(&ad,streaming?14:11); expect(&ad,streaming?16:13);
    }
    /* Synth disables looping and installs its silence-buffer address before
     * the final streaming segment ends. That unused address may be a header. */
    AXPB one_shot=ad;one_shot.addr.loopFlag=0;
    put_address(&one_shot,first,first,0x8000);
    s16 final_sample;assert(melee_ax_decode_sample(&one_shot,&final_sample));
    assert(!one_shot.state);expect(&one_shot,0);
    one_shot=ad;one_shot.addr.loopFlag=1;
    put_address(&one_shot,first,first,0x8000);failure(&one_shot);
    AXPB bad=ad; bad.addr.format=99; failure(&bad);
    bad=ad; put_address(&bad,first,last-15,first); failure(&bad); /* end at frame header */
    bad=voice->pb; bad.state=1; put_address(&bad,UINT32_MAX,UINT32_MAX,0); failure(&bad);
    /* Source conversion retains the original four-sample delay/history. */
    AXPB start=voice->pb; start.state=1; start.src.ratioHi=1;
    start.src.ratioLo=0; start.src.currentAddressFrac=0;
    memset(start.src.last_samples,0,sizeof(start.src.last_samples));
    put_address(&start,base/2,base/2+3,base/2);
    s16 converted[160];
    AXPB converted_pb=start; converted_pb.srcSelect=2;
    assert(melee_ax_resample(&converted_pb,converted,8,NULL,0));
    for(unsigned i=0;i<8;++i) assert(converted[i]==(i<4?pcm_expected[i]:0));
    converted_pb=start; converted_pb.srcSelect=1;
    assert(melee_ax_resample(&converted_pb,converted,8,NULL,0));
    s16 linear[]={0,0,0,-32768,-1,1,32767,0};
    assert(!memcmp(converted,linear,sizeof(linear)));
    converted_pb=start; converted_pb.srcSelect=1; converted_pb.src.ratioHi=0;
    converted_pb.src.ratioLo=0x8000;
    assert(melee_ax_resample(&converted_pb,converted,16,NULL,0));
    s16 half[]={0,0,0,0,0,0,-16384,-32768,-16385,-1,0,1,16384,32767,16383,0};
    assert(!memcmp(converted,half,sizeof(half)));
    /* Synthetic coefficients isolate the filter arithmetic. */
    s16 coefficients[1536]={0};
    for(unsigned phase=0;phase<128;++phase) {
        coefficients[512+phase*4]=16384;
        coefficients[512+phase*4+1]=16384;
        for(unsigned tap=0;tap<4;++tap) coefficients[1024+phase*4+tap]=32767;
    }
    converted_pb=start; converted_pb.srcSelect=0; converted_pb.coefSelect=1;
    assert(melee_ax_resample(&converted_pb,converted,8,coefficients,1536));
    s16 filtered[]={0,0,-16384,-16385,0,16384,16383,0};
    assert(!memcmp(converted,filtered,sizeof(filtered)));
    for(unsigned negative=0;negative<2;++negative) {
        converted_pb=start; converted_pb.srcSelect=0; converted_pb.coefSelect=2;
        converted_pb.src.ratioHi=0;
        for(unsigned j=0;j<4;++j) converted_pb.src.last_samples[j]=negative?0x8000:0x7FFF;
        assert(melee_ax_resample(&converted_pb,converted,1,coefficients,1536));
        assert(converted[0]==(negative?-32768:32767));
    }
    for(unsigned phase=0;phase<128;++phase) coefficients[phase*4]=phase*128;
    for(unsigned phase=0;phase<128;++phase) {
        converted_pb=start; converted_pb.srcSelect=0; converted_pb.coefSelect=0;
        converted_pb.src.ratioHi=0; converted_pb.src.currentAddressFrac=phase*512+511;
        converted_pb.src.last_samples[0]=1000;
        assert(melee_ax_resample(&converted_pb,converted,1,coefficients,1536));
        assert(converted[0]==(s16)(1000*phase/256));
    }
    /* Splitting a mixer block must not change its samples or final state. */
    u32 ratios[]={0,0x8000,0x10000,0x18000,0x40000};
    for(unsigned mode=0;mode<3;++mode) for(unsigned r=0;r<5;++r) {
        AXPB whole=start; whole.srcSelect=mode; whole.coefSelect=1;
        whole.addr.loopFlag=1; whole.src.ratioHi=ratios[r]>>16;
        whole.src.ratioLo=ratios[r]; whole.src.currentAddressFrac=0x4321;
        AXPB split=whole; s16 other[160];
        assert(melee_ax_resample(&whole,converted,160,coefficients,1536));
        unsigned at=0;
        while(at<160) {
            unsigned count=(at%11)+1; if(count>160-at) count=160-at;
            assert(melee_ax_resample(&split,other+at,count,coefficients,1536)); at+=count;
        }
        assert(!memcmp(converted,other,sizeof(other)) && !memcmp(&whole,&split,sizeof(whole)));
    }
    for(unsigned variant=0;variant<4;++variant) {
        converted_pb=start; converted_pb.srcSelect=0; converted_pb.coefSelect=1;
        if(variant==3) { converted_pb.srcSelect=2; put_address(&converted_pb,ARGetSize()/2-1,UINT32_MAX,0); }
        AXPB before=converted_pb;
        memset(converted,0xA7,sizeof(converted));
        assert(!melee_ax_resample(&converted_pb,converted,variant==2?161:8,
            variant==0?NULL:coefficients,variant==1?1023:1536));
        assert(!memcmp(&converted_pb,&before,sizeof(before)));
        for(unsigned i=0;i<160;++i) assert((u16)converted[i]==0xA7A7);
    }
    assert(melee_ax_resample(&converted_pb,NULL,0,NULL,0));
    /* Read the unmodified big-endian dependency independently of the compiled
     * include. Check every bank/phase against signed wide-integer arithmetic. */
    FILE* coefficient_file=fopen("third_party/dolphin_dsp/dsp_coef.bin","rb");
    assert(coefficient_file);
    unsigned char coefficient_bytes[4096];
    assert(fread(coefficient_bytes,1,sizeof(coefficient_bytes),coefficient_file)==4096);
    assert(fgetc(coefficient_file)==EOF); assert(!fclose(coefficient_file));
    s16 pinned[1536];
    for(unsigned i=0;i<1536;++i) {
        unsigned word=(unsigned)coefficient_bytes[2*i]*256+coefficient_bytes[2*i+1];
        pinned[i]=(s16)(word<32768?(int)word:(int)word-65536);
    }
    for(unsigned bank=0;bank<3;++bank) for(unsigned phase=0;phase<128;++phase)
    for(unsigned pattern=0;pattern<6;++pattern) {
        converted_pb=start; converted_pb.srcSelect=0; converted_pb.coefSelect=bank;
        converted_pb.src.ratioHi=0; converted_pb.src.ratioLo=0;
        converted_pb.src.currentAddressFrac=phase*512+511;
        int64_t sum=0;
        for(unsigned tap=0;tap<4;++tap) {
            int value=pattern<4?(pattern==tap?32767:0):
                pattern==4?-32768:(tap%2?-12345:23456);
            converted_pb.src.last_samples[tap]=(u16)value;
            sum+=(int64_t)value*pinned[bank*512+phase*4+tap];
        }
        int64_t expected=sum/32768;
        if(sum<0 && sum%32768) --expected;
        if(expected>32767) expected=32767;
        if(expected< -32768) expected= -32768;
        assert(melee_ax_resample_native(&converted_pb,converted,1));
        assert(converted[0]==expected);
    }
    for(unsigned bank=0;bank<3;++bank) for(unsigned r=0;r<5;++r) {
        AXPB whole=start; whole.srcSelect=0; whole.coefSelect=bank;
        whole.addr.loopFlag=1; whole.src.ratioHi=ratios[r]>>16;
        whole.src.ratioLo=ratios[r]; whole.src.currentAddressFrac=0x4321;
        AXPB split=whole, explicit_table=whole; s16 other[160], explicit_output[160];
        assert(melee_ax_resample_native(&whole,converted,160));
        assert(melee_ax_resample(&explicit_table,explicit_output,160,pinned,1536));
        for(unsigned at=0;at<160;) {
            unsigned count=at%11+1; if(count>160-at) count=160-at;
            assert(melee_ax_resample_native(&split,other+at,count)); at+=count;
        }
        assert(!memcmp(converted,other,sizeof(other)) && !memcmp(&whole,&split,sizeof(whole)));
        assert(!memcmp(converted,explicit_output,sizeof(other)) && !memcmp(&whole,&explicit_table,sizeof(whole)));
    }
    AXFreeVoice(voice); ARReset(); failure(&ad);
    puts("AX decoding/SRC: PCM/ADPCM vectors, loop histories, direct/linear/four-tap conversion, split-block continuity and transactional failures passed");
}
