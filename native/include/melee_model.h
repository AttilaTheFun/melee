#ifndef MELEE_NATIVE_MODEL_H
#define MELEE_NATIVE_MODEL_H
#include "melee_archive.h"
#include "melee_vertex.h"
#include "melee_material.h"
typedef struct MeleeModel MeleeModel;
typedef struct MeleeFighterAnimation MeleeFighterAnimation;
typedef struct {
    uint32_t joint_offset, drawable_offset, polygon_offset, material_offset;
    size_t drawable_index;
    const MeleeMaterial* material;
    uint16_t polygon_flags;
    MeleeHostBool hidden;
    size_t vertex_count, draw_count;
    const MeleeVertex* vertices;
    const MeleeDraw* draws;
} MeleeModelPart;
/* Requires initialized HSD heap/AObj/FObj/Vec pools. Owns decoded geometry,
 * joints, skin bindings, materials and texture pixels; archive bytes are not
 * borrowed. Parts borrow material and geometry storage from this model. */
MeleeModel* melee_model_decode(const MeleeArchive* archive, uint32_t root);
void melee_model_free(MeleeModel* model);
size_t melee_model_part_count(const MeleeModel* model);
const MeleeModelPart* melee_model_part(const MeleeModel* model, size_t index);
size_t melee_model_drawable_count(const MeleeModel* model);
/* Fighter preorder indices include empty drawables. This visibility is combined
 * with animated joint visibility and survives subsequent pose updates. */
MeleeHostBool melee_model_set_drawable_hidden(MeleeModel*,size_t index,MeleeHostBool hidden);
/* Animation storage is borrowed until another binding or model destruction. */
MeleeHostBool melee_model_bind(MeleeModel* model, MeleeFighterAnimation* animation);
MeleeHostBool melee_model_request(MeleeModel* model, float frame);
MeleeHostBool melee_model_step(MeleeModel* model);
#endif
