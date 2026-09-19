#include <sysdolphin/baselib/robj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    static _Alignas(32) u8 arena[1024 * 1024];
    assert(OSInitAlloc(arena, arena + sizeof(arena), 1));
    HSD_SetHeap(OSCreateHeap(arena, arena + sizeof(arena)));
    HSD_RObjInitAllocData();
    HSD_Joint joints[3] = {0};
    HSD_RvalueList values[] = {{1, &joints[0]}, {2, &joints[1]}, {4, &joints[2]}, {0, NULL}};
    u8 code[] = {2, 0, 0, 1};
    HSD_ByteCodeExpDesc expression = {code, values};
    HSD_RObjDesc desc = {0};
    desc.flags = REFTYPE_BYTECODE | 1;
    desc.u.bcexp = &expression;
    for (int pass = 0; pass < 20; ++pass) {
        HSD_RObj* object = HSD_RObjLoadDesc(&desc);
        assert(object && object->u.exp.is_bytecode && object->u.exp.nb_args == UINT32_MAX);
        HSD_Rvalue* value = object->u.exp.rvalue;
        for (int i = 0; i < 3; ++i) {
            assert(value && value->flags == (1u << i));
            value = value->next;
        }
        assert(!value);
        HSD_RObjRemoveAll(object);
        assert(!HSD_RObjGetAllocData()->used && !HSD_RvalueObjGetAllocData()->used);
    }
    puts("Optimized game expression lists: three arguments, repeated ownership and cleanup passed");
}
