/* Native entry points for the SDK's paired-single math interface. Reuse the
 * original scalar C implementations; do not execute PowerPC assembly.
 * Mathematical behavior is preserved, but PPC estimate/rounding bits still
 * require differential validation against the original executable. */
#include <dolphin/mtx.h>
#include <string.h>

void PSMTXIdentity(Mtx m) { C_MTXIdentity(m); }
void PSMTXCopy(Mtx a, Mtx b) { C_MTXCopy(a, b); }
void PSMTXConcat(Mtx a, Mtx b, Mtx out) { C_MTXConcat(a, b, out); }
void PSMTXTranspose(Mtx a, Mtx b) { C_MTXTranspose(a, b); }
u32 PSMTXInverse(Mtx a, Mtx b) { return C_MTXInverse(a, b); }
u32 PSMTXInvXpose(Mtx a, Mtx b) { return C_MTXInvXpose(a, b); }
void PSMTXRotTrig(Mtx m, char axis, f32 s, f32 c) { C_MTXRotTrig(m, axis, s, c); }
void PSMTXRotAxisRad(Mtx m, Vec* axis, f32 angle) { C_MTXRotAxisRad(m, axis, angle); }
void PSMTXScale(Mtx m, f32 x, f32 y, f32 z) { C_MTXScale(m, x, y, z); }
void PSMTXQuat(Mtx m, Quaternion* q) { C_MTXQuat(m, q); }
void PSMTXTrans(Mtx m, f32 x, f32 y, f32 z)
{
    C_MTXIdentity(m);
    m[0][3] = x; m[1][3] = y; m[2][3] = z;
}
void PSVECAdd(Vec* a, Vec* b, Vec* out) { C_VECAdd(a, b, out); }
void PSVECSubtract(Vec* a, Vec* b, Vec* out) { C_VECSubtract(a, b, out); }
void PSVECScale(Vec* a, Vec* out, f32 scale) { C_VECScale(a, out, scale); }
void PSVECNormalize(Vec* a, Vec* out) { C_VECNormalize(a, out); }
f32 PSVECMag(Vec* a) { return C_VECMag(a); }
f32 PSVECSquareMag(Vec* a) { return C_VECSquareMag(a); }
f32 PSVECDotProduct(Vec* a, Vec* b) { return C_VECDotProduct(a, b); }
void PSVECCrossProduct(Vec* a, Vec* b, Vec* out) { C_VECCrossProduct(a, b, out); }
f32 PSVECSquareDistance(Vec* a, Vec* b) { return C_VECSquareDistance(a, b); }
void PSMTXMultVec(Mtx44 m, Vec* a, Vec* out) { C_MTXMultVec(m, a, out); }
void PSMTXMultVecSR(Mtx44 m, Vec* a, Vec* out) { C_MTXMultVecSR(m, a, out); }
void PSMTXMultVecArray(Mtx m, Vec* a, Vec* out, u32 n) { C_MTXMultVecArray(m, a, out, n); }

void PSMTXReorder(Mtx src, ROMtx dst)
{
    ROMtx tmp;
    for (unsigned column = 0; column < 4; ++column)
        for (unsigned row = 0; row < 3; ++row) tmp[column][row] = src[row][column];
    memcpy(dst, tmp, sizeof(tmp));
}

static Vec ro_transform(ROMtx m, Vec v)
{
    Vec out;
    out.x = ((m[0][0]*v.x + m[3][0]) + m[1][0]*v.y) + m[2][0]*v.z;
    out.y = ((m[0][1]*v.x + m[3][1]) + m[1][1]*v.y) + m[2][1]*v.z;
    out.z = ((m[0][2]*v.x + m[3][2]) + m[1][2]*v.y) + m[2][2]*v.z;
    return out;
}

void PSMTXROMultVecArray(ROMtx* m, Vec* src, Vec* dst, u32 count)
{
    for (u32 i = 0; i < count; ++i) dst[i] = ro_transform(*m, src[i]);
}

void PSMTXROSkin2VecArray(ROMtx* m0, ROMtx* m1, f32* weights,
                         Vec* src, Vec* dst, u32 count)
{
    for (u32 i = 0; i < count; ++i) {
        ROMtx blend;
        /* Match the source's m0 + weight * (m1 - m0), including translation.
         * Preserve weights outside [0,1]; the SDK does not clamp them. */
        for (unsigned c = 0; c < 4; ++c)
            for (unsigned r = 0; r < 3; ++r)
                blend[c][r] = (*m0)[c][r] + weights[i]*((*m1)[c][r] - (*m0)[c][r]);
        dst[i] = ro_transform(blend, src[i]);
    }
}

void PSMTXROMultS16VecArray(ROMtx* m, S16Vec* src, Vec* dst, u32 count)
{
    /* The original routine sets GQR6 to signed-16, scale zero. The asset
     * importer must have converted the components to host-endian s16. */
    for (u32 i = 0; i < count; ++i)
        dst[i] = ro_transform(*m, (Vec){src[i].x, src[i].y, src[i].z});
}

void PSMTXMultS16VecArray(Mtx44* m, S16Vec* src, Vec* dst, u32 count)
{
    for (u32 i = 0; i < count; ++i) {
        Vec v = {src[i].x, src[i].y, src[i].z};
        C_MTXMultVec(*m, &v, &dst[i]);
    }
}
