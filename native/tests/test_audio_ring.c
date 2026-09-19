/* Include the production implementation to seed counters near rollover
 * without transferring four billion frames or exposing a test-only API. */
#include "../src/audio_ring.c"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>

static MeleeAudioRing* queue;
enum { TOTAL = 500000 };
static void sample(int16_t* out,unsigned index)
{ out[0]=(int16_t)((int)(index%30001)-15000); out[1]=(int16_t)(15000-(int)(index%30001)); }
static void* produce(void* unused)
{
    (void)unused; unsigned at=0;
    while(at<TOTAL) {
        int16_t values[514]; unsigned count=at%257+1;
        if(count>TOTAL-at) count=TOTAL-at;
        for(unsigned i=0;i<count;++i) sample(values+2*i,at+i);
        size_t written=melee_audio_ring_write(queue,values,count);
        at+=(unsigned)written; if(!written) sched_yield();
    }
    return NULL;
}
int main(void)
{
    queue=melee_audio_ring_create(); assert(queue);
    int16_t values[16384],received[16384];
    for(unsigned i=0;i<8192;++i) sample(values+2*i,i);
    memset(received,0xA7,sizeof(received));
    assert(!melee_audio_ring_read(queue,received,8192));
    for(unsigned i=0;i<16384;++i) assert(!received[i]);
    assert(melee_audio_ring_write(queue,values,8192)==8192);
    assert(!melee_audio_ring_write(queue,values,1));
    assert(melee_audio_ring_read(queue,received,8179)==8179);
    assert(!memcmp(values,received,8179*4));
    assert(melee_audio_ring_write(queue,values,100)==100);
    assert(melee_audio_ring_read(queue,received,120)==113);
    assert(!memcmp(received,values+8179*2,13*4));
    assert(!memcmp(received+26,values,100*4));
    for(unsigned i=226;i<240;++i) assert(!received[i]);
    memset(received,0xA7,sizeof(received));
    assert(!melee_audio_ring_read(queue,received,8193));
    assert((uint16_t)received[0]==0xA7A7);
    assert(!melee_audio_ring_read(queue,NULL,1));
    assert(!melee_audio_ring_write(queue,NULL,1));
    assert(!melee_audio_ring_read(queue,NULL,0));
    assert(!melee_audio_ring_write(queue,NULL,0));
    MeleeAudioStats stats = melee_audio_ring_stats(queue);
    assert(stats.requested_frames == 8192 + 8179 + 120);
    assert(stats.missing_frames == 8192 + 7 && stats.underruns == 2);
    assert(stats.largest_request == 8192);
    atomic_store(&queue->produced,UINT32_MAX-49);
    atomic_store(&queue->consumed,UINT32_MAX-49);
    assert(melee_audio_ring_write(queue,values,100)==100);
    assert(atomic_load(&queue->produced)==50);
    assert(melee_audio_ring_read(queue,received,100)==100);
    assert(atomic_load(&queue->consumed)==50);
    assert(!memcmp(values,received,400));
    pthread_t worker; assert(!pthread_create(&worker,NULL,produce,NULL));
    unsigned at=0;
    while(at<TOTAL) {
        unsigned count=at%191+1;
        size_t available=melee_audio_ring_read(queue,received,count);
        for(unsigned i=0;i<available;++i) {
            int16_t expected[2]; sample(expected,at+i);
            assert(received[2*i]==expected[0] && received[2*i+1]==expected[1]);
        }
        for(size_t i=available*2;i<count*2;++i) assert(!received[i]);
        at+=(unsigned)available; if(!available) sched_yield();
    }
    assert(!pthread_join(worker,NULL));
    assert(melee_audio_ring_write(queue,values,100)==100);
    melee_audio_ring_reset(queue);
    assert(!melee_audio_ring_read(queue,received,100));
    for(unsigned i=0;i<200;++i) assert(!received[i]);
    melee_audio_ring_destroy(queue);
    puts("Audio ring: full/empty, wraparound, underrun silence and concurrent ordered stereo transport passed");
}
