#include "melee_ax_pool_mix.h"
#include "melee_ax_voice.h"
#include "melee_ax_output.h"
#include "melee_ax_stream.h"
#include "melee_audio_producer.h"
#include <stdatomic.h>
#include "melee_ax_aux.h"
#include "../../extern/dolphin/src/dolphin/ax/__ax.h"
#include <dolphin/ar.h>
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition=PTHREAD_COND_INITIALIZER;
static int complete;
static unsigned stolen;
static unsigned effect_calls;
static unsigned frame_calls, fail_after;
static AXVPB* callback_voice;
static MeleeAXStream* callback_stream;
static _Atomic unsigned worker_calls;
static bool worker_fail;
static pthread_t control_thread;
static void unexpected_frame(void) { assert(0); }
static void unexpected_aux(void* buffers, void* context) { assert(0); }
static void worker_callback(void)
{
    assert(!pthread_equal(pthread_self(),control_thread));
    unsigned calls=atomic_fetch_add(&worker_calls,1)+1;
    if(worker_fail && calls==2) callback_voice->pb.srcSelect=99;
}
static void pause_tick(void)
{ const struct timespec interval={.tv_nsec=1000000}; nanosleep(&interval,NULL); }
static void frame_callback(void)
{
    ++frame_calls;
    AXPBVE ve={.currentVolume=frame_calls%2?0x2000:0x4000};
    AXSetVoiceVe(callback_voice,&ve);
    if(fail_after && frame_calls==fail_after) callback_voice->pb.srcSelect=99;
    s16 sentinel[2]={123,456};
    assert(!melee_ax_stream_read(callback_stream,sentinel,1));
    assert(sentinel[0]==123 && sentinel[1]==456);
}
static void effect(void* data,void* context)
{
    struct AX_AUX_DATA* channels=data;
    for(unsigned i=0;i<160;++i) {
        channels->l[i]*=2; channels->r[i]*=2; channels->s[i]*=2;
    }
    ++effect_calls;
    MeleeAXOutput* state=context;
    MeleeAXOutput saved=*state;
    s16 output[320]; memset(output,0xA7,sizeof(output));
    assert(!melee_ax_output_frame(state,output));
    assert(!memcmp(state,&saved,sizeof(saved)));
    for(unsigned i=0;i<320;++i) assert((u16)output[i]==0xA7A7);
}
static _Alignas(32) unsigned char source[320];
static void dma_done(ARQRequest* unused)
{
    (void)unused; pthread_mutex_lock(&mutex); complete=1;
    pthread_cond_signal(&condition); pthread_mutex_unlock(&mutex);
}
static void on_stolen(void* object)
{
    AXVPB* voice=object; assert(voice->priority==1 && voice->userContext==123);
    ++stolen;
}
static void configure(AXVPB* voice,u32 base,u16 envelope)
{
    AXPBADDR address={.format=10,.loopFlag=1};
    address.loopAddressHi=address.currentAddressHi=(base/2)>>16;
    address.loopAddressLo=address.currentAddressLo=base/2;
    address.endAddressHi=(base/2+159)>>16; address.endAddressLo=base/2+159;
    AXSetVoiceAddr(voice,&address); AXSetVoiceSrcType(voice,AX_SRC_TYPE_NONE);
    AXPBMIX mix={.vL=0x8000}; AXSetVoiceMix(voice,&mix);
    AXPBVE ve={.currentVolume=envelope}; AXSetVoiceVe(voice,&ve);
    AXSetVoiceState(voice,1);
}
static void check(const MeleeAXMixBuffers* buffers,s32 steady,s32 fade,s32 step)
{
    for(unsigned i=0;i<160;++i) {
        assert(buffers->main[0][i]==steady+fade-(s32)i*step);
        assert(!buffers->main[1][i] && !buffers->main[2][i]);
        for(unsigned c=0;c<3;++c) assert(!buffers->auxA[c][i] && !buffers->auxB[c][i]);
    }
}
int main(void)
{
    MeleeAXMixBuffers buffers,saved;
    memset(&buffers,0xA7,sizeof(buffers)); saved=buffers;
    assert(!melee_ax_voice_pool_render(&buffers));
    assert(!memcmp(&buffers,&saved,sizeof(buffers)));
    assert(!melee_ax_voice_pool_render(NULL));
    u32 stack[1]; ARInit(stack,1); u32 base=ARAlloc(sizeof(source));
    for(unsigned i=0;i<160;++i) { source[i*2]=0x0F; source[i*2+1]=0xA0; }
    ARRegisterDMACallback(dma_done);
    ARStartDMA(ARAM_DIR_MRAM_TO_ARAM,(ARAddress)source,base,sizeof(source));
    struct timespec deadline; clock_gettime(CLOCK_REALTIME,&deadline); deadline.tv_sec+=10;
    pthread_mutex_lock(&mutex);
    while(!complete) assert(!pthread_cond_timedwait(&condition,&mutex,&deadline));
    pthread_mutex_unlock(&mutex);
    melee_ax_voice_pool_init();
    AXVPB* voices[64];
    for(unsigned i=0;i<64;++i) {
        voices[i]=AXAcquireVoice(1,on_stolen,123); assert(voices[i]);
        configure(voices[i],base,0x4000);
    }
    assert(melee_ax_voice_pool_render(&buffers)); check(&buffers,128000,0,0);
    for(unsigned i=0;i<64;++i) assert(!voices[i]->sync && voices[i]->pb.dpop.aL==2000);
    AXVPB* replacement=AXAcquireVoice(2,NULL,0);
    assert(replacement==voices[0] && stolen==1 && replacement->depop);
    configure(replacement,base,0x1000);
    AXPBDPOP misleading={.aL=12345}; AXSetVoiceDpop(replacement,&misleading);
    assert(melee_ax_voice_pool_render(&buffers)); check(&buffers,126500,2000,12);
    assert(!replacement->depop && replacement->pb.dpop.aL==500);
    assert(melee_ax_voice_pool_render(&buffers)); check(&buffers,126500,0,0);
    AXSetVoiceState(replacement,0);
    misleading.aL=30000; AXSetVoiceDpop(replacement,&misleading);
    assert(melee_ax_voice_pool_render(&buffers)); check(&buffers,126000,500,3);
    assert(melee_ax_voice_pool_render(&buffers)); check(&buffers,126000,0,0);
    AXFreeVoice(voices[1]); assert(voices[1]->depop && !voices[1]->priority);
    assert(melee_ax_voice_pool_render(&buffers)); check(&buffers,124000,2000,12);
    assert(!voices[1]->depop);
    assert(melee_ax_voice_pool_render(&buffers)); check(&buffers,124000,0,0);
    /* A failed render must retain retirement flags and all voice parameters,
     * even after earlier voices have already rendered into temporary buses. */
    AXSetVoiceState(voices[3],0); voices[2]->pb.srcSelect=99;
    AXVPB before[64];
    for(unsigned i=0;i<64;++i) before[i]=*voices[i];
    memset(&buffers,0xA7,sizeof(buffers)); saved=buffers;
    assert(!melee_ax_voice_pool_render(&buffers));
    assert(!memcmp(&buffers,&saved,sizeof(buffers)));
    for(unsigned i=0;i<64;++i) assert(!memcmp(&before[i],voices[i],sizeof(AXVPB)));
    AXSetVoiceSrcType(voices[2],AX_SRC_TYPE_NONE);
    assert(melee_ax_voice_pool_render(&buffers)); check(&buffers,122000,2000,12);
    assert(melee_ax_voice_pool_render(&buffers)); check(&buffers,122000,0,0);
    /* Reset invalidates render histories and any outstanding fade. */
    AXSetVoiceState(voices[4],0);
    assert(melee_ax_voice_pool_render(&buffers));
    melee_ax_voice_pool_init();
    assert(melee_ax_voice_pool_render(&buffers)); check(&buffers,0,0,0);
    /* The native renderer consumes a scheduled update once, then resets the
     * SDK writer so the next callback can fill the next frame. */
    AXVPB* scheduled=AXAcquireVoice(1,NULL,0); configure(scheduled,base,0x4000);
    AXSetVoiceUpdateWrite(scheduled,50,0x2000);
    scheduled->pb.update.updNum[0]=1;
    assert(melee_ax_voice_pool_render(&buffers)); check(&buffers,1000,0,0);
    assert(!scheduled->updateMS && !scheduled->updateCounter &&
           scheduled->updateWrite==scheduled->updateData && !scheduled->sync);
    for(unsigned i=0;i<5;++i) assert(!scheduled->pb.update.updNum[i]);
    AXPBVE ve={.currentVolume=0x4000}; AXSetVoiceVe(scheduled,&ve);
    assert(melee_ax_voice_pool_render(&buffers)); check(&buffers,2000,0,0);
    AXFreeVoice(scheduled);
    /* Complete PCM frames exercise surround carry, aux latency and channel
     * ordering together with the real allocator, decoder and mixer. */
    MeleeAXOutput output_state={0}; s16 stereo[320];
    memset(stereo,0xA7,sizeof(stereo));
    assert(!melee_ax_output_frame(&output_state,stereo)); /* Aux not initialized. */
    for(unsigned mode=0;mode<4;++mode) {
        melee_ax_voice_pool_init(); assert(melee_ax_aux_init()); __AXClMode=mode;
        effect_calls=0; AXRegisterAuxACallback(effect,&output_state);
        AXVPB* v=AXAcquireVoice(1,NULL,0); configure(v,base,0x4000);
        AXPBMIX mix={.vL=0x8000,.vR=0x4000,.vS=0x2000,.vAuxAL=0x1000};
        AXSetVoiceMix(v,&mix);
        for(unsigned frame=0;frame<4;++frame) {
            assert(melee_ax_output_frame(&output_state,stereo));
            int carry=frame && mode<2?500:0;
            int left=2000+(mode==1?-carry:carry)+(frame>=2?500:0);
            int right=1000+carry;
            for(unsigned i=0;i<160;++i) {
                assert(stereo[2*i]==left && stereo[2*i+1]==right);
                assert(output_state.surround[i]==500);
            }
            assert(effect_calls==frame+1);
        }
        MeleeAXOutput history=output_state;
        v->pb.srcSelect=99; AXVPB prior=*v;
        memset(stereo,0xA7,sizeof(stereo));
        assert(!melee_ax_output_frame(&output_state,stereo));
        assert(effect_calls==4 && !memcmp(v,&prior,sizeof(prior)));
        assert(!memcmp(&history,&output_state,sizeof(history)));
        for(unsigned i=0;i<320;++i) assert((u16)stereo[i]==0xA7A7);
        AXSetVoiceSrcType(v,AX_SRC_TYPE_NONE);
        __AXClMode=4;
        assert(!melee_ax_output_frame(&output_state,stereo)); assert(effect_calls==4);
        __AXClMode=mode;
        assert(melee_ax_output_frame(&output_state,stereo)); assert(effect_calls==5);
    }
    /* Host PCM clamps after accumulation, with L/R ordering intact. */
    melee_ax_voice_pool_init(); assert(melee_ax_aux_init()); __AXClMode=2;
    for(unsigned i=0;i<64;++i) {
        AXVPB* v=AXAcquireVoice(1,NULL,0); configure(v,base,i<32?0x7FFF:0x8000);
        AXPBMIX mix={0}; if(i<32) mix.vL=0xFFFF; else mix.vR=0xFFFF;
        AXSetVoiceMix(v,&mix);
    }
    assert(melee_ax_output_frame(&output_state,stereo));
    for(unsigned i=0;i<160;++i) assert(stereo[2*i]==32767 && stereo[2*i+1]==-32768);
    melee_ax_voice_pool_init(); assert(melee_ax_aux_init()); __AXClMode=0;
    assert(melee_ax_output_frame(&output_state,stereo));
    for(unsigned i=0;i<320;++i) assert(!stereo[i]);
    assert(!melee_ax_output_frame(NULL,stereo));
    assert(!melee_ax_output_frame(&output_state,NULL));
    /* Device-sized pulls must not change callback cadence or PCM. */
    s16 whole[2000],split[2000];
    MeleeAXStream stream;
    for(unsigned run=0;run<2;++run) {
        melee_ax_voice_pool_init(); assert(melee_ax_aux_init()); __AXClMode=2;
        callback_voice=AXAcquireVoice(1,NULL,0); configure(callback_voice,base,0x4000);
        assert(melee_ax_stream_init(&stream)); callback_stream=&stream; frame_calls=0;
        AXRegisterCallback(frame_callback);
        if(!run) assert(melee_ax_stream_read(&stream,whole,1000));
        else for(unsigned at=0;at<1000;) {
            unsigned count=at%211+1; if(count>1000-at) count=1000-at;
            assert(melee_ax_stream_read(&stream,split+2*at,count)); at+=count;
        }
        assert(frame_calls==7 && stream.position==40);
        assert(melee_ax_stream_read(&stream,NULL,0)); assert(frame_calls==7);
    }
    assert(!memcmp(whole,split,sizeof(whole)));
    for(unsigned i=0;i<1000;++i) {
        assert(whole[2*i]==((i/160)%2?1000:2000)); assert(!whole[2*i+1]);
    }
    /* Keep valid prefix, zero a failed suffix, then latch without callbacks. */
    melee_ax_voice_pool_init(); assert(melee_ax_aux_init());
    callback_voice=AXAcquireVoice(1,NULL,0); configure(callback_voice,base,0x4000);
    assert(melee_ax_stream_init(&stream)); frame_calls=0; fail_after=1;
    assert(!melee_ax_stream_read(&stream,whole,200)); assert(frame_calls==1 && stream.failed);
    for(unsigned i=0;i<200;++i) assert(whole[2*i]==(i<160?2000:0) && !whole[2*i+1]);
    assert(!melee_ax_stream_read(&stream,whole,200)); assert(frame_calls==1);
    for(unsigned i=0;i<400;++i) assert(!whole[i]);
    AXRegisterCallback(NULL); fail_after=0;
    melee_ax_voice_pool_init(); assert(melee_ax_aux_init());
    assert(melee_ax_stream_read(&stream,whole,1)); assert(!stream.failed && !whole[0] && !whole[1]);
    MeleeAXStream stream_before=stream;
    assert(!melee_ax_stream_read(&stream,whole,4097));
    assert(!melee_ax_stream_read(&stream,NULL,1));
    assert(!memcmp(&stream,&stream_before,sizeof(stream)));
    /* An actual producer thread renders and paces against consumer progress. */
    control_thread=pthread_self();
    for(unsigned failing=0;failing<2;++failing) {
        melee_ax_voice_pool_init(); assert(melee_ax_aux_init()); __AXClMode=2;
        callback_voice=AXAcquireVoice(1,NULL,0); configure(callback_voice,base,0x4000);
        atomic_store(&worker_calls,0); worker_fail=failing;
        AXRegisterCallback(worker_callback);
        MeleeAudioRing* ring=melee_audio_ring_create(); assert(ring);
        MeleeAudioProducer* producer=melee_audio_producer_start(ring); assert(producer);
        assert(!melee_audio_producer_start(ring));
        unsigned ticks=0;
        while((failing?melee_audio_producer_status(producer)!=MELEE_AUDIO_FAILED:
                         atomic_load(&worker_calls)<MELEE_AUDIO_PREBUFFER_FRAMES/160) && ticks++<5000) pause_tick();
        assert(ticks<5000);
        if(failing) {
            assert(atomic_load(&worker_calls)==2);
            assert(melee_audio_ring_read(ring,whole,640)==320);
            for(unsigned i=0;i<640;++i) assert(whole[i*2]==(i<320?2000:0) && !whole[i*2+1]);
        } else {
            for(unsigned i=0;i<20;++i) pause_tick();
            assert(atomic_load(&worker_calls)==MELEE_AUDIO_PREBUFFER_FRAMES/160); /* No device demand, no extra callbacks. */
            /* Reproduce the physical iPhone's 48 kHz / 1024-frame route:
             * its source demand exceeds the old 640-frame producer ceiling.
             * Then model a larger route quantum and verify adaptive refill. */
            s16 device_pcm[1366 * 2];
            const unsigned quantum[] = {683, 1366};
            for (unsigned q=0; q<2; ++q) {
                assert(melee_audio_ring_read(ring,device_pcm,quantum[q]) == quantum[q]);
                for (unsigned i=0; i<quantum[q]; ++i)
                    assert(device_pcm[2*i]==2000 && !device_pcm[2*i+1]);
                unsigned target = quantum[q] + 640;
                if (target < MELEE_AUDIO_PREBUFFER_FRAMES) target = MELEE_AUDIO_PREBUFFER_FRAMES;
                target = (target + 159) / 160 * 160;
                ticks=0;
                while (melee_audio_ring_queued(ring) <= target-160 && ticks++<5000) pause_tick();
                assert(ticks<5000);
            }
            assert(melee_audio_ring_stats(ring).missing_frames == 0);
            unsigned received=0; ticks=0;
            while(received<3200 && ticks++<5000) {
                unsigned requested=3200-received; if(requested>191) requested=191;
                size_t count=melee_audio_ring_read(ring,whole,requested);
                for(unsigned i=0;i<count;++i) assert(whole[2*i]==2000 && !whole[2*i+1]);
                received+=(unsigned)count; if(!count) pause_tick();
            }
            assert(received==3200);
            assert(melee_audio_producer_status(producer)==MELEE_AUDIO_PRODUCING);
            melee_audio_producer_request_stop(producer);
        }
        assert(melee_audio_producer_destroy(producer));
        unsigned stopped_calls=atomic_load(&worker_calls);
        for(unsigned i=0;i<5;++i) pause_tick();
        assert(atomic_load(&worker_calls)==stopped_calls);
        melee_audio_ring_destroy(ring);
    }
    /* ITD follows envelope processing and delays every L/R bus, while all
     * surround buses use the current quantum without a delay. */
    AXInit();
    AXVPB* itd_voice=AXAcquireVoice(1,NULL,0); assert(itd_voice);
    AXPBITDBUFFER* itd_history=melee_ax_voice_itd_at(itd_voice->index);
    configure(itd_voice,base,0x4000);
    AXPBMIX itd_mix={.vL=0x8000,.vR=0x8000,.vS=0x8000,
        .vAuxAL=0x8000,.vAuxAR=0x8000,.vAuxAS=0x8000,
        .vAuxBL=0x8000,.vAuxBR=0x8000,.vAuxBS=0x8000};
    AXSetVoiceMix(itd_voice,&itd_mix); AXSetVoiceItdOn(itd_voice);
    itd_voice->pb.itd.shiftR=itd_voice->pb.itd.targetShiftR=31;
    assert(melee_ax_voice_pool_render(&buffers));
    for(unsigned i=0;i<160;++i) {
        for(unsigned bus=0;bus<3;++bus) {
            s32 (*channels)[160]=bus==0?buffers.main:bus==1?buffers.auxA:buffers.auxB;
            assert(channels[0][i]==(i<32?0:2000));
            assert(channels[1][i]==(i<1?0:2000));
            assert(channels[2][i]==2000);
        }
    }
    for(unsigned i=0;i<32;++i) assert(itd_history->data[i]==2000);
    AXPBVE zero_envelope={0}; AXSetVoiceVe(itd_voice,&zero_envelope);
    assert(melee_ax_voice_pool_render(&buffers));
    assert(buffers.main[0][0]==2000 && !buffers.main[0][32]);
    AXSetVoiceItdOn(itd_voice);
    for(unsigned i=0;i<32;++i) itd_history->data[i]=1234;
    assert(melee_ax_voice_pool_render(&buffers));
    for(unsigned i=0;i<160;++i) assert(!buffers.main[0][i]);
    AXSetVoiceItdOn(itd_voice); AXSetVoiceItdTarget(itd_voice,0,31);
    for(unsigned i=0;i<32;++i) itd_history->data[i]=1234;
    assert(melee_ax_voice_pool_render(&buffers));
    assert(buffers.main[0][0]==1234 && itd_voice->pb.itd.shiftR==5);

    /* A later voice failure rolls back earlier history, even a pending SDK
     * buffer reset. The retry applies that reset exactly once. */
    AXInit();
    AXVPB* bad_itd=AXAcquireVoice(1,NULL,0);
    itd_voice=AXAcquireVoice(1,NULL,0);
    itd_history=melee_ax_voice_itd_at(itd_voice->index);
    assert(itd_voice->index<bad_itd->index);
    configure(bad_itd,base,0x4000); bad_itd->pb.srcSelect=99;
    configure(itd_voice,base,0x4000); AXSetVoiceItdOn(itd_voice);
    memset(itd_history,0xA7,sizeof(*itd_history));
    AXPBITDBUFFER saved_itd=*itd_history;
    AXPB saved_itd_pb=itd_voice->pb;
    memset(&buffers,0xA7,sizeof(buffers)); saved=buffers;
    assert(!melee_ax_voice_pool_render(&buffers));
    assert(!memcmp(&saved_itd,itd_history,sizeof(saved_itd)));
    assert(!memcmp(&saved_itd_pb,&itd_voice->pb,sizeof(saved_itd_pb)));
    assert(!memcmp(&saved,&buffers,sizeof(saved)));
    AXFreeVoice(bad_itd);
    assert(melee_ax_voice_pool_render(&buffers));
    assert(!buffers.main[0][0] && buffers.main[0][32]==2000);
    for(unsigned i=0;i<32;++i) assert(itd_history->data[i]==2000);
    MeleeAXOutput reset_output={0}; s16 reset_pcm[320];
    for(unsigned pass=0;pass<2;++pass) {
        AXVPB* dirty=AXAcquireVoice(1,NULL,0); assert(dirty);
        dirty->pb.state=1; dirty->pb.srcSelect=99;
        AXRegisterCallback(unexpected_frame);
        AXRegisterAuxACallback(unexpected_aux,NULL);
        __AXClMode=4;
        AXInit();
        assert(__AXClMode==0 && melee_ax_aux_ready());
        assert(melee_ax_output_frame(&reset_output,reset_pcm));
        for(unsigned i=0;i<320;++i) assert(!reset_pcm[i]);
        AXVPB* reset_voices[64];
        for(unsigned i=0;i<64;++i) {
            reset_voices[i]=AXAcquireVoice(1,NULL,0); assert(reset_voices[i]);
            for(unsigned j=0;j<i;++j) assert(reset_voices[i]!=reset_voices[j]);
        }
        for(unsigned i=0;i<64;++i) AXFreeVoice(reset_voices[i]);
    }
    AXRegisterCallback(NULL); ARReset();
    puts("AX output: 64 voices, retirement fades, surround modes, aux returns, stereo clipping/order, failure recovery and reset passed");
}
