#ifndef MELEE_AX_ITD_H
#define MELEE_AX_ITD_H
#include <dolphin/ax.h>
#include <stdbool.h>
/* Process one 32-sample, post-envelope quantum. Left/right use the previous
 * history followed by current samples. Surround buses use current input
 * directly. Failure preserves state, history and output. */
bool melee_ax_itd_ms(AXPBITD* state, AXPBITDBUFFER* history,
                     const s16 input[32], s16 output[2][32]);
#endif
