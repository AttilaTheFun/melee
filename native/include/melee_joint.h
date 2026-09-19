#ifndef MELEE_NATIVE_JOINT_H
#define MELEE_NATIVE_JOINT_H
#include "melee_archive.h"

/* Decoded joint records. Payload and constraint offsets deliberately remain
 * archive references until their own schemas are decoded. UINT32_MAX is null;
 * offset zero is a valid relocated reference. No disk bytes are host pointers. */
typedef struct MeleeJointNode {
    uint32_t offset, flags, payload_offset, constraints_offset;
    struct MeleeJointNode *child, *next;
    char *class_name;
    float rotation[3], scale[3], position[3];
    MeleeHostBool has_inverse_bind;
    float inverse_bind[3][4];
} MeleeJointNode;
typedef struct MeleeJointGraph MeleeJointGraph;
MeleeJointGraph* melee_joint_decode(const MeleeArchive* archive, uint32_t root);
const MeleeJointNode* melee_joint_root(const MeleeJointGraph* graph);
const MeleeJointNode* melee_joint_node(const MeleeJointGraph* graph, size_t index);
size_t melee_joint_count(const MeleeJointGraph* graph);
void melee_joint_free(MeleeJointGraph* graph);
#endif
