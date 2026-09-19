#include "melee_action.h"
#include "melee_fighter_command.h"
#include <melee/lb/lbcommand.h>
#include <melee/lb/types.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
extern u32 ftAction_CommandWordCount(u32 opcode);
struct MeleeAction {
    union CmdUnion* words;
    uint32_t* raw;
    uint8_t* starts;
    size_t count,root;
    MeleeHostBool fighter;
    CommandInfo command;
    uint8_t stack_kind[5];
    MeleeActionResult fault;
};
void melee_action_free(MeleeAction* a){if(a){free(a->words);free(a->raw);free(a->starts);free(a);}}
void melee_action_reset(MeleeAction* a){if(a){memset(&a->command,0,sizeof(a->command));memset(a->stack_kind,0,sizeof(a->stack_kind));a->command.u=a->words+a->root;a->fault=MELEE_ACTION_WAIT;}}
static MeleeAction* decode(const MeleeArchive* archive,const uint32_t* roots,size_t root_count,MeleeHostBool fighter)
{
    if(!archive || !archive->bytes || !roots || !root_count || root_count>archive->data_size/4)return NULL;
    for(size_t i=0;i<root_count;i++)if((roots[i]&3)||roots[i]>=archive->data_size)return NULL;
    uint32_t root=roots[0];
    MeleeAction* a=calloc(1,sizeof(*a));if(!a)return NULL;
    a->count=archive->data_size/4;a->root=root/4;a->fighter=fighter;
    a->words=calloc(a->count,sizeof(*a->words));a->raw=calloc(a->count,sizeof(*a->raw));a->starts=calloc(a->count,1);
    size_t* pending=malloc((a->count?a->count:1)*sizeof(*pending));size_t count=0;
    if(!a->words || !a->raw || !a->starts || !pending)goto fail;
    for(size_t i=0;i<root_count;i++)pending[count++]=roots[i]/4;
    while(count){
        size_t pc=pending[--count];
        for(;;){
            if(pc>=a->count || a->starts[pc]==2)goto fail;
            if(a->starts[pc]==1)break;
            uint32_t word;
            if(!melee_archive_u32(archive,(uint32_t)pc*4,&word))goto fail;
            uint32_t opcode=word>>26,length=ftAction_CommandWordCount(opcode);
            if(!length || length>a->count-pc)goto fail;
            a->starts[pc]=1;
            for(size_t i=0;i<length;i++){
                if(i && a->starts[pc+i])goto fail;
                if(i)a->starts[pc+i]=2;
                if(!melee_archive_u32(archive,(uint32_t)(pc+i)*4,a->raw+pc+i))goto fail;
            }
            a->words[pc].Command_00.code=opcode;
            a->words[pc].Command_00.value=word&0x03ffffff;
            if(fighter && opcode>=9){
                if(opcode==9){
                    a->words[pc].Command_09=(struct Command_09){9,(word>>18)&255,word&0x3ffff};
                }else if(opcode==11){
                    uint32_t following;
                    if(!melee_archive_u32(archive,(uint32_t)(pc+length)*4,&following)||
                       !melee_fighter_hitbox_decode(a->raw+pc,length,following,a->words+pc))goto fail;
                }else if(!melee_fighter_command_decode(a->raw+pc,length,a->words+pc))goto fail;
            }
            if(opcode==5 || opcode==7){
                uint32_t target;MeleeHostBool present;
                if(!melee_archive_pointer(archive,(uint32_t)(pc+1)*4,&target,&present))goto fail;
                /* The original interpreter stops when a call or jump assigns
                 * NULL to its command cursor. Pichu uses this terminator.
                 * A relocated offset zero still points to the first word. */
                if(!present){a->words[pc+1].Command_05.ptr=NULL;break;}
                if((target&3) || target/4>=a->count)goto fail;
                a->words[pc+1].Command_05.ptr=a->words+target/4;
                if(opcode==7){pc=target/4;continue;}
                if(count>=a->count)goto fail;pending[count++]=target/4;
            }
            if(opcode==0 || opcode==6)break;
            pc+=length;
        }
    }
    free(pending);melee_action_reset(a);return a;
fail:free(pending);melee_action_free(a);return NULL;
}
MeleeAction* melee_action_decode(const MeleeArchive* archive,uint32_t root)
{return decode(archive,&root,1,false);}
MeleeAction* melee_fighter_actions_decode(const MeleeArchive* archive,const uint32_t* roots,size_t count)
{return decode(archive,roots,count,true);}
union CmdUnion* melee_fighter_actions_script(MeleeAction* a,uint32_t offset)
{
    if(!a||!a->fighter||(offset&3)||offset/4>=a->count||a->starts[offset/4]!=1)return NULL;
    return a->words+offset/4;
}
MeleeActionResult melee_action_step(MeleeAction* a,float frame,float delta,size_t budget,MeleeActionCallback callback,void* context)
{
    if(!a)return MELEE_ACTION_INVALID_STACK;
    if(a->fault!=MELEE_ACTION_WAIT)return a->fault;
    if(!isfinite(frame) || !isfinite(delta) || delta<0){a->fault=MELEE_ACTION_INVALID_STACK;return a->fault;}
    CommandInfo* c=&a->command;c->frame_count=frame;
    if(c->timer!=F32_MAX)c->timer-=delta;
    while(c->u){
        if(c->timer==F32_MAX){if(frame>=delta)return MELEE_ACTION_WAIT;c->timer=-frame;}
        else if(c->timer>0)return MELEE_ACTION_WAIT;
        if(!budget--){a->fault=MELEE_ACTION_BUDGET;return a->fault;}
        uintptr_t address=(uintptr_t)c->u,base=(uintptr_t)a->words;
        if(address<base || (address-base)%sizeof(*a->words) || (address-base)/sizeof(*a->words)>=a->count){a->fault=MELEE_ACTION_INVALID_STACK;return a->fault;}
        size_t pc=(address-base)/sizeof(*a->words);if(a->starts[pc]!=1){a->fault=MELEE_ACTION_INVALID_STACK;return a->fault;}
        uint32_t opcode=a->raw[pc]>>26;
        if((opcode==3 && c->loop_count>3) || (opcode==5 && c->loop_count>=5) || (opcode==4 && c->loop_count<2) || (opcode==6 && !c->loop_count)){
            a->fault=MELEE_ACTION_INVALID_STACK;return a->fault;
        }
        if((opcode==4 && (a->stack_kind[c->loop_count-1]!=2 || a->stack_kind[c->loop_count-2]!=1)) ||
           (opcode==6 && a->stack_kind[c->loop_count-1]!=3)){
            a->fault=MELEE_ACTION_INVALID_STACK;return a->fault;
        }
        if(opcode==3){a->stack_kind[c->loop_count]=1;a->stack_kind[c->loop_count+1]=2;}
        if(opcode==5)a->stack_kind[c->loop_count]=3;
        switch(opcode){
        case 0:Command_00(c);break;case 1:Command_01(c);break;case 2:Command_02(c);break;
        case 3:Command_03(c);break;case 4:Command_04(c);break;case 5:Command_05(c);break;
        case 6:Command_06(c);break;case 7:Command_07(c);break;case 8:Command_08(c);break;
        default:{
            MeleeActionEvent event={.offset=(uint32_t)pc*4,.opcode=opcode,.count=ftAction_CommandWordCount(opcode)};
            memcpy(event.words,a->raw+pc,event.count*4);
            if(!callback || !callback(context,&event)){a->fault=MELEE_ACTION_UNHANDLED;return a->fault;}
            c->u+=event.count;break;
        }}
        if(c->timer==F32_MAX)return MELEE_ACTION_WAIT;
    }
    return MELEE_ACTION_DONE;
}
