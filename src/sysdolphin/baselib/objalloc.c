#include "objalloc.h"

#include <string.h>

#include "initialize.h"
#include "memory.h"
#include <dolphin/os/OSAlloc.h>

static objheap obj_heap = { 0, 0, -1, -1 };

static HSD_ObjAllocData* alloc_datas;

void HSD_ObjSetHeap(u32 size, void* ptr)
{
#ifdef MELEE_NATIVE
    HSD_ASSERT(0, !ptr || (uintptr_t) ptr <= UINTPTR_MAX - size);
    obj_heap.curr = (uintptr_t) ptr;
    obj_heap.top = (uintptr_t) ptr;
#else
    obj_heap.curr = (u32) ptr;
    obj_heap.top = (u32) ptr;
#endif
    obj_heap.remain = size;
    obj_heap.size = size;
}

s32 HSD_ObjAllocAddFree(HSD_ObjAllocData* data, u32 num)
{
#ifdef MELEE_NATIVE
    u8* pool_start;
    u32 pool_size;
    HSD_ASSERT(0xEE, data);
    if (!num || !data->size || num > INT32_MAX ||
        num > UINT32_MAX / data->size || num > UINT32_MAX - data->free) {
        return 0;
    }
    pool_size = data->size * num;
    if (obj_heap.top) {
        uintptr_t end = obj_heap.top + obj_heap.size;
        if (obj_heap.curr > UINTPTR_MAX - data->align) return 0;
        uintptr_t start = (obj_heap.curr + data->align) & ~(uintptr_t) data->align;
        if (start >= end) return 0;
        if (pool_size > end - start) {
            pool_size = (u32) (end - start);
            pool_size -= pool_size % data->size;
        }
        num = pool_size / data->size;
        if (!num) return 0;
        pool_start = (void*) start;
        obj_heap.curr = start + pool_size;
        obj_heap.remain = (u32) (end - obj_heap.curr);
    } else {
        /* The SDK heap guarantees 32 bytes; larger pool alignments need slack.
         * Pools persist until the owning arena is reset, as on GameCube. */
        size_t bytes = pool_size + (data->align > 31 ? (size_t) data->align : 0);
        void* allocation = HSD_MemAlloc(bytes);
        if (!allocation) return 0;
        pool_start = (void*) (((uintptr_t) allocation + data->align) &
                             ~(uintptr_t) data->align);
        obj_heap.remain = bytes < obj_heap.remain ? obj_heap.remain - bytes : 0;
    }
#else
    u32 computed_start;
    u32 pool_end;
    u32 pool_size;
    u8* pool_start;

    u8 _[4];

    HSD_ASSERT(0xEE, data);
    pool_size = data->size * num;
    if (obj_heap.top != 0) {
        pool_end = obj_heap.top + obj_heap.size;
        computed_start = (obj_heap.curr + data->align) & ~data->align;
        pool_start = (void*) computed_start;
        if (computed_start > pool_end) {
            return 0;
        }
        if (pool_end - (u32) pool_start < pool_size) {
            pool_size = pool_end - (u32) pool_start -
                        (pool_end - (u32) pool_start) % data->size;
        }
        num = pool_size / data->size;
        if (num == 0) {
            return 0;
        }
        obj_heap.curr = (u32) pool_start + pool_size;
        obj_heap.remain = pool_end - obj_heap.curr;
    } else {
        pool_start = HSD_MemAlloc(pool_size);
        if (pool_start == 0) {
            return 0;
        }
        obj_heap.remain -= pool_size;
    }

#endif

    {
        int i;
        for (i = 0; (unsigned) i < num - 1; i++) {
            *(void**) (pool_start + data->size * i) =
                (void*) (pool_start + data->size * (i + 1));
        }
        *(void**) (pool_start + data->size * i) = data->freehead;
    }

    data->freehead = (HSD_ObjAllocLink*) pool_start;
    data->free += num;
    return num;
}

void* HSD_ObjAlloc(HSD_ObjAllocData* data)
{
    HSD_ObjAllocLink* cur;
    u32 size;

    if (data->num_limit_flag && data->used >= data->num_limit) {
        return NULL;
    }
    if (data->heap_limit_flag) {
        if (data->heap_limit_num == (unsigned) -1) {
            if (obj_heap.top != 0) {
                size = obj_heap.remain;
            } else {
                size = OSCheckHeap(HSD_GetHeap());
            }
            if (size <= data->heap_limit_size) {
                data->heap_limit_num = data->used + data->free;
            }
        } else {
            if (obj_heap.top != 0) {
                size = obj_heap.remain;
            } else {
                size = OSCheckHeap(HSD_GetHeap());
            }
            if (size > data->heap_limit_size) {
                data->heap_limit_num = -1;
            }
        }
        if (data->used >= data->heap_limit_num) {
            return NULL;
        }
    }
    if (data->free == 0) {
        HSD_ObjAllocAddFree(data, 1);
        if (data->free == 0) {
            return NULL;
        }
    }
    cur = data->freehead;
    data->freehead = cur->next;
    data->used += 1;
    data->free -= 1;
    if (data->used > data->peak) {
        data->peak = data->used;
    }
    return cur;
}

void HSD_ObjFree(HSD_ObjAllocData* data, void* obj)
{
    HSD_ObjAllocLink* link = obj;
    link->next = data->freehead;
    data->freehead = link;
    data->free += 1;
    data->used -= 1;
}

static inline void removeAll(HSD_ObjAllocData* data)
{
    HSD_ObjAllocData** cur = &alloc_datas;
    while (*cur != NULL) {
        if (*cur == data) {
            *cur = (*cur)->next;
        } else {
            cur = &(*cur)->next;
        }
    }
}

void HSD_ObjAllocInit(HSD_ObjAllocData* data, size_t size, u32 align)
{
    HSD_ASSERT(0x185, data);
#ifdef MELEE_NATIVE
    HSD_ASSERT(0, align && !(align & (align - 1)));
    if (align < _Alignof(HSD_ObjAllocLink)) align = _Alignof(HSD_ObjAllocLink);
    if (size < sizeof(HSD_ObjAllocLink)) size = sizeof(HSD_ObjAllocLink);
    HSD_ASSERT(0, size <= UINT32_MAX - (align - 1));
#endif
    if (data != NULL) {
        removeAll(data);
    } else {
        alloc_datas = NULL;
    }
    memset(data, 0, sizeof(HSD_ObjAllocData));
    data->num_limit = -1;
    data->heap_limit_size = 0;
    data->heap_limit_num = -1;
    data->align = align - 1;
    data->size = (size + data->align) & ~data->align;
    data->next = alloc_datas;
    alloc_datas = data;
}

void _HSD_ObjAllocForgetMemory(void* low, void* high)
{
    alloc_datas = NULL;
}
