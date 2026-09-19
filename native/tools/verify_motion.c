#include "melee_animation.h"
#include <stdio.h>
#include <math.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <stdlib.h>
#include <string.h>
#include <sysdolphin/baselib/aobj.h>
static uint32_t be32(const uint8_t* p)
{ return ((uint32_t)p[0]<<24) | ((uint32_t)p[1]<<16) | ((uint32_t)p[2]<<8) | p[3]; }
static size_t samples;
static int invalid_value;
static void sample(void* object, int type, HSD_ObjData* value)
{
    (void)object; (void)type;
    ++samples;
    if (!isfinite(value->fv)) invalid_value = 1;
}
static void interpret(MeleeFighterAnimation* animation)
{
    FigaTree* tree = melee_fighter_animation_tree(animation);
    FigaTrack* t = tree->tracks;
    for (size_t joint = 0; joint < melee_fighter_animation_joint_count(animation); ++joint) {
        for (int i = 0; i < tree->nodes[joint]; ++i, ++t) {
            HSD_FObjDesc desc = {.length = t->length, .startframe = (s16)t->startframe,
                .type = t->obj_type, .frac_value = t->frac_value, .frac_slope = t->frac_slope, .ad = t->ad_head};
            HSD_FObj* track = HSD_FObjLoadDesc(&desc);
            HSD_FObjReqAnimAll(track, 0);
            HSD_FObjInterpretAnim(track, NULL, sample, 0);
            HSD_FObjInterpretAnim(track, NULL, sample, tree->frames / 2);
            HSD_FObjInterpretAnim(track, NULL, sample, tree->frames / 2);
            HSD_FObjRemoveAll(track);
        }
    }
}
static void timeline(MeleeFighterAnimation* animation)
{
    FigaTree* tree = melee_fighter_animation_tree(animation);
    /* Bound this exhaustive verification workload before converting to integer. */
    if (!isfinite(tree->frames) || tree->frames < 0 || tree->frames > 100000) {
        fprintf(stderr, "Timeline verification supports at most 100000 frames per motion\n");
        invalid_value = 1;
        return;
    }
    FigaTrack* tracks = tree->tracks;
    for (size_t joint = 0; joint < melee_fighter_animation_joint_count(animation); ++joint) {
        s8 count = tree->nodes[joint];
        HSD_AObj* controller = lbAnim_LoadAObj(tree, tracks, count);
        tracks += count;
        if (!count) continue;
        if (!controller) { invalid_value = 1; continue; }
        HSD_AObjReqAnim(controller, 0);
        /* Run through the end and one additional frame (including loop wrap). */
        for (unsigned frame = 0; frame <= (unsigned)ceilf(tree->frames) + 1; ++frame) {
            HSD_AObjInitEndCallBack();
            HSD_AObjInterpretAnim(controller, NULL, sample);
            HSD_AObjInvokeCallBacks();
            if (!isfinite(controller->curr_frame)) invalid_value = 1;
        }
        /* lbAnim_LoadAObj creates a track-only AObj, without hsd_obj ownership. */
        HSD_AObjSetFObj(controller, NULL);
        HSD_AObjFree(controller);
    }
}
int main(int argc, char** argv)
{
    int run_timeline = argc == 3 && !strcmp(argv[2], "--timeline");
    if (argc != 2 && !run_timeline) return 2;
    static _Alignas(32) uint8_t arena[65536];
    if (!OSInitAlloc(arena, arena + sizeof(arena), 1)) return 1;
    HSD_SetHeap(OSCreateHeap(arena, arena + sizeof(arena)));
    HSD_FObjInitAllocData();
    HSD_AObjInitAllocData();
    FILE* f = fopen(argv[1], "rb"); if (!f) return 1;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return 1; }
    long length = ftell(f); rewind(f);
    uint8_t* bytes = length > 0 ? malloc((size_t)length) : NULL;
    if (!bytes || fread(bytes, 1, length, f) != (size_t)length) { free(bytes); fclose(f); return 1; }
    fclose(f);
    size_t cursor = 0, joints = 0, accepted = 0, rejected = 0;
    while (cursor < (size_t)length) {
        if ((size_t)length - cursor < 32) { ++rejected; break; }
        uint32_t size = be32(bytes + cursor), root;
        const char* name;
        MeleeArchive archive;
        if (size > (size_t)length - cursor || !melee_archive_open(&archive, bytes + cursor, size) ||
            !melee_archive_public(&archive, 0, &name, &root)) { ++rejected; break; }
        MeleeFighterAnimation* animation = melee_fighter_animation_decode(&archive, root);
        if (!animation) { printf("Rejected motion: %s at %zu\n", name, cursor); ++rejected; }
        else { ++accepted; joints += melee_fighter_animation_joint_count(animation); if (run_timeline) timeline(animation); else interpret(animation); }
        melee_fighter_animation_free(animation);
        if (size == (size_t)length - cursor) break;
        cursor += ((size_t)size + 31) & ~(size_t)31;
    }
    printf("Fighter motion decode: %zu accepted, %zu joints, %zu rejected\n", accepted, joints, rejected);
    printf("Original interpreter: %zu callbacks, nonfinite=%d, outstanding objects=%u\n", samples, invalid_value, HSD_FObjGetAllocData()->used);
    printf("Original animation controllers outstanding: %u\n", HSD_AObjGetAllocData()->used);
    free(bytes); return rejected || invalid_value || HSD_FObjGetAllocData()->used || HSD_AObjGetAllocData()->used ? 1 : 0;
}
