#include <dolphin/mtx.h>
#include <melee/lb/lbvector.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void near(float a, float b) { assert(fabsf(a - b) < 0.00002f); }
static void identity(Mtx m)
{
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 4; ++c) near(m[r][c], r == c ? 1 : 0);
}

int main(void)
{
    Mtx a, b, c, inverse;
    PSMTXIdentity(a); identity(a);
    PSMTXScale(a, 2, 3, 4); PSMTXTrans(b, 5, -6, 7);
    PSMTXConcat(b, a, c);
    Vec v = {1, 2, 3};
    PSMTXMultVec(c, &v, &v);
    near(v.x, 7); near(v.y, 0); near(v.z, 19);
    assert(PSMTXInverse(c, inverse)); PSMTXMultVec(inverse, &v, &v);
    near(v.x, 1); near(v.y, 2); near(v.z, 3);
    PSMTXConcat(c, inverse, c); identity(c); /* output aliases input */
    PSMTXConcat(b, a, b); assert(PSMTXInverse(b, b)); /* in-place inversion */
    PSMTXScale(a, 0, 1, 1); memset(c, 0x55, sizeof(c));
    Mtx saved; memcpy(saved, c, sizeof(c));
    assert(!PSMTXInverse(a, c)); assert(!memcmp(c, saved, sizeof(c)));

    PSMTXScale(a, 2, 3, 4); assert(PSMTXInvXpose(a, c));
    near(c[0][0], 0.5f); near(c[1][1], 1.0f/3); near(c[2][2], 0.25f);
    MTXRotRad(a, 'z', (float)M_PI_2);
    v = (Vec){1, 0, 0}; PSMTXMultVecSR(a, &v, &v);
    near(v.x, 0); near(v.y, 1); near(v.z, 0);
    Vec axis = {0, 0, 2}; PSMTXRotAxisRad(a, &axis, (float)M_PI_2);
    v = (Vec){1, 0, 0}; PSMTXMultVec(a, &v, &v); near(v.y, 1);
    Quaternion q = {0, 0, 0, 1}; PSMTXQuat(a, &q); identity(a);
    Vec eye = {0, 0, 10}, up = {0, 1, 0}, target = {0, 0, 0};
    C_MTXLookAt(a, &eye, &up, &target); PSMTXMultVec(a, &eye, &v);
    near(v.x, 0); near(v.y, 0); near(v.z, 0);
    PSMTXMultVec(a, &target, &v); near(v.z, -10);

    Vec x = {1, 0, 0}, y = {0, 1, 0};
    PSVECCrossProduct(&x, &y, &x); near(x.z, 1); near(PSVECDotProduct(&x, &y), 0);
    v = (Vec){3, 4, 0}; PSVECNormalize(&v, &v); near(PSVECMag(&v), 1);
    /* Exercise an original game routine which previously needed a PPC SDK symbol. */
    x = (Vec){2, 0, 0}; y = (Vec){0, 3, 0};
    lbVector_CrossprodNormalized(&x, &y, &v); near(v.z, 1);
    Vec array[] = {{1, 2, 3}, {-1, 0, 4}};
    PSMTXTrans(a, 3, 4, 5); PSMTXMultVecArray(a, array, array, 2);
    near(array[0].x, 4); near(array[1].z, 9);
    PSMTXMultVecArray(a, NULL, NULL, 0);
    ROMtx ro0, ro1;
    PSMTXReorder(a, ro0);
    Vec packed[] = {{1, 2, 3}, {-1, 0, 4}, {0, 0, 0}};
    PSMTXROMultVecArray(&ro0, packed, packed, 3);
    near(packed[0].x, 4); near(packed[1].z, 9); near(packed[2].y, 4);
    PSMTXTrans(b, 7, 8, 9); PSMTXReorder(b, ro1);
    float weights[] = {0, 0.5f, 1};
    Vec skinned[] = {{1, 0, 0}, {1, 0, 0}, {1, 0, 0}};
    PSMTXROSkin2VecArray(&ro0, &ro1, weights, skinned, skinned, 3);
    near(skinned[0].x, 4); near(skinned[1].x, 6); near(skinned[2].x, 8);
    near(skinned[1].z, 7);
    S16Vec integers[] = {{-32768, 32767, -1}};
    PSMTXROMultS16VecArray(&ro0, integers, packed, 1);
    near(packed[0].x, -32765); near(packed[0].y, 32771); near(packed[0].z, 4);
    Mtx44 wide = {{1, 0, 0, 3}, {0, 1, 0, 4}, {0, 0, 1, 5}, {0, 0, 0, 1}};
    PSMTXMultS16VecArray(&wide, integers, packed, 1);
    near(packed[0].x, -32765); near(packed[0].y, 32771);
    PSMTXROMultVecArray(NULL, NULL, NULL, 0);
    PSMTXROSkin2VecArray(NULL, NULL, NULL, NULL, NULL, 0);
    PSMTXROMultS16VecArray(NULL, NULL, NULL, 0);
    PSMTXMultS16VecArray(NULL, NULL, NULL, 0);

    /* The SDK projection has four rows despite old decompiler Mtx annotations.
     * GameCube depth maps the near plane to -1 and the far plane to 0. */
    Mtx44 projection;
    MTXPerspective(projection, 90, 1, 1, 100);
    near(projection[0][0], 1); near(projection[1][1], 1);
    near((projection[2][2]*-1 + projection[2][3])/1, -1);
    near((projection[2][2]*-100 + projection[2][3])/100, 0);
    near(projection[3][2], -1);
    MTXOrtho(projection, 1, -1, -1, 1, 1, 100);
    near(projection[2][2]*-1 + projection[2][3], -1);
    near(projection[2][2]*-100 + projection[2][3], 0);
    puts("Native SDK transforms, aliasing, inverses, camera, projection and original game vector integration passed.");
    return 0;
}
