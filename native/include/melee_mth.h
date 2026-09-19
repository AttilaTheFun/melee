#ifndef MELEE_NATIVE_MTH_H
#define MELEE_NATIVE_MTH_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t version, buffer_size, width, height, frame_rate, frame_count;
    uint32_t first_frame, frame_offsets, first_frame_size;
} MeleeMTHHeader;
/* Parse a 64-byte big-endian MTHP header and validate its first frame against
 * the enclosing file. Output is unchanged on failure. Frame chains require
 * separate validation as each packed frame arrives. */
int melee_mth_header(const void* bytes, size_t length, size_t file_size,
                     MeleeMTHHeader* output);
#ifdef __cplusplus
}
#endif
#endif
