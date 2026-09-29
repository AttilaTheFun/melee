#include <dolphin/os.h>
#include "melee_pad_backend.h"
#include <pthread.h>
#include <stdatomic.h>

/* Host threads have no GameCube interrupt mask. Serialize cooperating game
 * critical sections instead. The previous-state token retains the SDK's
 * nesting semantics: restoring false must leave the outer lock held.
 * This does not stop arbitrary host threads or implement an OS scheduler. */
static pthread_mutex_t interrupt_gate = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local MeleeHostBool enabled = true;
static atomic_bool reset_pressed;
#ifdef __EMSCRIPTEN__
int melee_native_interrupts_enabled(void) { return enabled; }
#endif

BOOL OSDisableInterrupts(void)
{
    MeleeHostBool previous = enabled;
    if (previous) { pthread_mutex_lock(&interrupt_gate); enabled = false; }
    return previous;
}

BOOL OSRestoreInterrupts(BOOL level)
{
    MeleeHostBool previous = enabled;
    if (level && !enabled) { enabled = true; pthread_mutex_unlock(&interrupt_gate); }
    else if (!level && enabled) { pthread_mutex_lock(&interrupt_gate); enabled = false; }
    return previous;
}

BOOL OSEnableInterrupts(void) { return OSRestoreInterrupts(true); }
BOOL OSGetResetSwitchState(void) { return atomic_load(&reset_pressed); }
void melee_native_reset_switch(MeleeHostBool pressed) { atomic_store(&reset_pressed, pressed); }
