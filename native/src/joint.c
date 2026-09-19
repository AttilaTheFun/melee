#include "melee_joint.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct MeleeJointGraph {
    MeleeJointNode* nodes;
    size_t count, capacity;
};
static MeleeHostBool reference(const MeleeArchive* a, uint32_t slot, uint32_t* target)
{
    MeleeHostBool present;
    if (!melee_archive_pointer(a, slot, target, &present)) return false;
    if (!present) *target = UINT32_MAX;
    return true;
}

static MeleeHostBool range(const MeleeArchive* a, uint32_t at, size_t size)
{ return at <= a->data_size && size <= a->data_size - at; }
static MeleeHostBool floats(const MeleeArchive* a, uint32_t at, float* out, unsigned n)
{
    if (!range(a, at, n * 4)) return false;
    for (unsigned i = 0; i < n; ++i)
        if (!melee_archive_f32(a, at + 4*i, out + i) || !isfinite(out[i])) return false;
    return true;
}
static size_t find(const MeleeJointGraph* g, uint32_t offset)
{
    for (size_t i = 0; i < g->count; ++i) if (g->nodes[i].offset == offset) return i;
    return SIZE_MAX;
}
static MeleeHostBool add(MeleeJointGraph* g, const MeleeArchive* a, uint32_t offset)
{
    if (offset == UINT32_MAX || find(g, offset) != SIZE_MAX) return true;
    if ((offset & 3) || !range(a, offset, 64)) return false;
    if (g->count == g->capacity) {
        size_t capacity = g->capacity ? g->capacity * 2 : 32;
        if (capacity < g->capacity || capacity > SIZE_MAX / sizeof(*g->nodes)) return false;
        MeleeJointNode* nodes = realloc(g->nodes, capacity * sizeof(*nodes));
        if (!nodes) return false;
        g->nodes = nodes; g->capacity = capacity;
    }
    g->nodes[g->count++] = (MeleeJointNode){.offset = offset};
    return true;
}
void melee_joint_free(MeleeJointGraph* g)
{
    if (!g) return;
    for (size_t i = 0; i < g->count; ++i) free(g->nodes[i].class_name);
    free(g->nodes); free(g);
}
const MeleeJointNode* melee_joint_root(const MeleeJointGraph* g)
{ return g && g->count ? g->nodes : NULL; }
const MeleeJointNode* melee_joint_node(const MeleeJointGraph* g, size_t i)
{ return g && i < g->count ? g->nodes + i : NULL; }
size_t melee_joint_count(const MeleeJointGraph* g) { return g ? g->count : 0; }

/* Explicit stack: malformed or very deep graphs cannot exhaust the C stack.
 * Shared instance references are preserved, but directed cycles are rejected. */
static MeleeHostBool acyclic(const MeleeJointGraph* g)
{
    typedef struct { size_t index; unsigned edge; } Frame;
    if (g->count > SIZE_MAX / sizeof(Frame)) return false;
    Frame* stack = malloc(g->count * sizeof(*stack));
    uint8_t* state = calloc(g->count, 1);
    if (!stack || !state) { free(stack); free(state); return false; }
    size_t depth = 1; stack[0] = (Frame){0, 0}; state[0] = 1;
    MeleeHostBool okay = true;
    while (depth) {
        Frame* f = stack + depth - 1;
        if (f->edge == 2) { state[f->index] = 2; --depth; continue; }
        const MeleeJointNode* n = g->nodes + f->index;
        const MeleeJointNode* target = f->edge++ == 0 ? n->child : n->next;
        if (!target) continue;
        size_t i = (size_t)(target - g->nodes);
        if (state[i] == 1) { okay = false; break; }
        if (!state[i]) { state[i] = 1; stack[depth++] = (Frame){i, 0}; }
    }
    free(stack); free(state); return okay;
}
MeleeJointGraph* melee_joint_decode(const MeleeArchive* a, uint32_t root)
{
    if (!a || !a->bytes || root == UINT32_MAX) return NULL;
    MeleeJointGraph* g = calloc(1, sizeof(*g));
    if (!g) return NULL;
    if (!add(g, a, root)) goto fail;
    for (size_t i = 0; i < g->count; ++i) {
        uint32_t at = g->nodes[i].offset, child, next, name, matrix;
        MeleeJointNode* n = g->nodes + i;
        if (!melee_archive_u32(a, at + 4, &n->flags) ||
            !reference(a, at, &name) || !reference(a, at + 8, &child) ||
            !reference(a, at + 12, &next) || !reference(a, at + 16, &n->payload_offset) ||
            !reference(a, at + 56, &matrix) || !reference(a, at + 60, &n->constraints_offset) ||
            !floats(a, at + 20, n->rotation, 3) || !floats(a, at + 32, n->scale, 3) ||
            !floats(a, at + 44, n->position, 3)) goto fail;
        if (name != UINT32_MAX) {
            if (!range(a, name, 1)) goto fail;
            const char* text = (const char*)a->bytes + 32 + name;
            const char* end = memchr(text, 0, a->data_size - name);
            if (!end) goto fail;
            size_t size = (size_t)(end - text) + 1;
            n->class_name = malloc(size);
            if (!n->class_name) goto fail;
            memcpy(n->class_name, text, size);
        }
        if (matrix != UINT32_MAX) {
            for (unsigned row = 0; row < 3; ++row)
                if (!range(a, matrix, 48) || !floats(a, matrix + row*16, n->inverse_bind[row], 4)) goto fail;
            n->has_inverse_bind = true;
        }
        if (!add(g, a, child) || !add(g, a, next)) goto fail;
    }
    /* Resolve only after the array has stopped moving. */
    for (size_t i = 0; i < g->count; ++i) {
        uint32_t child, next, at = g->nodes[i].offset;
        if (!reference(a, at + 8, &child) || !reference(a, at + 12, &next)) goto fail;
        if (child != UINT32_MAX) g->nodes[i].child = g->nodes + find(g, child);
        if (next != UINT32_MAX) g->nodes[i].next = g->nodes + find(g, next);
    }
    if (!acyclic(g)) goto fail;
    return g;
fail:
    melee_joint_free(g); return NULL;
}
