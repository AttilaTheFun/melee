#include "melee_ax_output.h"
#include "melee_ax_aux.h"
#include "melee_ax_pool_mix.h"
#include "melee_ax_voice.h"
#include "../../extern/dolphin/src/dolphin/ax/__ax.h"
#include <dolphin/os.h>
#include <string.h>

static bool rendering;
static void (*frame_callback)(void);

void AXInit(void)
{
    int old = OSDisableInterrupts();
    /* Apple output scheduling starts separately, after game initialization.
     * Native frame/depop/surround history resets on the new pool generation. */
    if (rendering || !melee_ax_aux_init())
        OSPanic(__FILE__, __LINE__, "AXInit during native audio rendering");
    melee_ax_voice_pool_init();
    __AXClMode = 0;
    frame_callback = NULL;
    OSRestoreInterrupts(old);
}

void AXRegisterCallback(void (*callback)())
{
    int old = OSDisableInterrupts();
    frame_callback = callback;
    OSRestoreInterrupts(old);
}

static s16 clamp(s32 value)
{
    return value < -32768 ? -32768 : value > 32767 ? 32767 : (s16) value;
}

bool melee_ax_output_frame(MeleeAXOutput* state, s16* stereo)
{
    if (!state || !stereo) return false;
    int old = OSDisableInterrupts();
    bool success = false;
    u64 generation = melee_ax_voice_generation();
    if (rendering || !generation || !melee_ax_aux_ready()) goto finish;
    rendering = true;
    s32 previous[MELEE_AX_FRAME_SAMPLES] = {0};
    if (state->generation == generation)
        memcpy(previous, state->surround, sizeof(previous));
    MeleeAXMixBuffers buffers;
    if (!melee_ax_voice_pool_render_output(&buffers, previous, __AXClMode)) goto rendered;
    /* The gate excludes registration/mode changes; no callbacks run between
     * the preflight and this call, so aux processing cannot reject the frame. */
    if (!melee_ax_aux_process_frame(&buffers))
        OSPanic(__FILE__, __LINE__, "AX auxiliary readiness changed inside the render gate");
    for (unsigned i = 0; i < MELEE_AX_FRAME_SAMPLES; ++i) {
        stereo[i * 2] = clamp(buffers.main[0][i]);
        stereo[i * 2 + 1] = clamp(buffers.main[1][i]);
    }
    memcpy(state->surround, buffers.main[2], sizeof(state->surround));
    state->generation = generation;
    /* AXOut invokes the user callback after auxiliary processing. Its edits
     * feed the next frame, while this frame's samples are already complete. */
    if (frame_callback) frame_callback();
    success = true;
rendered:
    rendering = false;
finish:
    OSRestoreInterrupts(old);
    return success;
}
