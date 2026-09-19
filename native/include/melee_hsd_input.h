#ifndef MELEE_HSD_INPUT_H
#define MELEE_HSD_INPUT_H
#include <stdbool.h>
#include <stdint.h>

/* Small host-facing view of the original HSD_PadGameStatus. The underlying
 * controller.c / rumble.c are linked and executed, not reimplemented. */
typedef struct {
    uint32_t button, trigger, release, repeat;
    float stick_x, stick_y, c_x, c_y, left, right;
    int error;
} MeleeGamePad;
void melee_hsd_input_init(void);
void melee_hsd_input_frame(MeleeGamePad output[4]);
#endif
