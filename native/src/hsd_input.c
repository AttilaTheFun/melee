#include "melee_hsd_input.h"
#include <sysdolphin/baselib/controller.h>
#include <dolphin/os.h>

static HSD_PadData samples[5];
static HSD_PadRumbleListData rumble[12];

void melee_hsd_input_init(void)
{
    bool previous = OSDisableInterrupts();
    HSD_PadInit(5, samples, 12, rumble);
    /* The game's gmmain.c configures these raw PAD ranges at boot. */
    HSD_PadLibData.clamp_stickType = 0;
    HSD_PadLibData.clamp_stickShift = 1;
    HSD_PadLibData.clamp_stickMax = 80;
    HSD_PadLibData.clamp_stickMin = 0;
    HSD_PadLibData.scale_stick = 80;
    HSD_PadLibData.clamp_analogLRShift = 1;
    HSD_PadLibData.clamp_analogLRMax = 140;
    HSD_PadLibData.clamp_analogLRMin = 0;
    HSD_PadLibData.scale_analogLR = 140;
    OSRestoreInterrupts(previous);
}

void melee_hsd_input_frame(MeleeGamePad output[4])
{
    bool previous = OSDisableInterrupts();
    HSD_PadRenewStatus();
    for (unsigned i = 0; i < 4; ++i) {
        const HSD_PadStatus* p = &HSD_PadGameStatus[i];
        output[i] = (MeleeGamePad){.button = p->button, .trigger = p->trigger,
            .release = p->release, .repeat = p->repeat,
            .stick_x = p->nml_stickX, .stick_y = p->nml_stickY,
            .c_x = p->nml_subStickX, .c_y = p->nml_subStickY,
            .left = p->nml_analogL, .right = p->nml_analogR, .error = p->err};
    }
    OSRestoreInterrupts(previous);
}
