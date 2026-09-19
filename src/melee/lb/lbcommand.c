#include "lbcommand.h"

#include "inlines.h"
#include "lb_0219.h"
#include "types.h"
#ifdef MELEE_NATIVE
#include <stdint.h>
#endif

void (*lbCommand_803B9840[16])(CommandInfo*) = {
    Command_00, Command_01, Command_02, Command_03, Command_04, Command_05,
    Command_06, Command_07, Command_08, Command_09, NULL,       NULL,
    NULL,       NULL,       NULL,       NULL
};

/// Reset
void Command_00(CommandInfo* info)
{
    info->u = NULL;
}

/// SynchronousTimer
void Command_01(CommandInfo* info)
{
    info->timer += info->u->Command_00.value;
    NEXT_CMD(info);
}

/// AsynchronousTimer
void Command_02(CommandInfo* info)
{
    info->timer = info->u->Command_02.value - info->frame_count;
    NEXT_CMD(info);
}

/// SetLoop
void Command_03(CommandInfo* info)
{
    info->event_return[info->loop_count++] = info->u + 1;
    info->event_return[info->loop_count++] =
#ifdef MELEE_NATIVE
        (union CmdUnion*) (uintptr_t) info->u->Command_03.value;
#else
        (union CmdUnion*) info->u->Command_03.value;
#endif
    NEXT_CMD(info);
}

/// Execute Loop
void Command_04(CommandInfo* info)
{
#ifdef MELEE_NATIVE
    /* The console alias below addresses a four-byte stack slot through the
     * CommandInfo header. Native stack entries are full-width pointers. Loop
     * counters retain the console's unsigned 32-bit decrement semantics. */
    u32 count = (u32) (uintptr_t) info->event_return[info->loop_count - 1];
    count -= 1;
    info->event_return[info->loop_count - 1] =
        (union CmdUnion*) (uintptr_t) count;
    if (count != 0) {
        info->u = info->event_return[info->loop_count - 2];
        return;
    }
#else
    u32* ptr = (u32*) info;
    ptr[info->loop_count + 3] -= 1;

    if ((s32) info->event_return[info->loop_count - 1]) {
        info->ptr[0] = &info->ptr[info->loop_count][0];
        return;
    }
#endif
    NEXT_CMD(info);
    info->loop_count -= 2;
}

/// Subroutine
void Command_05(CommandInfo* info)
{
    NEXT_CMD(info);
    info->event_return[info->loop_count++] = info->u + 1;
    info->u = info->u->Command_05.ptr;
}

/// Return
void Command_06(CommandInfo* info)
{
    info->u = info->event_return[info->loop_count -= 1];
}

/// Goto
void Command_07(CommandInfo* info)
{
    NEXT_CMD(info);
    info->u = info->u->Command_07.ptr;
}

/// SetTimerAnimation
void Command_08(CommandInfo* info)
{
    NEXT_CMD(info);
    info->timer = F32_MAX;
}

void Command_09(CommandInfo* info)
{
    lbBgFlash_80021C48(info->u->Command_09.param_1,
                       info->u->Command_09.param_2);
    NEXT_CMD(info);
}

bool Command_Execute(CommandInfo* info, u32 command)
{
    if (command < 10) {
        lbCommand_803B9840[command](info);
        return true;
    }
    return false;
}
