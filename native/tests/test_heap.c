#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static _Alignas(32) unsigned char arena[65536];
static _Alignas(128) unsigned char pool[512];
static void* exercise(void* context)
{
    int h = *(int*)context;
    for (int i = 0; i < 2000; ++i) {
        unsigned char* p = OSAllocFromHeap(h, 97);
        assert(p && !((uintptr_t)p & 31));
        memset(p, i, 97);
        OSFreeToHeap(h, p);
    }
    return NULL;
}
int main(void)
{
    assert(!OSInitAlloc(arena, arena + 1, 2));
    assert(OSInitAlloc(arena + 1, arena + sizeof(arena), 2) == arena + 32);
    int h = OSCreateHeap(arena + 32, arena + 32768);
    int other = OSCreateHeap(arena + 32768, arena + sizeof(arena));
    assert(h == 0 && other == 1);
    assert(OSCreateHeap(arena + 64, arena + 128) == -1);
    assert(OSCheckHeap(-1) == -1);
    long capacity = OSCheckHeap(h);
    assert(capacity == 32704);
    assert(OSSetCurrentHeap(h) == -1);
    HSD_SetHeap(h);
    assert(HSD_GetHeap() == h);
    assert(!HSD_MemAlloc(0) && !HSD_MemAlloc(-1));
    assert(!OSAllocFromHeap(h, ULONG_MAX));
    void* a = HSD_MemAlloc(1);
    void* b = OSAlloc(64);
    void* c = OSAlloc(96);
    assert(a && b && c && OSReferentSize(a) == 32);
    HSD_Free(a);
    OSFree(c);
    OSFree(b);
    assert(OSCheckHeap(h) == capacity);
    a = OSAllocFromHeap(h, capacity);
    assert(a && !OSAllocFromHeap(h, 1) && OSCheckHeap(h) == 0);
    OSFreeToHeap(h, a);
    assert(OSCheckHeap(h) == capacity);
    pthread_t workers[4];
    for (int i = 0; i < 4; ++i) assert(!pthread_create(&workers[i], NULL, exercise, &h));
    for (int i = 0; i < 4; ++i) assert(!pthread_join(workers[i], NULL));
    assert(OSCheckHeap(h) == capacity);
    assert(OSCheckHeap(other) == 32736);

    HSD_ObjAllocData small = {0}, aligned = {0};
    HSD_ObjSetHeap(sizeof(pool) - 1, pool + 1);
    HSD_ObjAllocInit(&small, 1, 1);
    assert(small.size >= sizeof(void*) && small.align >= _Alignof(void*) - 1);
    assert(!HSD_ObjAllocAddFree(&small, 0));
    assert(!HSD_ObjAllocAddFree(&small, UINT32_MAX));
    assert(HSD_ObjAllocAddFree(&small, 3) == 3);
    a = HSD_ObjAlloc(&small); b = HSD_ObjAlloc(&small);
    assert((uintptr_t)a > UINT32_MAX && !((uintptr_t)a & 7));
    assert(a != b && small.used == 2 && small.free == 1 && small.peak == 2);
    HSD_ObjFree(&small, a);
    assert(HSD_ObjAlloc(&small) == a);
    HSD_ObjAllocSetNumLimit(&small, 2);
    HSD_ObjAllocEnableNumLimit(&small);
    assert(!HSD_ObjAlloc(&small));
    HSD_ObjAllocInit(&aligned, 33, 128);
    assert(HSD_ObjAllocAddFree(&aligned, 100) == 3);
    for (int i = 0; i < 3; ++i) {
        void* p = HSD_ObjAlloc(&aligned);
        assert(p && !((uintptr_t)p & 127));
        memset(p, 0xa5, 33);
    }
    assert(!HSD_ObjAlloc(&aligned));
    _HSD_ObjAllocForgetMemory(NULL, NULL);
    HSD_ObjSetHeap(UINT32_MAX, NULL);
    HSD_ObjAllocInit(&aligned, 33, 128);
    assert(HSD_ObjAllocAddFree(&aligned, 3) == 3);
    for (int i = 0; i < 3; ++i) assert(!((uintptr_t)HSD_ObjAlloc(&aligned) & 127));
    HSD_AObjInitAllocData();
    HSD_AObj* animation = HSD_AObjAlloc();
    assert(animation->flags == AOBJ_NO_ANIM && animation->framerate == 1.0f);
    assert(!animation->fobj && !animation->hsd_obj && animation->curr_frame == 0);
    animation->curr_frame = 30;
    HSD_AObjFree(animation);
    assert(HSD_AObjAlloc() == animation && animation->curr_frame == 0);
    HSD_AObjFree(animation);
    HSD_AObjFree(NULL);
    assert(HSD_AObjGetAllocData()->used == 0);
    assert(OSCheckHeap(h) > 0);
    OSDestroyHeap(h);
    assert(OSCheckHeap(h) == -1 && HSD_GetHeap() == -1 && __OSCurrHeap == -1);
    assert(OSCreateHeap(arena + 32, arena + 32768) == h);
    assert(OSCheckHeap(h) == capacity);
    puts("Native heap and original HSD object allocator tests passed");
}
