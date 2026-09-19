#ifndef MELEE_NATIVE_SCENE_H
#define MELEE_NATIVE_SCENE_H
#include "melee_archive.h"
#include <sysdolphin/baselib/forward.h>
#include <sysdolphin/baselib/spline.h>
typedef struct MeleeScene MeleeScene;
/* Owns archive bytes, host descriptors, and original HSD class instances.
 * Requires initialized HSD heap/pools. Unsupported schemas fail before loading.
 * Supports ordinary SRT and spline-path joints, rigid/envelope and XYZ morph
 * polygons, and materials. Spline data is owned and arc-length knots validated.
 * Vertex arrays/display lists retain GameCube byte order for the GX backend. */
MeleeScene* melee_scene_decode(const MeleeArchive* archive, uint32_t root);
/* Standalone spline owner; only spline output and free are applicable. */
MeleeScene* melee_scene_decode_spline(const MeleeArchive*,uint32_t,HSD_Spline**);
HSD_JObj* melee_scene_root(MeleeScene* scene);
/* Borrowed descriptors remain valid until melee_scene_free. Instances loaded
 * from them must be removed before freeing the owner. */
HSD_Joint* melee_scene_joint_descriptor(MeleeScene* scene);
HSD_MatAnimJoint* melee_scene_material_descriptor(MeleeScene* scene);
HSD_AnimJoint* melee_scene_animation_descriptor(MeleeScene* scene);
/* Preserve empty shape hierarchies. Actual morph tracks are rejected. */
/* Bind average/additive XYZ morph animation with validated weight channels. */
MeleeHostBool melee_scene_bind_shapes(MeleeScene* scene,uint32_t root);
MeleeHostBool melee_scene_bind_empty_shapes(MeleeScene* scene,uint32_t root);
HSD_ShapeAnimJoint* melee_scene_shape_descriptor(MeleeScene* scene);
/* Bind one ordinary SRT animation tree from the scene-owned archive. */
MeleeHostBool melee_scene_bind_joints(MeleeScene* scene,uint32_t root);
/* For HSD_JObjAddAnim consumers: bind only the root, omit child/next trees. */
MeleeHostBool melee_scene_bind_joint_root(MeleeScene* scene,uint32_t root);
/* Release validation/runtime objects while retaining owned descriptors. After
 * this, only descriptor access and free are valid operations on this owner. */
void melee_scene_release_objects(MeleeScene* scene);
size_t melee_scene_joint_count(const MeleeScene* scene);
/* Binds an owned FigaTree in depth-first joint order. Unsupported channels or
 * mismatched skeletons preserve the previous animation. */
MeleeHostBool melee_scene_bind(MeleeScene* scene,const MeleeArchive* animation,uint32_t root);
/* Attach one costume material tree from the scene-owned archive. Currently
 * accepts material color/alpha/PE and texture transform, blend, LOD,
 * TEV color and image/palette selection tracks.
 * A failed bind leaves the scene unchanged. */
MeleeHostBool melee_scene_bind_materials(MeleeScene* scene,uint32_t root);
MeleeHostBool melee_scene_request(MeleeScene* scene,float frame);
MeleeHostBool melee_scene_step(MeleeScene* scene);
/* Borrow a material image descriptor by source archive offset. */
HSD_ImageDesc* melee_scene_find_image(MeleeScene*,uint32_t);
void melee_scene_free(MeleeScene* scene);
#endif
