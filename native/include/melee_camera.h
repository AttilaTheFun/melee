#ifndef MELEE_NATIVE_CAMERA_H
#define MELEE_NATIVE_CAMERA_H
#include "melee_archive.h"
#include <sysdolphin/baselib/forward.h>
typedef struct MeleeCamera MeleeCamera;
/* Owned static camera descriptor. Custom classes and world-object constraints
 * are rejected. Animation is a separate scene-level resource. */
MeleeCamera* melee_camera_decode(const MeleeArchive* archive, uint32_t offset);
HSD_CObjDesc* melee_camera_descriptor(MeleeCamera* camera);
/* Decode an explicitly empty animation; animated tracks remain unsupported. */
MeleeHostBool melee_camera_empty_animation_decode(const MeleeArchive*,uint32_t,HSD_CameraAnim*);
typedef struct MeleeCameraAnimation MeleeCameraAnimation;
/* Owned camera and eye/interest XYZ tracks. Constraints and spline paths are
 * rejected until their ownership is supported here. */
MeleeCameraAnimation* melee_camera_animation_decode(const MeleeArchive*, uint32_t);
HSD_CameraAnim* melee_camera_animation_descriptor(MeleeCameraAnimation*);
void melee_camera_animation_free(MeleeCameraAnimation*);
void melee_camera_free(MeleeCamera* camera);
#endif
