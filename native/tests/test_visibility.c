#include "melee_visibility.h"
#include "melee_action.h"
#include <sysdolphin/baselib/dobj.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t bytes[32+128+32];
static void word(uint8_t* p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void fixture(void)
{
    memset(bytes,0,sizeof(bytes));word(bytes,sizeof(bytes));word(bytes+4,128);word(bytes+8,8);
    uint8_t* d=bytes+32;word(d,2);word(d+4,8);word(d+8,40);
    word(d+40,2);word(d+44,56);word(d+48,1);word(d+52,72);
    word(d+56,2);word(d+60,80);word(d+64,1);word(d+68,82);word(d+72,1);word(d+76,83);
    d[80]=0;d[81]=1;d[82]=2;d[83]=3;
    uint32_t slots[]={4,8,44,52,60,68,76,4};for(unsigned i=0;i<8;i++)word(d+128+i*4,slots[i]);
}
static MeleeVisibility* decode(size_t costume){MeleeArchive a;assert(melee_archive_open(&a,bytes,sizeof(bytes)));return melee_visibility_decode(&a,0,costume,0,5);}
static MeleeHostBool model_event(void* v,const MeleeActionEvent* event)
{return event->count==1 && melee_visibility_command(v,event->words[0]);}
int main(void)
{
    fixture();MeleeArchive explicitArchive;assert(melee_archive_open(&explicitArchive,bytes,sizeof(bytes)));
    MeleeVisibility* explicitTable=melee_visibility_decode_lookup(&explicitArchive,2,40,5);assert(explicitTable);
    for(unsigned i=0;i<4;i++)assert(melee_visibility_controls(explicitTable,i));assert(!melee_visibility_controls(explicitTable,4));
    const int selected[]={1,0};assert(melee_visibility_select(explicitTable,selected,2));
    assert(melee_visibility_hidden(explicitTable,0)&&!melee_visibility_hidden(explicitTable,2)&&!melee_visibility_hidden(explicitTable,3));
    melee_visibility_free(explicitTable);
    assert(!melee_visibility_decode_lookup(&explicitArchive,12,40,5));
    assert(!melee_visibility_decode_lookup(&explicitArchive,2,125,5));
    assert(!melee_visibility_decode_lookup(&explicitArchive,2,40,3));
    assert(!melee_visibility_decode_lookup(&explicitArchive,2,40,125));
    assert(!melee_visibility_decode_lookup(NULL,2,40,5));
    MeleeVisibility* v=decode(1);assert(v); /* null costume entry falls back */
    assert(melee_visibility_group_count(v)==2 && melee_visibility_choice_count(v,0)==2);
    for(unsigned i=0;i<4;i++)assert(melee_visibility_hidden(v,i));assert(!melee_visibility_hidden(v,4));
    HSD_DObj actual[5]={0};HSD_DObj* pointers[5];
    for(unsigned i=0;i<5;i++){actual[i].flags=0x80;pointers[i]=actual+i;}
    actual[4].flags|=DOBJ_HIDDEN;
    assert(!melee_visibility_bind(v,pointers,4));pointers[1]=pointers[0];
    assert(!melee_visibility_bind(v,pointers,5));assert(actual[0].flags==0x80);
    pointers[1]=actual+1;assert(melee_visibility_bind(v,pointers,5));
    for(unsigned i=0;i<5;i++)assert(actual[i].flags==0x81);
    memset(bytes,0,sizeof(bytes));int choices[]={0,-1};assert(melee_visibility_select(v,choices,2));
    assert(!melee_visibility_hidden(v,0) && !melee_visibility_hidden(v,1) && melee_visibility_hidden(v,2) && melee_visibility_hidden(v,3));
    assert(actual[0].flags==0x80 && actual[2].flags==0x81 && actual[4].flags==0x81);
    choices[0]=1;choices[1]=0;assert(melee_visibility_select(v,choices,2));
    assert(melee_visibility_hidden(v,0) && melee_visibility_hidden(v,1) && !melee_visibility_hidden(v,2) && !melee_visibility_hidden(v,3));
    pointers[0]=actual+2;pointers[2]=actual;
    assert(melee_visibility_bind(v,pointers,5));assert(actual[0].flags==0x80&&actual[2].flags==0x81);
    pointers[0]=actual;pointers[2]=actual+2;assert(melee_visibility_bind(v,pointers,5));
    choices[0]=2;assert(melee_visibility_select(v,choices,2));assert(melee_visibility_hidden(v,2));
    choices[0]=1;assert(melee_visibility_select(v,choices,2));
    choices[0]=128;assert(!melee_visibility_select(v,choices,2));assert(!melee_visibility_hidden(v,2));
    choices[0]=-129;assert(!melee_visibility_select(v,choices,2));
    assert(!melee_visibility_select(v,NULL,2));
    const int defaults[]={0,-1};assert(melee_visibility_defaults(v,defaults,2));
    const int bad_defaults[]={-1,128};assert(!melee_visibility_defaults(v,bad_defaults,2));
    assert(melee_visibility_command(v,32u<<26));
    assert(!melee_visibility_hidden(v,0) && !melee_visibility_hidden(v,1) && melee_visibility_hidden(v,2) && melee_visibility_hidden(v,3));
    assert(melee_visibility_command(v,31u<<26|1));assert(melee_visibility_hidden(v,0) && !melee_visibility_hidden(v,2));
    assert(melee_visibility_command(v,31u<<26|0x7ffff));assert(melee_visibility_hidden(v,2));
    assert(melee_visibility_command(v,31u<<26|1u<<19));assert(!melee_visibility_hidden(v,3));
    assert(!melee_visibility_command(v,31u<<26|127u<<19));assert(!melee_visibility_hidden(v,3));
    assert(!melee_visibility_command(v,31u<<26|2u<<19));
    assert(!melee_visibility_command(v,30u<<26));
    assert(melee_visibility_command(v,33u<<26));for(unsigned i=0;i<4;i++)assert(melee_visibility_hidden(v,i));
    assert(melee_visibility_command(v,32u<<26));assert(!melee_visibility_hidden(v,0) && melee_visibility_hidden(v,3));
    melee_visibility_free(v);
    fixture();bytes[32+80]=5;assert(!decode(0));
    fixture();word(bytes+32,12);assert(!decode(0));
    fixture();word(bytes+32+40,129);assert(!decode(0));
    fixture();word(bytes+32+60,127);assert(!decode(0));
    fixture();assert(!decode(SIZE_MAX));
    fixture();v=decode(0);assert(v && melee_visibility_controls(v,0) && !melee_visibility_controls(v,4));
    assert(melee_visibility_defaults(v,defaults,2));
    uint8_t* d=bytes+32;word(d+84,31u<<26|1);word(d+88,1u<<26|2);word(d+92,33u<<26);word(d+96,1u<<26|2);word(d+100,32u<<26);
    MeleeArchive archive;assert(melee_archive_open(&archive,bytes,sizeof(bytes)));
    MeleeAction* action=melee_action_decode(&archive,84);assert(action);memset(bytes,0,sizeof(bytes));
    assert(melee_action_step(action,0,0,100,model_event,v)==MELEE_ACTION_WAIT && !melee_visibility_hidden(v,2));
    assert(melee_action_step(action,2,2,100,model_event,v)==MELEE_ACTION_WAIT && melee_visibility_hidden(v,2));
    assert(melee_action_step(action,4,2,100,model_event,v)==MELEE_ACTION_DONE && !melee_visibility_hidden(v,0) && melee_visibility_hidden(v,3));
    assert(!melee_visibility_init_fighter(v,0)); /* incompatible table shape */
    assert(!melee_visibility_init_fighter(v,UINT32_MAX));
    melee_action_free(action);melee_visibility_free(v);
    assert(!melee_visibility_decode(NULL,0,0,0,0));melee_visibility_free(NULL);
    puts("Original fighter visibility: owned tables, costume fallback, hide/select transitions and malformed indices passed");
}
