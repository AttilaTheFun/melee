#ifndef MELEE_HOST_TYPES_H
#define MELEE_HOST_TYPES_H
#include <stdbool.h>

/* Independent of Runtime/platform.h's 32-bit game bool, including when a
 * translation unit includes the game headers before this host interface. */
#ifdef __cplusplus
typedef decltype(true) MeleeHostBool;
#else
typedef _Bool MeleeHostBool;
#endif
#endif
