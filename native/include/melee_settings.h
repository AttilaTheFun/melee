#ifndef MELEE_SETTINGS_H
#define MELEE_SETTINGS_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Configure before starting the game. Parent directory must exist. Missing
 * files use stereo/non-progressive defaults; malformed files are rejected.
 * Reconfiguration is permitted only while the game is stopped. */
bool melee_settings_open(const char* path);
#ifdef __cplusplus
}
#endif
#endif
