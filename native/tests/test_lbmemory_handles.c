#include "../../src/melee/lb/lbmemory.c"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#undef __assert
void OSReport(char* format,...){(void)format;abort();}
u32 ARAlloc(u32 length){assert(length==32);return 0x4000;}
u32 ARFree(u32* length){*length=32;return 0x4000;}
u32 ARGetSize(void){return 16*1024*1024;}
void __assert(char* file,u32 line,char* expression)
{fprintf(stderr,"%s:%u %s\n",file,line,expression);abort();}
static OSAlarm* pending;
static unsigned completions,ticks;
BOOL OSDisableInterrupts(void){return 1;}
BOOL OSRestoreInterrupts(BOOL enabled){assert(enabled);return 1;}
void OSCreateAlarm(OSAlarm* alarm){memset(alarm,0,sizeof(*alarm));}
void OSSetAlarm(OSAlarm* alarm,OSTime tick,OSAlarmHandler handler)
{assert(!pending&&tick>0);alarm->handler=handler;pending=alarm;}
int HSD_DevComRequest(int file,uintptr_t src,uintptr_t dest,size_t size,int type,int pri,HSD_DevComCallback cb,void* args)
{assert(!"Host RAM compaction must not request ARAM DMA");return 0;}
static void compacted(u32 value){assert(value==123);completions++;}
static void drain(void){while(pending){OSAlarm* alarm=pending;pending=NULL;alarm->handler(alarm,NULL);assert(++ticks<10);}}
int main(void)
{
    lbMemory_8001564C();
    unsigned count=0;
    for(Handle* h=_p(free_heap);h;h=h->x0_next)assert(++count<=5);
    assert(count==5&&_p(x69C)==&_p(x638_heap)[0]);
    void* memory=malloc(262144);assert(memory&&(uintptr_t)memory>UINT32_MAX);
    Handle* handles[5];
    for(unsigned i=0;i<5;i++){
        handles[i]=lbMemory_80014E24(memory,(u8*)memory+262144);
        assert(handles[i]->x4_lo==memory&&handles[i]->x8_hi==(u8*)memory+262144);
    }
    assert(!_p(free_heap));
    Handle* a=lbMemory_80014FC8(handles[0],64);
    Handle* b=lbMemory_80014FC8(handles[0],96);
    assert(a->x4_lo==memory&&b->x4_lo==(u8*)memory+64);
    memset(a->x4_lo,0x31,64);memset(b->x4_lo,0x72,96);
    assert(lbMemory_80014F7C(handles[0])==262144-160);
    lbMemFreeToHeap(handles[0],a->x4_lo);
    Handle* c=lbMemory_80014FC8(handles[0],32);
    assert(c->x4_lo==memory);
    for(unsigned i=0;i<96;i++)assert(((u8*)b->x4_lo)[i]==0x72);
    assert(lbMemory_80014F7C(handles[0])==262144-128);
    Handle* prefix=lbMemory_80014FC8(handles[1],32);
    memset(prefix->x4_lo,0x44,32);
    Handle* gap=lbMemory_80014FC8(handles[1],32);
    Handle* large=lbMemory_80014FC8(handles[1],200000);
    Handle* tail=lbMemory_80014FC8(handles[1],64);
    for(unsigned i=0;i<200000;i++)((u8*)large->x4_lo)[i]=(u8)(i*37);
    memset(tail->x4_lo,0xe9,64);
    lbMemFreeToHeap(handles[1],gap->x4_lo);
    assert(lbMemory_8001529C(handles[1],compacted,123)==1&&pending&&!completions);
    drain();assert(completions==1&&ticks==3);
    assert(large->x4_lo==(u8*)memory+32&&tail->x4_lo==(u8*)memory+200032);
    for(unsigned i=0;i<32;i++)assert(((u8*)prefix->x4_lo)[i]==0x44);
    for(unsigned i=0;i<200000;i++)assert(((u8*)large->x4_lo)[i]==(u8)(i*37));
    for(unsigned i=0;i<64;i++)assert(((u8*)tail->x4_lo)[i]==0xe9);
    assert(lbMemory_8001529C(handles[1],compacted,123)==0&&!pending&&completions==1);
    for(unsigned i=0;i<5;i++)lbMemory_80014EEC(handles[i]);
    lbMemory_800155A4();count=0;
    for(Handle* h=_p(free_heap);h;h=h->x0_next)assert(++count<=6);
    assert(count==6);
    free(memory);puts("Original LB heap handles: native initialization, five host heaps, ARAM heap, overlapping multi-chunk host compaction and release passed");
}
