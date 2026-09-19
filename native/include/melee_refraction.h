#ifndef MELEE_NATIVE_REFRACTION_H
#define MELEE_NATIVE_REFRACTION_H
#include "melee_archive.h"
/* Decode the lbRefData parameter pairs into caller-owned storage. */
MeleeHostBool melee_refraction_decode(const MeleeArchive*,float* pairs,size_t capacity,unsigned* count);
#endif
