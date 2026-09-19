#ifndef MELEE_ARAM_H
#define MELEE_ARAM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Read currently committed ARAM bytes for native sample decoding. No alignment
 * is required. Does not wait for pending DMA; each DMA chunk and this read are
 * serialized. Failure leaves the caller's destination unchanged. */
bool melee_aram_read(uint32_t offset, void* destination, size_t bytes);
#ifdef __cplusplus
}
#endif
#endif
