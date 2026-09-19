#include "melee_ax_aux.h"
#include "../../extern/dolphin/src/dolphin/ax/__ax.h"
#include <dolphin/os.h>
#include <string.h>

static bool initialized;
static bool processing;

bool melee_ax_aux_ready(void)
{
    int old = OSDisableInterrupts();
    bool ready = initialized && !processing && __AXClMode <= 3;
    OSRestoreInterrupts(old);
    return ready;
}

bool melee_ax_aux_init(void)
{
    int old = OSDisableInterrupts();
    bool success = !processing;
    if (success) { __AXAuxInit(); initialized = true; }
    OSRestoreInterrupts(old);
    return success;
}

static void transfer(s32 source[3][160], s32 main[3][160],
                      AXAuxAddress input, AXAuxAddress output)
{
    if (!input) return;
    memcpy((void*) input, source, 3 * 160 * sizeof(s32));
    const s32* returned = (const s32*) output;
    for (unsigned channel = 0; channel < 3; ++channel) {
        for (unsigned sample = 0; sample < 160; ++sample) {
            u32 sum = (u32) main[channel][sample] + (u32) returned[channel * 160 + sample];
            memcpy(&main[channel][sample], &sum, sizeof(sum));
        }
    }
}

bool melee_ax_aux_process_frame(MeleeAXMixBuffers* buffers)
{
    if (!buffers) return false;
    int old = OSDisableInterrupts();
    bool success = false;
    if (!initialized || processing || __AXClMode > 3) goto finish;
    processing = true;
    AXAuxAddress input, output;
    __AXGetAuxAInput(&input); __AXGetAuxAOutput(&output);
    transfer(buffers->auxA, buffers->main, input, output);
    __AXGetAuxBInput(&input); __AXGetAuxBOutput(&output);
    transfer(buffers->auxB, buffers->main, input, output);
    __AXProcessAux();
    processing = false;
    success = true;
finish:
    OSRestoreInterrupts(old);
    return success;
}
