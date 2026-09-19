#include "melee_pad_backend.h"
#include <dolphin/os.h>
#include <pthread.h>
#include <string.h>

static pthread_mutex_t pad_lock = PTHREAD_MUTEX_INITIALIZER;
static PADStatus latest[4] = {
    {.err = PAD_ERR_NO_CONTROLLER}, {.err = PAD_ERR_NO_CONTROLLER},
    {.err = PAD_ERR_NO_CONTROLLER}, {.err = PAD_ERR_NO_CONTROLLER},
};
static u32 motors[4];
static u32 motor_capabilities;
static u32 sampling_rate;

void melee_pad_publish(const PADStatus status[4], u32 motor_mask)
{
    pthread_mutex_lock(&pad_lock);
    motor_capabilities = motor_mask & 0xF0000000u;
    for (unsigned i = 0; i < 4; ++i) {
        latest[i] = status[i];
        if (latest[i].err != PAD_ERR_NONE) {
            s8 err = latest[i].err;
            memset(&latest[i], 0, sizeof(latest[i]));
            latest[i].err = err;
            motor_capabilities &= ~(PAD_CHAN0_BIT >> i);
        }
        if (!(motor_capabilities & (PAD_CHAN0_BIT >> i))) motors[i] = PAD_MOTOR_STOP;
    }
    pthread_mutex_unlock(&pad_lock);
}

void melee_pad_disconnect_all(void)
{
    PADStatus disconnected[4] = {0};
    for (unsigned i = 0; i < 4; ++i) disconnected[i].err = PAD_ERR_NO_CONTROLLER;
    melee_pad_publish(disconnected, 0);
}

u32 PADRead(PADStatus* status)
{
    pthread_mutex_lock(&pad_lock);
    memcpy(status, latest, sizeof(latest));
    u32 mask = motor_capabilities;
    pthread_mutex_unlock(&pad_lock);
    return mask;
}

BOOL PADInit(void) { return TRUE; }

int PADReset(unsigned long mask)
{
    /* HID discovery/reconnection is handled by the host. Never erase a newly
     * published sample because an earlier HSD sample requested a reset. */
    return (mask & ~0xF0000000UL) == 0;
}

BOOL PADRecalibrate(u32 mask)
{
    /* Host stick axes are already centered; applying the held position as an
     * origin here would produce drift after a reset with a key held down. */
    return PADReset(mask);
}

void PADControlMotor(s32 port, u32 command)
{
    if (port < 0 || port >= 4 || command > PAD_MOTOR_STOP_HARD) return;
    pthread_mutex_lock(&pad_lock);
    if (motor_capabilities & (PAD_CHAN0_BIT >> port)) motors[port] = command;
    pthread_mutex_unlock(&pad_lock);
}

void PADControlAllMotors(const u32* commands)
{
    pthread_mutex_lock(&pad_lock);
    for (unsigned i = 0; i < 4; ++i)
        if ((motor_capabilities & (PAD_CHAN0_BIT >> i)) && commands[i] <= PAD_MOTOR_STOP_HARD)
            motors[i] = commands[i];
    pthread_mutex_unlock(&pad_lock);
}

u32 melee_pad_motor(unsigned port)
{
    if (port >= 4) return PAD_MOTOR_STOP;
    pthread_mutex_lock(&pad_lock);
    u32 command = motors[port];
    pthread_mutex_unlock(&pad_lock);
    return command;
}

/* The host publishes decoded PAD_SPEC_5 states, not SI wire packets. */
void PADSetSpec(u32 spec)
{
    if (spec != PAD_SPEC_5) {
        OSPanic(__FILE__, __LINE__, "Native PAD input requires specification 5");
    }
}

unsigned long PADGetSpec(void)
{
    return PAD_SPEC_5;
}

void PADSetSamplingRate(unsigned long msec)
{
    pthread_mutex_lock(&pad_lock);
    sampling_rate = msec > 11 ? 11 : (u32) msec;
    pthread_mutex_unlock(&pad_lock);
}

u32 melee_pad_sampling_rate(void)
{
    pthread_mutex_lock(&pad_lock);
    u32 result = sampling_rate;
    pthread_mutex_unlock(&pad_lock);
    return result;
}
