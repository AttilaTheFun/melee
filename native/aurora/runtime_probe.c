#include "melee_runtime.h"
extern int melee_runtime_test_rumble(int setting);
extern int melee_runtime_test_unlock(int setting);
#include "melee_pad_backend.h"
#include "melee_audio_producer.h"
#include <string.h>
#include <assert.h>
#include <png.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
static const char* disc;
static const char* cache;
static void* run(void* unused){(void)unused;melee_runtime_run_with_save(disc,cache,getenv("MELEE_TEST_CARD"));return NULL;}
int main(int argc,char** argv){
    if(argc!=4&&(argc!=5||strcmp(argv[4],"--audio")))return 2;
    int audio=argc==5;
    MeleeAudioRing* queue=audio?melee_audio_ring_create():NULL;
    MeleeAudioProducer* producer=NULL;
    uint64_t audio_tick=0,audio_frames=0,audible=0;unsigned input_event=0;
    const unsigned events[]={180,186,300,306,450,456};
    disc=argv[1];cache=argv[2];
    unsigned char* pixels=malloc(640*480*4);assert(pixels);
    assert(melee_runtime_state()==MELEE_RUNTIME_IDLE);
    assert(melee_runtime_copy_frame(pixels,640*480*4,0)==0);
    PADStatus ports[4]={0};for(int i=1;i<4;++i)ports[i].err=PAD_ERR_NO_CONTROLLER;
    melee_pad_publish(ports,0);
    pthread_t thread;assert(!pthread_create(&thread,NULL,run,NULL));pthread_detach(thread);
    uint64_t sequence=0;time_t started=time(NULL);int tested_pause=0;
    while(time(NULL)-started<60){
        if(producer){
            assert(melee_audio_producer_status(producer)==MELEE_AUDIO_PRODUCING);
            struct timespec now;clock_gettime(CLOCK_MONOTONIC,&now);
            uint64_t tick=(uint64_t)now.tv_sec*1000000000+now.tv_nsec;
            if(tick>=audio_tick){
                int16_t pcm[320];size_t count=melee_audio_ring_read(queue,pcm,160);
                for(size_t i=0;i<count*2;++i)audible+=pcm[i]!=0;
                audio_frames+=count;audio_tick=tick+5000000;
            }
        }
        if(melee_runtime_state()==MELEE_RUNTIME_FAILED){char error[512];melee_runtime_error(error,sizeof(error));fprintf(stderr,"Runtime failed: %s\n",error);return 3;}
        uint64_t next=melee_runtime_copy_frame(pixels,640*480*4,sequence);
        if(!next){usleep(1000);continue;}sequence=next;
        if(audio&&!producer){producer=melee_audio_producer_start(queue);assert(producer);}
        if(audio&&input_event<6&&sequence>=events[input_event]){
            ports[0].button=(input_event%2)?0:PAD_BUTTON_A;
            melee_pad_publish(ports,0);++input_event;
        }
        assert(!melee_runtime_copy_frame(pixels,1,0));
        assert(!melee_runtime_copy_frame(pixels,640*480*4,UINT64_MAX));
        if(sequence>=60&&!tested_pause){
            melee_runtime_set_paused(1);usleep(200000);
            // Joining must work even after the game has entered its pause wait.
            if(producer){assert(melee_audio_producer_destroy(producer));producer=NULL;melee_audio_ring_reset(queue);}
            uint64_t held=melee_runtime_copy_frame(pixels,640*480*4,0);assert(held);
            usleep(100000);assert(!melee_runtime_copy_frame(pixels,640*480*4,held));
            if(getenv("MELEE_TEST_SAVE_WRITE")){
                assert(melee_runtime_test_rumble(-1));
                if(getenv("MELEE_TEST_SAVE_UNLOCK")){
                    assert(melee_runtime_test_unlock(-1)==0);
                    assert(melee_runtime_test_unlock(1)==1);
                }
                melee_runtime_test_rumble(0);
                assert(!melee_runtime_test_rumble(-1));
                fprintf(stderr,"Save fixture: disabled P1 rumble before confirming creation\n");
            }
            melee_runtime_set_paused(0);tested_pause=1;
        }
        if(sequence>=(audio?600:180)){
            if(getenv("MELEE_TEST_SAVE_EXPECT")){
                melee_runtime_set_paused(1);usleep(200000);
                int expected=atoi(getenv("MELEE_TEST_SAVE_EXPECT"));
                int actual=melee_runtime_test_rumble(-1);
                fprintf(stderr,"Save fixture: loaded rumble=%d expected=%d\n",actual,expected);
                assert(actual==expected);
                if(getenv("MELEE_TEST_SAVE_UNLOCK")){
                    assert(melee_runtime_test_unlock(-1)==1);
                    fprintf(stderr,"Save fixture: Luigi unlock restored\n");
                }
            }
            png_image image={0};image.version=PNG_IMAGE_VERSION;image.width=640;image.height=480;image.format=PNG_FORMAT_RGBA;
            assert(png_image_write_to_file(&image,argv[3],0,pixels,2560,NULL));
            unsigned nonblack=0;for(unsigned i=0;i<640*480;++i){
                nonblack+=pixels[4*i]||pixels[4*i+1]||pixels[4*i+2];
                assert(pixels[4*i+3]==255);
            }
            assert(nonblack>1000&&tested_pause);
            fprintf(stderr,"PASS app runtime: sequence=%llu, nonblack=%u, pause/resume and bounded frame copies\n",(unsigned long long)sequence,nonblack);
            if(audio){
                assert(producer&&melee_audio_producer_destroy(producer));
                assert(audio_frames>32000&&audible>0);
                fprintf(stderr,"PASS game audio: %llu PCM frames, %llu nonzero samples; producer stop/resume\n",(unsigned long long)audio_frames,(unsigned long long)audible);
                melee_audio_ring_destroy(queue);
            }
            fflush(stderr);_Exit(0);
        }
    }
    fprintf(stderr,"Runtime probe timed out at sequence %llu\n",(unsigned long long)sequence);return 124;
}
