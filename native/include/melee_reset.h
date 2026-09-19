#ifndef MELEE_NATIVE_RESET_H
#define MELEE_NATIVE_RESET_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    int kind; /* SDK restart=0, hot reset=1, shutdown=2 */
    uint32_t code;
    int force_menu;
} MeleeResetRequest;
/* The handler must transfer control to the host without returning. It runs on
 * the requesting game thread with the native interrupt gate released. The
 * host owns worker shutdown and fresh runtime construction; this API does not
 * reset static game state or emulate a console menu. Configure before startup,
 * and only reconfigure after the previous game and its workers have stopped. */
typedef void (*MeleeResetHandler)(MeleeResetRequest request, void* context);
void melee_reset_configure(uint32_t boot_code, MeleeResetHandler handler, void* context);
#ifdef __cplusplus
}
#endif
#endif
