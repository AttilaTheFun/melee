#ifndef MELEE_AX_DEPOP_H
#define MELEE_AX_DEPOP_H
#include "melee_ax_mix.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Zero-initialize while audio is stopped. Sums belong to the mixer, not an
 * individual voice, and survive voice release/reuse. */
typedef struct MeleeAXDepop {
    s32 main[3];
    s32 auxA[3];
    s32 auxB[3];
} MeleeAXDepop;
/* Add a retired voice's last rendered contributions exactly once. The caller
 * owns retirement tracking; this does not change the voice parameter block. */
bool melee_ax_depop_add(MeleeAXDepop* state, const AXPBDPOP* last);
/* Initialize all nine frame buses with the pending fades, advancing their
 * sums by one frame. Call before mixing voices or auxiliary returns. */
bool melee_ax_depop_begin_frame(MeleeAXDepop* state, MeleeAXMixBuffers* buffers);
#ifdef __cplusplus
}
#endif
#endif
