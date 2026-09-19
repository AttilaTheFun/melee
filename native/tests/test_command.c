#include <melee/lb/lbcommand.h>
#include <melee/lb/types.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    union CmdUnion program[16]={0};CommandInfo info={0};
    info.u=program;info.timer=-0.5f;program[0].Command_00.value=3;
    Command_01(&info);assert(info.u==program+1 && info.timer==2.5f);
    info.frame_count=7.5f;program[1].Command_02.value=12;
    Command_02(&info);assert(info.u==program+2 && info.timer==4.5f);
    Command_08(&info);assert(info.u==program+3 && info.timer==F32_MAX);
    Command_00(&info);assert(!info.u);
    /* Nested loops use four slots; a subroutine at the inner body uses the
     * fifth. All return addresses must survive on a 64-bit host. */
    memset(&info,0,sizeof(info));info.u=program;
    program[0].Command_03.value=2;program[1].Command_03.value=3;
    program[3].Command_05.ptr=program+10;
    unsigned bodies=0;
    Command_03(&info);assert(info.loop_count==2 && info.u==program+1);
    for(unsigned outer=0;outer<2;outer++){
        Command_03(&info);assert(info.loop_count==4 && info.u==program+2);
        for(unsigned inner=0;inner<3;inner++){
            Command_05(&info);assert(info.loop_count==5 && info.u==program+10);
            bodies++;Command_06(&info);assert(info.loop_count==4 && info.u==program+4);
            Command_04(&info);assert(info.u==program+(inner==2?5:2));
        }
        assert(info.loop_count==2);Command_04(&info);
        assert(info.u==program+(outer==1?6:1));
    }
    assert(bodies==6 && !info.loop_count);
    info.u=program+7;program[8].Command_07.ptr=program+12;
    Command_07(&info);assert(info.u==program+12);
    info.u=program;program[0].Command_03.value=0;
    Command_03(&info);info.u=program+1;Command_04(&info);
    assert((uintptr_t)info.event_return[1]==UINT32_MAX && info.u==program+1);
    puts("Original script commands: timers, nested loops, five-slot return stack, subroutines, goto and 32-bit counter wrap passed");
}
