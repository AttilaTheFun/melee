#include "melee_input.h"
#include "melee_pad_backend.h"
#include "melee_hsd_input.h"
#include <dolphin/os.h>
#include <sysdolphin/baselib/controller.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>

static void publish(u16 buttons, float x, float trigger)
{
    PADStatus ports[4] = {0};
    ports[0] = melee_analog_pad(buttons, x, 0, 0, 0, trigger, 0);
    for (int i = 1; i < 4; ++i) ports[i].err = PAD_ERR_NO_CONTROLLER;
    melee_pad_publish(ports, PAD_CHAN0_BIT);
}

static void game_pipeline(void)
{
    MeleeGamePad p[4];
    melee_pad_disconnect_all();
    melee_hsd_input_init();
    melee_hsd_input_frame(p);
    for (int i = 0; i < 4; ++i) assert(p[i].error == PAD_ERR_NO_CONTROLLER);
    publish(PAD_BUTTON_A, 1, 0);
    melee_hsd_input_frame(p);
    assert(p[0].error == 0 && p[0].stick_x == 1.0f);
    assert((p[0].button & PAD_BUTTON_A) && (p[0].trigger & PAD_BUTTON_A));
    assert(p[0].release == 0);
    melee_hsd_input_frame(p);
    assert((p[0].button & PAD_BUTTON_A) && p[0].trigger == 0);
    /* Run the actual HSD command interpreter on host-endian commands:
     * rumble one frame, hard stop one frame, then terminate. */
    u16 commands[] = {0x2001, 0x6001, 0};
    assert(HSD_PadRumbleAdd(0, 1, -2, 0, commands));
    melee_hsd_input_frame(p); assert(melee_pad_motor(0) == PAD_MOTOR_RUMBLE);
    melee_hsd_input_frame(p); assert(melee_pad_motor(0) == PAD_MOTOR_STOP_HARD);
    melee_hsd_input_frame(p);
    publish(0, 0, 0);
    melee_hsd_input_frame(p);
    assert(!(p[0].button & PAD_BUTTON_A) && (p[0].release & PAD_BUTTON_A));
    publish(PAD_BUTTON_B, -1, 1);
    melee_hsd_input_frame(p);
    assert(p[0].stick_x == -1 && p[0].left == 1);
    assert(p[0].trigger & PAD_BUTTON_B);
    for (int i = 0; i < HSD_PadLibData.repeat_start; ++i) melee_hsd_input_frame(p);
    assert(p[0].repeat & PAD_BUTTON_B);
    publish(0, 0.5f, 0.5f);
    melee_hsd_input_frame(p);
    assert(p[0].stick_x == 0.5f && p[0].left == 0.5f);
    publish(PAD_BUTTON_B, 0, 0);
    melee_hsd_input_frame(p);
    melee_pad_disconnect_all();
    melee_hsd_input_frame(p);
    assert(p[0].error == -1 && p[0].button == 0 && p[0].stick_x == 0);
    assert(p[0].release & PAD_BUTTON_B);
    /* HSD's original reset-switch state machine fires on release. */
    melee_native_reset_switch(true); melee_hsd_input_frame(p);
    assert(!HSD_PadGetResetSwitch());
    melee_native_reset_switch(false); melee_hsd_input_frame(p);
    assert(HSD_PadGetResetSwitch());
    HSD_PadReset(); assert(!HSD_PadGetResetSwitch());
}

static void sdk_contract(void)
{
    PADStatus ports[4];
    melee_pad_disconnect_all();
    assert(PADInit()); assert(PADRead(ports) == 0);
    for (int i = 0; i < 4; ++i) assert(ports[i].err == -1 && ports[i].button == 0);
    publish(PAD_BUTTON_A, 0, 0);
    assert(PADRead(ports) == PAD_CHAN0_BIT);
    assert(PADReset(PAD_CHAN0_BIT)); assert(!PADReset(1));
    assert(PADRecalibrate(PAD_CHAN0_BIT));
    PADRead(ports); assert(ports[0].button == PAD_BUTTON_A);
    PADControlMotor(0, PAD_MOTOR_RUMBLE); assert(melee_pad_motor(0) == PAD_MOTOR_RUMBLE);
    PADControlMotor(-1, PAD_MOTOR_RUMBLE); PADControlMotor(0, 3);
    assert(melee_pad_motor(0) == PAD_MOTOR_RUMBLE);
    u32 commands[4] = {PAD_MOTOR_STOP_HARD, PAD_MOTOR_RUMBLE, 0, 0};
    PADControlAllMotors(commands);
    assert(melee_pad_motor(0) == PAD_MOTOR_STOP_HARD && melee_pad_motor(1) == 0);
    ports[0].err = PAD_ERR_NO_CONTROLLER;
    melee_pad_publish(ports, PAD_CHAN0_BIT);
    assert(PADRead(ports) == 0 && ports[0].button == 0 && melee_pad_motor(0) == 0);
}

static unsigned counter;
static void* count(void* unused)
{
    (void)unused;
    for (int i = 0; i < 2000; ++i) {
        BOOL outer = OSDisableInterrupts(); assert(outer);
        BOOL inner = OSDisableInterrupts(); assert(!inner);
        ++counter;
        assert(!OSRestoreInterrupts(inner));
        assert(!OSRestoreInterrupts(outer));
    }
    return NULL;
}

static void* writer(void* unused)
{
    (void)unused;
    for (int n = 0; n < 5000; ++n) {
        PADStatus ports[4] = {0};
        for (int i = 0; i < 4; ++i) ports[i].button = n & 0xFFF;
        melee_pad_publish(ports, 0);
    }
    return NULL;
}

static void concurrency(void)
{
    pthread_t a, b;
    assert(!pthread_create(&a, NULL, count, NULL));
    assert(!pthread_create(&b, NULL, count, NULL));
    assert(!pthread_join(a, NULL)); assert(!pthread_join(b, NULL));
    assert(counter == 4000);
    melee_pad_disconnect_all();
    assert(!pthread_create(&a, NULL, writer, NULL));
    for (int n = 0; n < 5000; ++n) {
        PADStatus ports[4]; PADRead(ports);
        for (int i = 1; i < 4; ++i) assert(ports[i].button == ports[0].button);
    }
    assert(!pthread_join(a, NULL));
}

int main(void)
{
    sdk_contract(); game_pipeline(); concurrency();
    puts("Original HSD input/rumble pipeline, PAD contracts and native synchronization passed.");
    return 0;
}
