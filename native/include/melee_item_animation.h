#ifndef MELEE_NATIVE_ITEM_ANIMATION_H
#define MELEE_NATIVE_ITEM_ANIMATION_H
#include "melee_scene.h"
/* Decode the three animation pointers in one ItemStateDesc against its model.
 * The script is handled separately. Input must already have external symbols
 * resolved according to the archive owner's policy. NONE means a null model.
 * Returns an owned scene with attached animation, or NULL on unsupported data.
 * States with no model must be handled by the article owner. */
MeleeScene* melee_item_animation_decode(const MeleeArchive*,uint32_t joint,uint32_t state);
#endif
