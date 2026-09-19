#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

extern HSD_ObjAllocData gobj_alloc_data, gobjproc_alloc_data;
static int log_entries[128], log_count, object_removed, user_removed;
static int payload;
static HSD_GObj* victim;
static void record(HSD_GObj* g)
{
    assert(HSD_GObj_CurrentInvokedProcGObj == g);
    assert(HSD_GObj_CurrentInvokedProc->gobj == g);
    assert(log_count < 128);
    log_entries[log_count++] = g->classifier;
}
static void expect(const int* entries, int count)
{
    assert(log_count == count);
    assert(!memcmp(log_entries, entries, count * sizeof(int)));
    log_count = 0;
    assert(!HSD_GObj_CurrentInvokedProcGObj && !HSD_GObj_CurrentInvokedProc);
    assert(!HSD_GObj_DelayedProcInfo.flags);
}
#define EXPECT(...) do { const int e[] = {__VA_ARGS__}; expect(e, sizeof(e)/sizeof(*e)); } while (0)
static void destroy_object(HSD_Obj* p) { assert((void*)p == &payload); object_removed++; }
static void destroy_user(void* p) { assert(p == &payload); user_removed++; }
static void self_remove(HSD_GObj* g)
{
    record(g);
    HSD_GObjFree(g);
    assert(HSD_GObj_DelayedProcInfo.delay_remove_gobj);
    assert(g->user_data == &payload && !user_removed && !object_removed);
}
static void remove_proc(HSD_GObj* g)
{
    record(g);
    HSD_GObjProc_RemoveProc(HSD_GObj_CurrentInvokedProc);
}
static void remove_next(HSD_GObj* g)
{
    record(g);
    HSD_GObjFree(victim);
    victim = NULL;
    HSD_GObjProc_RemoveProc(HSD_GObj_CurrentInvokedProc);
}
static void change_priority(HSD_GObj* g)
{
    record(g);
    if (g->p_link == 1) HSD_GObjPLink_ChangeGObjPri_Unk(0, g, 63, 0, NULL);
}
static HSD_GObj* spawned;
static void insert_proc(HSD_GObj* g)
{
    record(g);
    spawned = GObj_Create(13, 0, 1);
    assert(spawned);
    HSD_GObj_SetupProc(spawned, record, 0);
    HSD_GObjProc_RemoveProc(HSD_GObj_CurrentInvokedProc);
}
static void skip_later_procs(HSD_GObj* g)
{
    record(g);
    HSD_GObj_80390CD4(g);
    HSD_GObjProc_RemoveProc(HSD_GObj_CurrentInvokedProc);
}
static void render(HSD_GObj* g, int pass)
{
    assert(HSD_GObj_804D7814 == g);
    log_entries[log_count++] = g->classifier * 10 + pass;
}
static HSD_GObj* create(int id, int link, int priority, HSD_GObjEvent callback, int proc_priority)
{
    HSD_GObj* g = GObj_Create(id, link, priority);
    assert(g);
    if (callback) assert(HSD_GObj_SetupProc(g, callback, proc_priority));
    return g;
}
int main(void)
{
    static _Alignas(32) unsigned char arena[1024 * 1024];
    assert(OSInitAlloc(arena, arena + sizeof(arena), 1));
    HSD_SetHeap(OSCreateHeap(arena, arena + sizeof(arena)));
    u64 disabled = 0;
    GObjFunc destructors[] = {destroy_object};
    GObjFuncs handlers = {NULL, 1, destructors};
    HSD_GObjLibInitDataType data;
    HSD_GObj_803912E0(&data);
    data.funcs = &handlers; data.unk_2 = &disabled;
    HSD_GObjInitWithHandlers(&data);
    assert(HSD_GObj_JObjKind == HSD_GOBJ_OBJ_NONE);
    HSD_GObj *a = create(1, 2, 20, record, 0);
    HSD_GObj *b = create(2, 2, 10, record, 0);
    HSD_GObj *c = create(3, 1, 0, record, 1);
    HSD_GObj *d = create(4, 63, 0, record, 0);
    HSD_GObj_80390CFC(); EXPECT(2, 1, 4, 3);
    disabled = 1ULL << 63;
    HSD_GObj_80390C5C(b);
    HSD_GObj_80390CFC(); EXPECT(1, 3);
    disabled = 0; HSD_GObj_80390C84(b);
    c->proc->flags_2 = 1;
    HSD_GObj_80390CFC(); EXPECT(2, 1, 4);
    HSD_GObj_80390CAC(c);
    HSD_GObj_80390CFC(); EXPECT(2, 1, 4, 3);
    HSD_GObjFree(a); HSD_GObjFree(b); HSD_GObjFree(c); HSD_GObjFree(d);

    a = create(5, 0, 0, self_remove, 0);
    HSD_GObj_SetupProc(a, record, 1); /* Must not run after owner deletion. */
    HSD_GObjObject_80390A70(a, 0, &payload);
    GObj_InitUserData(a, 0, destroy_user, &payload);
    b = create(6, 0, 1, remove_proc, 0);
    HSD_GObj_80390CFC(); EXPECT(5, 6);
    assert(object_removed == 1 && user_removed == 1 && !b->proc);
    HSD_GObjFree(b);
    assert(!gobj_alloc_data.used && !gobjproc_alloc_data.used);

    a = create(7, 0, 0, remove_next, 0);
    victim = create(8, 0, 1, record, 0);
    b = create(9, 0, 2, record, 0);
    HSD_GObj_80390CFC(); EXPECT(7, 9);
    assert(!victim);
    HSD_GObjFree(a); HSD_GObjFree(b);

    a = create(10, 1, 0, change_priority, 0);
    b = create(11, 2, 0, record, 0);
    HSD_GObj_80390CFC(); EXPECT(10, 11); /* Moved callback runs once this frame. */
    assert(a->p_link == 63);
    HSD_GObj_80390CFC(); EXPECT(11, 10);
    HSD_GObjFree(a); HSD_GObjFree(b);

    a = create(12, 0, 0, insert_proc, 0);
    b = create(14, 0, 2, record, 0);
    HSD_GObj_80390CFC(); EXPECT(12, 13, 14);
    HSD_GObj_80390CFC(); EXPECT(13, 14);
    HSD_GObjFree(a); HSD_GObjFree(spawned); HSD_GObjFree(b);

    a = create(15, 0, 0, skip_later_procs, 0);
    HSD_GObj_SetupProc(a, record, 1);
    HSD_GObj_80390CFC(); EXPECT(15);
    HSD_GObj_80390CFC(); EXPECT(15);
    HSD_GObjFree(a);

    a = create(1, 0, 0, NULL, 0); b = create(2, 0, 0, NULL, 0);
    c = create(3, 0, 0, NULL, 0);
    GObj_SetupGXLink(a, render, 63, 20);
    GObj_SetupGXLink(b, render, 63, 10);
    c->gxlink_prios = 1ULL << 63;
    HSD_GObj_80390ED0(c, 5); EXPECT(20, 10, 22, 12);
    assert(!HSD_GObj_804D7814);
    HSD_GObjFree(b);
    HSD_GObj_80390ED0(c, 1); EXPECT(10);
    HSD_GObjFree(a); HSD_GObjFree(c);
    for (int i = 0; i < 64; i++) {
        assert(!((HSD_GObj**)HSD_GObj_Entities)[i] && !plinklow_gobjs[i]);
        assert(!HSD_GObjGXLinkHead[i] && !HSD_GObj_804D7820[i]);
    }
    assert(!gobj_alloc_data.used && !gobjproc_alloc_data.used);
    puts("Original GObj scheduler: ordering, masks, pause, callback deletion, priority changes, rendering dispatch and cleanup passed");
}
