#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <assert.h>
#include <stdio.h>
_Static_assert(sizeof(HSD_IDKey) == sizeof(void*), "Native ID keys must hold descriptor pointers");
_Static_assert(sizeof(((HSD_JObj*)0)->id) == sizeof(void*), "Joint identity must retain pointer width");
_Static_assert(sizeof(((HSD_AObjDesc*)0)->obj_id) == sizeof(void*), "Animation object references must retain pointer width");
int main(void)
{
    static _Alignas(32) unsigned char arena[65536];
    assert(OSInitAlloc(arena, arena + sizeof(arena), 1));
    HSD_SetHeap(OSCreateHeap(arena, arena + sizeof(arena)));
    HSD_IDInitAllocData(); HSD_IDSetup();
    int a = 1, b = 2, c = 3, found;
    HSD_IDKey first = UINT64_C(0x100001234);
    HSD_IDKey second = first + (UINT64_C(101) << 32);
    HSD_IDKey third = second + (UINT64_C(101) << 32);
    assert((u32)first == (u32)second && first % 101 == second % 101);
    HSD_IDInsertToTable(NULL, first, &a);
    HSD_IDInsertToTable(NULL, second, &b);
    HSD_IDInsertToTable(NULL, third, &c);
    assert(HSD_IDGetData(first, &found) == &a && found == 1);
    assert(HSD_IDGetData(second, NULL) == &b && HSD_IDGetData(third, NULL) == &c);
    assert(HSD_IDGetAllocData()->used == 3);
    HSD_IDInsertToTable(NULL, second, &a);
    assert(HSD_IDGetAllocData()->used == 3 && HSD_IDGetData(second, NULL) == &a);
    HSD_IDRemoveByIDFromTable(NULL, second); /* middle */
    assert(!HSD_IDGetData(second, &found) && found == 0);
    HSD_IDRemoveByIDFromTable(NULL, first); /* tail */
    assert(HSD_IDGetData(third, NULL) == &c);
    HSD_IDRemoveByIDFromTable(NULL, third); /* head */
    HSD_IDRemoveByIDFromTable(NULL, third); /* missing removal is harmless */
    assert(HSD_IDGetAllocData()->used == 0);
    HSD_IDInsertToTable(NULL, 0, NULL);
    assert(!HSD_IDGetData(0, &found) && found == 1);
    HSD_IDRemoveByIDFromTable(NULL, 0);
    HSD_IDTable private_table = {0};
    HSD_Joint descriptor = {0}; HSD_JObj object = {0};
    HSD_IDKey pointer = (HSD_IDKey)&descriptor;
    assert(pointer > UINT32_MAX);
    object.id = pointer;
    HSD_AObjDesc animation = {.obj_id = pointer};
    HSD_IDInsertToTable(&private_table, pointer, &object);
    assert(!HSD_IDGetData(pointer, &found) && !found);
    assert(HSD_IDGetDataFromTable(&private_table, animation.obj_id, &found) == &object && found);
    HSD_IDRemoveByIDFromTable(&private_table, object.id);
    assert(HSD_IDGetAllocData()->used == 0);
    puts("Original HSD ID tables preserve native descriptor identities and collision chains");
}
