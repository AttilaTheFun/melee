#include "melee_action.h"
#include <melee/lb/types.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t bytes[32+128+4];
static void word(uint8_t* p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void fixture(void)
{
    memset(bytes,0,sizeof(bytes));word(bytes,sizeof(bytes));word(bytes+4,128);word(bytes+8,1);
    uint8_t* d=bytes+32;word(d,3u<<26|2);word(d+4,31u<<26|3u<<19|1);
    word(d+8,1u<<26|2);word(d+12,4u<<26);word(d+16,5u<<26);word(d+20,32);
    word(d+32,31u<<26|3u<<19|0x7ffff);word(d+36,6u<<26);word(d+128,20);
}
static MeleeAction* decode(void){MeleeArchive a;assert(melee_archive_open(&a,bytes,sizeof(bytes)));return melee_action_decode(&a,0);}
static MeleeHostBool event(void* context,const MeleeActionEvent* e)
{
    unsigned* events=context;assert(e->opcode==31 && e->count==1 && ((e->words[0]>>19)&127)==3);
    assert((e->words[0]&0x7ffff)==(*events==2?0x7ffff:1));++*events;return true;
}
int main(void)
{
    fixture();MeleeAction* a=decode();assert(a);memset(bytes,0,sizeof(bytes));unsigned events=0;
    for(unsigned frame=0;frame<=4;frame++){
        MeleeActionResult r=melee_action_step(a,(float)frame,frame?1:0,100,event,&events);
        assert(r==(frame==4?MELEE_ACTION_DONE:MELEE_ACTION_WAIT));
        assert(events==(frame<2?1:frame<4?2:3));
    }
    melee_action_reset(a);assert(melee_action_step(a,0,0,100,NULL,NULL)==MELEE_ACTION_UNHANDLED);
    assert(melee_action_step(a,0,0,100,event,&events)==MELEE_ACTION_UNHANDLED);melee_action_free(a);
    fixture();word(bytes+32,7u<<26);word(bytes+36,0);word(bytes+160,4);
    a=decode();assert(a && melee_action_step(a,0,0,8,NULL,NULL)==MELEE_ACTION_BUDGET);melee_action_free(a);
    fixture();word(bytes+32,6u<<26);a=decode();assert(a && melee_action_step(a,0,0,8,NULL,NULL)==MELEE_ACTION_INVALID_STACK);melee_action_free(a);
    fixture();word(bytes+36,6u<<26);a=decode();assert(a && melee_action_step(a,0,0,8,NULL,NULL)==MELEE_ACTION_INVALID_STACK);melee_action_free(a);
    fixture();word(bytes+32,5u<<26);word(bytes+36,0);word(bytes+40,0);word(bytes+160,4);
    a=decode();assert(a && melee_action_step(a,0,0,8,NULL,NULL)==MELEE_ACTION_INVALID_STACK);melee_action_free(a);
    fixture();word(bytes+52,20);assert(!decode()); /* target overlaps call operand */
    fixture();word(bytes+32,63u<<26);assert(!decode());
    fixture();word(bytes+52,124);word(bytes+156,10u<<26);assert(!decode());
    /* Unrelocated null calls/jumps terminate; relocated offset zero loops. */
    fixture();word(bytes+52,0);word(bytes+160,24);a=decode();assert(a);
    events=0;
    for(unsigned frame=0;frame<=4;frame++)
        assert(melee_action_step(a,(float)frame,frame?1:0,100,event,&events)==(frame==4?MELEE_ACTION_DONE:MELEE_ACTION_WAIT));
    assert(events==2);melee_action_free(a);
    fixture();word(bytes+32,7u<<26);word(bytes+36,0);
    a=decode();assert(a);memset(bytes,0,sizeof(bytes));
    assert(melee_action_step(a,0,0,8,NULL,NULL)==MELEE_ACTION_DONE);melee_action_free(a);
    fixture();word(bytes+52,28);word(bytes+160,24);assert(!decode()); /* nonzero without relocation */
    fixture();word(bytes+32,8u<<26);word(bytes+40,0);a=decode();assert(a);events=0;
    assert(melee_action_step(a,0,0,8,event,&events)==MELEE_ACTION_WAIT && !events);
    assert(melee_action_step(a,5,1,8,event,&events)==MELEE_ACTION_WAIT && !events);
    assert(melee_action_step(a,0,1,8,event,&events)==MELEE_ACTION_DONE && events==1);melee_action_free(a);
    fixture();MeleeArchive archive;assert(melee_archive_open(&archive,bytes,sizeof(bytes)));
    uint32_t roots[]={0,32};a=melee_fighter_actions_decode(&archive,roots,2);assert(a);
    union CmdUnion* entry=melee_fighter_actions_script(a,0);assert(entry);
    assert(entry[1].set_dobj_flags.idx==3&&entry[8].set_dobj_flags.value==-1);
    assert(entry[5].Command_05.ptr==entry+8&&!melee_fighter_actions_script(a,20));
    memset(bytes,0,sizeof(bytes));assert(entry[5].Command_05.ptr->set_dobj_flags.value==-1);melee_action_free(a);
    fixture();word(bytes+52,20);assert(melee_archive_open(&archive,bytes,sizeof(bytes)));assert(!melee_fighter_actions_decode(&archive,roots,2));
    melee_action_free(NULL);assert(!melee_action_decode(NULL,0));
    puts("Owned action programs: endian decoding, event payloads, original timers/loops/calls, archive independence, stack faults and instruction budget passed");
}
