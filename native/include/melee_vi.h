#ifndef MELEE_VI_H
#define MELEE_VI_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Native retrace clock. Presentation consumes the latched snapshot; these
 * pointers are opaque framebuffer identities, never host-readable pixels. */
typedef struct MeleeVIPresentation {
    void* framebuffer;
    bool black;
    uint32_t retrace;
} MeleeVIPresentation;
void melee_vi_init(void);
/* Optional foreground 60 Hz display pacing (NTSC ratio retained).
 * Pulses are nonblocking and coalesce;
 * callbacks still execute on the VI worker under the interrupt gate. Disable
 * before stopping the display link to resume the autonomous NTSC clock. */
void melee_vi_set_display_clock(bool enabled);
void melee_vi_display_tick(void);
/* Init/shutdown are serialized by the lifecycle owner. Shutdown requires
 * interrupts enabled and no concurrent waiters; never call from a callback. */
void melee_vi_shutdown(void);
void melee_vi_flush(void);
void melee_vi_configure(uint32_t tv_mode, uint16_t y_origin);
MeleeVIPresentation melee_vi_presentation(void);
#ifdef __cplusplus
}
#endif
#endif
