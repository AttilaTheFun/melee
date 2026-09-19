#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct { HSD_Obj parent; unsigned char payload[2000]; } LargeObject;
static void large_info_init(void), failed_info_init(void);
static HSD_ClassInfo large_class = {large_info_init};
static HSD_ClassInfo failed_class = {failed_info_init};
static int released;
static int large_init(HSD_Class* object)
{
    LargeObject* p = (LargeObject*)object;
    for (size_t i = 0; i < sizeof(p->payload); ++i) assert(p->payload[i] == 0);
    p->payload[0] = 42; return 0;
}
static void large_release(HSD_Class* object)
{ assert(((LargeObject*)object)->payload[0] == 42); ++released; }
static void large_info_init(void)
{
    hsdInitClassInfo(&large_class, &hsdObj, "native_test", "large", sizeof(large_class), sizeof(LargeObject));
    large_class.init = large_init; large_class.release = large_release;
}
static int failed_init(HSD_Class* object) { (void)object; return -1; }
static void failed_info_init(void)
{
    hsdInitClassInfo(&failed_class, &large_class, "native_test", "failed", sizeof(failed_class), sizeof(LargeObject));
    failed_class.init = failed_init;
}
int main(void)
{
    static _Alignas(32) unsigned char arena[4 * 1024 * 1024];
    /* Poison backing bytes so incompletely cleared pointer tables fail. */
    memset(arena, 0xa5, sizeof(arena));
    assert(OSInitAlloc(arena, arena + sizeof(arena), 1));
    HSD_SetHeap(OSCreateHeap(arena, arena + sizeof(arena)));
    HSD_Obj* base = hsdNew(&hsdObj);
    assert(base && base->parent.class_info == &hsdObj);
    assert(hsdObj.head.nb_exist == 1 && hsdObjIsDescendantOf(base, &hsdClass));
    LargeObject* first = hsdNew(&large_class);
    assert(first && first->payload[0] == 42);
    assert(large_class.head.obj_size == sizeof(*first));
    assert(hsdIsDescendantOf(&large_class, &hsdObj));
    assert(!hsdIsDescendantOf(&hsdObj, &large_class));
    assert(large_class.destroy == hsdClass.destroy);
    assert(large_class.head.nb_exist == 1 && large_class.head.nb_peak == 1);
    hsdDelete(first); assert(released == 1 && large_class.head.nb_exist == 0);
    LargeObject* reused = hsdNew(&large_class);
    assert(reused == first && reused->payload[0] == 42);
    assert(!hsdNew(&failed_class)); assert(failed_class.head.nb_exist == 0);
    assert(failed_class.head.nb_peak == 1 && released == 1);
    hsdDelete(reused); hsdDelete(base); hsdDelete(NULL);
    assert(released == 2 && !large_class.head.nb_exist && !hsdObj.head.nb_exist);
    /* Grow the allocation table again while preserving earlier size classes. */
    void* large_piece = hsdAllocMemPiece(8000);
    assert(large_piece); memset(large_piece, 0x7c, 8000);
    hsdFreeMemPiece(large_piece, 8000);
    assert(hsdAllocMemPiece(8000) == large_piece);
    hsdFreeMemPiece(large_piece, 8000);
    void* small_piece = hsdAllocMemPiece(64);
    assert(small_piece); hsdFreeMemPiece(small_piece, 64);
    assert(GetMemoryEntry(1)->nb_free > 0);
    assert(GetMemoryEntry((8000 + 31) / 32 - 1)->nb_free == 1);
    assert(OSCheckHeap(HSD_GetHeap()) > 0);
    puts("Original HSD classes: pointer-table growth, inheritance, initialization, reuse and deletion passed");
}
