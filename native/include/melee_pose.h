#ifndef MELEE_NATIVE_POSE_H
#define MELEE_NATIVE_POSE_H
#include "melee_joint.h"
#include "melee_animation.h"
#include <sysdolphin/baselib/jobj.h>
/* Owned transform-evaluation records, not registered scene-class objects.
 * Uses original HSD joint updates/matrix math. Rendering payloads, constraints,
 * instance graphs, custom classes, user matrices and quaternion animation
 * are not yet bound.
 * Initialize the HSD FObj/AObj/Vec pools and native heap before use. */
typedef struct MeleePose MeleePose;
MeleePose* melee_pose_create(const MeleeJointGraph* graph);
void melee_pose_free(MeleePose* pose);
size_t melee_pose_count(const MeleePose* pose);
const HSD_JObj* melee_pose_joint(const MeleePose* pose, size_t index);
uint32_t melee_pose_source_offset(const MeleePose* pose, size_t index);
/* Index order is depth-first preorder, as in ftParts_SetupParts.
 * Explicit mappings map animation node index to pose index. NULL permits only
 * equal-count preorder binding; it cannot resolve fighter-specific part tables.
 * The animation must outlive the binding. Failed binding leaves the old one. */
MeleeHostBool melee_pose_bind(MeleePose* pose, MeleeFighterAnimation* animation,
                             const size_t* mapping);
void melee_pose_unbind(MeleePose* pose);
MeleeHostBool melee_pose_request(MeleePose* pose, float frame);
MeleeHostBool melee_pose_set_rate(MeleePose* pose, float rate);
/* First step after request evaluates that exact frame; later steps advance. */
MeleeHostBool melee_pose_step(MeleePose* pose);
#endif
