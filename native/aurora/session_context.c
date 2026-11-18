/* Called only at a completed game-frame boundary, using the game's SDK ABI. */
#include <melee/gm/gm_1A3F.h>
#include <melee/gr/types.h>
#include <melee/gr/ground.h>
#include <melee/gr/stage.h>
#include <stdint.h>
uint64_t melee_game_session_context(void) {
    return ((uint64_t)(uint16_t)gm_GetCurrentGameMode() << 32) |
           ((uint64_t)(uint16_t)gm_GetCurrentSceneIndex() << 16) |
           (uint16_t)stage_info.grkind;
}
