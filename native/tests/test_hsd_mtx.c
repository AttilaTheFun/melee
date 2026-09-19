#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <assert.h>
#include <stdio.h>
static void near(float a, float b) { assert(fabsf(a - b) < 0.0001f); }
static void equal(Mtx a, Mtx b)
{ for (int i = 0; i < 3; ++i) for (int j = 0; j < 4; ++j) near(a[i][j], b[i][j]); }
int main(void)
{
    static _Alignas(32) u8 arena[65536];
    assert(OSInitAlloc(arena, arena + sizeof(arena), 1));
    HSD_SetHeap(OSCreateHeap(arena, arena + sizeof(arena)));
    HSD_VecInitAllocData(); HSD_MtxInitAllocData();
    Vec3* allocated = HSD_VecAlloc(); MtxPtr storage = HSD_MtxAlloc();
    assert(allocated && storage);
    *allocated = (Vec3){2, 3, 4};
    Vec3 scale = {2, 3, 4}, rotation = {0, 0, 1.57079632679f}, position = {5, 6, 7};
    Mtx euler, quaternion, inverse, identity, expected;
    Quaternion q = {0, 0, 0.70710678118f, 0.70710678118f};
    HSD_MtxSRT(euler, &scale, &rotation, &position, NULL);
    HSD_MtxSRTQuat(quaternion, &scale, &q, &position, NULL);
    equal(euler, quaternion);
    Vec3 point = {1, 0, 0}, transformed;
    PSMTXMultVec(euler, &point, &transformed);
    near(transformed.x, 5); near(transformed.y, 8); near(transformed.z, 7);
    HSD_MtxInverse(euler, inverse); PSMTXConcat(inverse, euler, identity);
    PSMTXIdentity(expected); equal(identity, expected);
    HSD_MtxInverseConcat(euler, euler, identity); equal(identity, expected);
    PSMTXCopy(euler, storage); HSD_MtxInverseConcat(storage, euler, storage); equal(storage, expected);
    HSD_MtxGetTranslate(euler, &transformed);
    near(transformed.x, 5); near(transformed.y, 6); near(transformed.z, 7);
    HSD_MtxGetScale(euler, &transformed);
    near(transformed.x, 2); near(transformed.y, 3); near(transformed.z, 4);
    Vec3 parent_scale = {2, 4, 8};
    HSD_MtxSRT(euler, &scale, &rotation, &position, &parent_scale);
    HSD_MtxSRTQuat(quaternion, &scale, &q, &position, &parent_scale);
    equal(euler, quaternion);
    /* Parent scale compensation: inverse(parent scale) * R * parent scale * S. */
    near(euler[0][1], -6); near(euler[1][0], 1);
    near(euler[2][2], 4); near(euler[0][3], 5);
    for (int i = 0; i < 20; ++i) {
        rotation = (Vec3){i * 0.07f, i * -0.035f, i * 0.11f};
        HSD_MtxSRT(euler, &scale, &rotation, &position, NULL);
        HSD_MtxGetRotation(euler, &transformed);
        HSD_MtxSRT(quaternion, &scale, &transformed, &position, NULL);
        equal(euler, quaternion);
        HSD_MtxInverseTranspose(euler, inverse);
        PSMTXInvXpose(euler, expected); equal(inverse, expected);
    }
    Mtx zero = {0}; HSD_MtxInverse(zero, inverse); PSMTXIdentity(expected); equal(inverse, expected);
    HSD_MtxInverseTranspose(zero, inverse); equal(inverse, zero);
    HSD_MtxInverseConcat(zero, euler, inverse); equal(inverse, euler);
    HSD_MtxScaledAdd(euler, quaternion, storage, 0.5f);
    for (int i = 0; i < 3; ++i) for (int j = 0; j < 4; ++j) near(storage[i][j], quaternion[i][j] + .5f*euler[i][j]);
    u32 bits = 0xffc12345; float value; memcpy(&value, &bits, 4);
    value = fabsf_bitwise(value); memcpy(&bits, &value, 4); assert(bits == 0x7fc12345);
    assert(!signbit(fabsf_bitwise(-0.0f)));
    HSD_VecFree(allocated); HSD_MtxFree(storage);
    assert(!HSD_VecGetAllocData()->used && !HSD_MtxGetAllocData()->used);
    puts("Original HSD transforms, parent scale, quaternion, inverse and pools passed");
}
