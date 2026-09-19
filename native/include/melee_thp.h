#ifndef MELEE_NATIVE_THP_H
#define MELEE_NATIVE_THP_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint16_t width, height;
    size_t y_bytes, uv_bytes;
} MeleeTHPInfo;
/* Bounded Melee JPEG/THP decoder. Outputs use GX I8 8x4 tiling.
 * Returns zero on success. Outputs remain unchanged on failure.
 * Input and the three output buffers must not overlap. */
int melee_thp_info(const void* file, size_t size, MeleeTHPInfo* out);
int melee_thp_decode(const void* file, size_t size,
                     void* y, size_t y_size, void* u, size_t u_size,
                     void* v, size_t v_size);
#ifdef __cplusplus
}
#endif
#endif
