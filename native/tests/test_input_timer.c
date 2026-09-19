/* Runs the original game's alarm-driven input pipeline. Only CARDProbe is a
 * test double: no physical/native card store is attached in this input test. */
#include <dolphin/os.h>
#include <dolphin/card.h>
#include <melee/lb/lb_0195.h>
#include <sysdolphin/baselib/controller.h>
#include "melee_hsd_input.h"
#include "melee_pad_backend.h"
#include "melee_alarm_backend.h"
#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

static atomic_int card_probes;
int CARDProbe(s32 channel)
{
    assert(channel == 0 || channel == 1);
    atomic_fetch_add(&card_probes, 1);
    return 0;
}

static void initialize(void) { PADSetSpec(PAD_SPEC_5); }

static void sample(u16 buttons, s8 x)
{
    BOOL enabled = OSDisableInterrupts();
    /* Drain old queued samples before publishing the next test event. */
    while (HSD_PadGetRawQueueCount()) lb_800198E0();
    PADStatus pads[4] = {0};
    pads[0].button = buttons;
    pads[0].stickX = x;
    for (int i = 1; i < 4; ++i) pads[i].err = PAD_ERR_NO_CONTROLLER;
    melee_pad_publish(pads, 0);
    OSRestoreInterrupts(enabled);
    OSTime deadline = OSGetTime() + OSSecondsToTicks((OSTime)2);
    while (!HSD_PadGetRawQueueCount()) {
        assert(OSGetTime() < deadline);
        struct timespec delay = {.tv_nsec=1000000};
        nanosleep(&delay, NULL);
    }
    enabled = OSDisableInterrupts();
    lb_800198E0();
    lb_80019900();
    assert(lb_80019A30(0));
    OSRestoreInterrupts(enabled);
}

int main(void)
{
    melee_hsd_input_init();
    lb_80019AAC(initialize);
    assert(PADGetSpec() == PAD_SPEC_5);
    assert(melee_pad_sampling_rate() == 11);
    sample(PAD_BUTTON_A, 80);
    assert(HSD_PadGameStatus[0].button & PAD_BUTTON_A);
    assert(HSD_PadGameStatus[0].trigger & PAD_BUTTON_A);
    assert(HSD_PadGameStatus[0].nml_stickX == 1.0f);
    sample(PAD_BUTTON_A, 80);
    assert(!(HSD_PadGameStatus[0].trigger & PAD_BUTTON_A));
    sample(0, -80);
    assert(HSD_PadGameStatus[0].release & PAD_BUTTON_A);
    assert(HSD_PadGameStatus[0].nml_stickX == -1.0f);
    BOOL enabled = OSDisableInterrupts();
    lb_80019880(OSMillisecondsToTicks((OSTime)8));
    lb_80019628();
    assert(melee_pad_sampling_rate() == 8);
    OSRestoreInterrupts(enabled);
    sample(PAD_BUTTON_B, 0);
    assert(HSD_PadGameStatus[0].trigger & PAD_BUTTON_B);
    assert(atomic_load(&card_probes) >= 12);
    lb_80019A48();
    melee_native_alarm_shutdown();
    int probes = atomic_load(&card_probes);
    struct timespec delay = {.tv_nsec=20000000};
    nanosleep(&delay, NULL);
    assert(atomic_load(&card_probes) == probes);
    PADSetSamplingRate(999);
    assert(melee_pad_sampling_rate() == 11);
    PADSetSamplingRate(0);
    assert(melee_pad_sampling_rate() == 0);
    puts("Original game input timer: native alarm polling, four-port PAD snapshots, button edges, stick range, rate change and cancellation passed");
    return 0;
}
