#include <sysdolphin/baselib/hsd_3A94.h>
#include <sysdolphin/baselib/hsd_3B27.h>
#include <assert.h>
#include <dolphin/card.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern volatile s32 hsd_804D7980, hsd_804D7984;
static int callback_calls;
static CardState* expected_state;
static int opens, closes;
s32 CARDOpen(s32 channel,char* filename,CARDFileInfo* info)
{
    assert(channel==2&&filename&&info==&expected_state->file_info);
    if(++opens<3)return -1;
    info->fileNo=19;return 0;
}
s32 CARDClose(CARDFileInfo* info)
{
    assert(info==&expected_state->file_info);
    return ++closes<2?-1:0;
}

static void callback(int channel,int result)
{ assert(channel==2&&result==17);callback_calls++; }
int main(void)
{
    CardState* state=calloc(1,sizeof(*state));assert(state);
    char* name=malloc(64);char* image=malloc(64);assert(name&&image);
    memset(name,'n',64);memset(image,'i',64);
    assert((uintptr_t)state>UINT32_MAX&&(uintptr_t)image>UINT32_MAX);
    /* Initialization must clear the real native storage, not the old BSS alias. */
    memset(hsd_native_card_requests,0xa5,sizeof(hsd_native_card_requests));
    hsd_803B2374();
    for(unsigned i=0;i<32;i++)assert(!hsd_native_card_requests[i].type);
    for(unsigned i=0;i<32;i++){
        assert(!hsd_803B286C((s32*)state,name,name,(HSD_CardWord)image,(HSD_CardWord)(image+32),callback));
        HsdCmdEntry* e=&hsd_native_card_requests[i];
        assert(e->type==3&&(CardState*)e->f1==state&&(char*)e->f2==name);
        assert((char*)e->f3==image&&(char*)e->f4==image+32&&e->x14==callback);
    }
    assert(hsd_804D7994==0&&hsd_803B286C((s32*)state,name,name,0,0,callback)==-265);
    assert(!memcmp(state->x370,name,64));
    hsd_native_card_requests[0].type=0;hsd_804D7990=1;
    assert(!hsd_803B2928((s32*)state,name,(HSD_CardWord)image,(HSD_CardWord)(image+32),callback));
    assert(hsd_804D7994==1&&hsd_native_card_requests[0].type==4);
    assert((char*)hsd_native_card_requests[0].f3==image);
    hsd_803B2374();
    assert(!hsd_803B27F4((s32*)state,name,(HSD_CardWord)image,(HSD_CardWord)(image+32),callback));
    assert(hsd_native_card_requests[0].type==6&&(char*)hsd_native_card_requests[0].f2==name);
    assert(!hsd_803B29D8((s32*)state,2,(u8*)image,callback));
    assert(hsd_native_card_requests[1].type==1&&(char*)hsd_native_card_requests[1].f3==image);
    state->x4C[2]=1;
    assert(!hsd_803B2A4C((s32*)state,2,(u8*)image,callback));
    assert(hsd_native_card_requests[2].type==2&&(char*)hsd_native_card_requests[2].f3==image);
    hsd_native_card_requests[2].x14(2,17);assert(callback_calls==1);
    state->x4C[2]=0;assert(hsd_803B2A4C((s32*)state,2,(u8*)image,callback)==-257);
    hsd_803B2374();expected_state=state;state->x4=2;
    assert(!hsd_803B2550((s32*)state,name,callback));
    assert(opens==3&&closes==2);
    assert(hsd_native_card_requests[0].type==5);
    assert((CardState*)hsd_native_card_requests[0].f1==state);
    assert(hsd_native_card_requests[0].f2==19&&hsd_native_card_requests[0].x14==callback);
    hsd_803B2374();
    HSD_CardWord command[9]={16,(HSD_CardWord)state,2,3,4,5,(HSD_CardWord)image,7,8};
    for(unsigned i=0;i<4;i++)hsd_native_card_commands.context[i]=0x123456789L+i;
    for(unsigned i=0;i<128;i++){
        command[3]=i;
        assert(!fn_803AC168(command));
        assert(!memcmp(hsd_native_card_commands.commands[i],command,sizeof(command)));
    }
    assert(fn_803AC168(command)==-265);
    for(unsigned i=0;i<4;i++)assert(hsd_native_card_commands.context[i]==0x123456789L+i);
    for(unsigned i=0;i<128;i++){
        assert(hsd_native_card_commands.commands[i][3]==i);
        assert((CardState*)hsd_native_card_commands.commands[i][1]==state);
        assert((char*)hsd_native_card_commands.commands[i][6]==image);
    }
    hsd_native_card_commands.commands[0][0]=0;hsd_804D7980=1;
    assert(!fn_803AC168(command)&&hsd_804D7984==1);
    hsd_803B2374();assert(!fn_803AC2A4(state));
    assert(hsd_native_card_commands.commands[0][0]==14);
    assert((CardState*)hsd_native_card_commands.commands[0][1]==state);
    hsd_803B2374();state->x8=8192;
    assert(!fn_803B26CC(state,(HSD_CardWord)name,(HSD_CardWord)image,(HSD_CardWord)(image+32),callback));
    assert(hsd_native_card_commands.commands[0][0]==11);
    assert((char*)hsd_native_card_commands.commands[0][3]==name);
    assert((char*)hsd_native_card_commands.commands[0][4]==image);
    assert((char*)hsd_native_card_commands.commands[0][5]==image+32);
    assert((CardState*)hsd_native_card_commands.context[1]==state);
    assert(hsd_native_card_commands.context[2]==(HSD_CardWord)callback);
    hsd_803B2374();
    assert(!fn_803ADE4C((HSD_CardWord)state,2,(HSD_CardWord)callback));
    assert((CardState*)hsd_native_card_commands.commands[0][1]==state);
    assert((CardState*)hsd_native_card_commands.commands[1][1]==state);
    assert(hsd_native_card_commands.context[2]==(HSD_CardWord)callback);
    hsd_803B2374();free(image);free(name);free(state);
    puts("Original card request producers: full-width pointers, callbacks, outer/inner capacity, wrap and reset passed");
}
