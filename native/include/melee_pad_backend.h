#ifndef MELEE_PAD_BACKEND_H
#define MELEE_PAD_BACKEND_H
#include <dolphin/pad.h>
#include "melee_host_types.h"

/* Atomically publish one complete four-port sample. Values are raw PAD units;
 * HSD owns clamping, normalization and button edges. Only advertise a motor
 * bit when the host can actually perform that port's rumble commands. */
void melee_pad_publish(const PADStatus status[4], u32 motor_mask);
void melee_pad_disconnect_all(void);
u32 melee_pad_motor(unsigned port);
/* Requested console SI polling interval (0..11). Host events update the latest
 * decoded sample; the original game alarm determines HSD read cadence. This
 * value does not throttle host input events or pretend to configure SI. */
u32 melee_pad_sampling_rate(void);
void melee_native_reset_switch(MeleeHostBool pressed);
#endif
