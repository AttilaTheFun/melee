#ifndef MELEE_NATIVE_SKIN_H
#define MELEE_NATIVE_SKIN_H
#include "melee_pose.h"
#include "melee_vertex.h"
typedef struct MeleeSkin MeleeSkin;
/* Decode one PObj's rigid/shared or envelope binding. Borrows the pose, which
 * must outlive the binding; archive bytes may be released after decoding.
 * Shape animation and camera-facing billboard matrices are not supported yet. */
MeleeSkin* melee_skin_decode(const MeleeArchive* archive, uint32_t polygon,
                            const MeleePose* pose, size_t owner_joint);
void melee_skin_free(MeleeSkin* skin);
size_t melee_skin_palette_count(const MeleeSkin* skin);
MeleeHostBool melee_skin_matrices(const MeleeSkin* skin, Mtx positions[10], Mtx normals[10]);
/* World-space CPU skinning, retaining UVs/colors/draw order. Output may alias
 * the input vertices, but must hold at least melee_vertex_count(batch) entries.
 * A false return leaves output unspecified. */
MeleeHostBool melee_skin_apply(const MeleeSkin* skin, const MeleeVertexBatch* batch,
                              MeleeVertex* output, size_t capacity);
#endif
