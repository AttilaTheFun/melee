#ifndef MELEE_NATIVE_ANIMATION_H
#define MELEE_NATIVE_ANIMATION_H
#include "melee_archive.h"
#include <sysdolphin/baselib/fobj.h>
#include <melee/lb/lbanim.h>

/* Owned native descriptors and copied command bytes. Keep this owner alive
 * until every HSD_FObj loaded from its descriptors has been removed. */
typedef struct MeleeAnimationTracks MeleeAnimationTracks;
MeleeAnimationTracks* melee_animation_tracks_decode(const MeleeArchive* archive,
                                                    uint32_t offset);
HSD_FObjDesc* melee_animation_tracks_descriptors(MeleeAnimationTracks* tracks);
void melee_animation_tracks_free(MeleeAnimationTracks* tracks);
MeleeHostBool melee_animation_stream_valid(const uint8_t* bytes, size_t length,
                                           uint8_t value_format, uint8_t slope_format);
/* Fighter motion archives use FigaTree and a flat track array per joint.
 * The returned owner retains all nodes, tracks and bytecode. It must outlive
 * HSD objects that borrow its streams, just like MeleeAnimationTracks. */
typedef struct MeleeFighterAnimation MeleeFighterAnimation;
MeleeFighterAnimation* melee_fighter_animation_decode(const MeleeArchive* archive, uint32_t offset);
FigaTree* melee_fighter_animation_tree(MeleeFighterAnimation* animation);
size_t melee_fighter_animation_joint_count(const MeleeFighterAnimation* animation);
void melee_fighter_animation_free(MeleeFighterAnimation* animation);
#endif
