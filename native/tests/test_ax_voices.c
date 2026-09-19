#include "melee_ax_voice.h"
#include "../../extern/dolphin/src/dolphin/ax/__ax.h"
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

static unsigned callback_count;
static u32 dropped_context[8];
static void dropped(void* pointer)
{
    AXVPB* voice=pointer;
    assert(melee_ax_voice_owned(voice) && (uintptr_t)voice>UINT32_MAX);
    assert(voice->priority>0 && callback_count<8);
    dropped_context[callback_count++]=voice->userContext;
}
static atomic_uint_fast64_t owned_indices;
static void* churn(void* unused)
{
    (void)unused;
    for(unsigned i=0;i<1000;++i) {
        AXVPB* voice=AXAcquireVoice(1,NULL,i);
        assert(voice);
        uint_fast64_t mask=UINT64_C(1)<<voice->index;
        assert(!(atomic_fetch_or(&owned_indices,mask)&mask));
        voice->pb.state=1;
        assert(atomic_fetch_and(&owned_indices,~mask)&mask);
        AXFreeVoice(voice);
    }
    return NULL;
}
static void expected_abort(int signal_number) { (void)signal_number; _exit(86); }
int main(void)
{
    melee_ax_voice_pool_init();
    AXVPB* voices[64];
    uint64_t indices=0;
    for(unsigned i=0;i<64;++i) {
        AXVPB* p=voices[i]=AXAcquireVoice(5,dropped,i);
        assert(p && melee_ax_voice_owned(p) && p->index<64);
        assert(!(indices&(UINT64_C(1)<<p->index)));
        indices|=UINT64_C(1)<<p->index;
        assert(p->priority==5 && p->pb.state==0 && p->sync==0xA4);
        assert(p->updateWrite==p->updateData && p->itdBuffer);
        assert(!((uintptr_t)p->itdBuffer&31));
        assert(p->pb.currHi==0 && p->pb.currLo==0); /* No truncated DSP address. */
    }
    assert(indices==UINT64_MAX);
    assert(!AXAcquireVoice(5,NULL,0));
    assert(!AXAcquireVoice(0,NULL,0) && !AXAcquireVoice(32,NULL,0));
    voices[0]->pb.state=1;
    AXVPB* stolen=AXAcquireVoice(6,dropped,100);
    assert(stolen==voices[0] && callback_count==1 && dropped_context[0]==0);
    assert(stolen->priority==6 && stolen->depop==1 && stolen->pb.state==0);
    AXSetVoicePriority(voices[1],1);
    stolen=AXAcquireVoice(2,dropped,101);
    assert(stolen==voices[1] && callback_count==2 && dropped_context[1]==1);
    AXSetVoicePriority(voices[2],7);
    AXFreeVoice(voices[2]);
    assert(AXAcquireVoice(1,dropped,102)==voices[2] && callback_count==2);
    __AXPushCallbackStack(voices[3]);
    __AXPushCallbackStack(voices[4]);
    __AXServiceCallbackStack();
    assert(callback_count==4 && dropped_context[2]==4 && dropped_context[3]==3);
    assert(voices[3]->priority==0 && voices[4]->priority==0);
    assert(!__AXPopCallbackStack());
    for(unsigned i=0;i<64;++i) if(voices[i]->priority) AXFreeVoice(voices[i]);
    pid_t child=fork(); assert(child>=0);
    if(!child) { signal(SIGABRT,expected_abort); AXFreeVoice(voices[0]); _exit(99); }
    int status;
    assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==86);
    assert(!melee_ax_voice_owned(NULL));
    assert(!melee_ax_voice_owned((AXVPB*)((char*)voices[0]+1)));
    pthread_t workers[8];
    for(unsigned i=0;i<8;++i) assert(!pthread_create(&workers[i],NULL,churn,NULL));
    for(unsigned i=0;i<8;++i) assert(!pthread_join(workers[i],NULL));
    assert(!atomic_load(&owned_indices));
    for(unsigned i=0;i<64;++i) { voices[i]=AXAcquireVoice(1,NULL,0); assert(voices[i]); }
    assert(!AXAcquireVoice(1,NULL,0));
    for(unsigned i=0;i<64;++i) AXFreeVoice(voices[i]);
    puts("Native AX voices: all 64 slots, original priority stealing, callback queues, reuse, double-free rejection and 8000 concurrent acquire/free cycles passed");
}
