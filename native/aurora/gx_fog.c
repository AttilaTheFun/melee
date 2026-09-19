/* GXInitFogAdjTable from the original SDK GXPixel.c, without its hardware
 * state assertions. Keep the game's float arithmetic and 12-bit table values.
 * This function computes data only; Aurora consumes it in GXSetFogRangeAdj. */
#include <dolphin/gx.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void invalid_fog(void)
{
    fputs("GXInitFogAdjTable: invalid projection or width\n", stderr);
    abort();
}

void GXInitFogAdjTable(GXFogAdjTable* table, u16 width, f32 projection[4][4])
{
    if (!table || !projection || !width || width > 640) invalid_fog();
    f32 near_z;
    f32 side_x;
    if (projection[3][3] == 0.0f) {
        near_z = projection[2][3] / (projection[2][2] - 1.0f);
        side_x = (near_z * (1.0f + projection[0][2])) / projection[0][0];
    } else {
        near_z = (1.0f + projection[2][3]) / projection[2][2];
        side_x = -(projection[0][3] - 1.0f) / projection[0][0];
    }
    if (!isfinite(near_z) || !isfinite(side_x) || near_z == 0) invalid_fog();
    f32 inverse_width = 2.0f / width;
    for (u32 i = 0; i < 10; ++i) {
        f32 x = (i + 1) << 5;
        x *= inverse_width;
        x *= side_x;
        f32 range = sqrtf(1.0f + (x * x) / (near_z * near_z));
        f32 scaled = 256.0f * range;
        if (!isfinite(scaled) || scaled < 0 || scaled >= 0x1p32f) invalid_fog();
        table->r[i] = (u32) scaled & 0xFFF;
    }
}
