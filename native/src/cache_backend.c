/* CPU copies back native DVD/ARAM transfers. Host RAM is coherent, so these
 * cache-maintenance entry points need ordering, not GameCube cache commands.
 * They do not implement GPU resource synchronization or locked-cache DMA. */
#include <dolphin/os/OSCache.h>
#include <stdatomic.h>
void DCInvalidateRange(void* address, u32 size)
{ (void)address; (void)size; atomic_thread_fence(memory_order_seq_cst); }
void DCStoreRange(void* address, u32 size)
{ (void)address; (void)size; atomic_thread_fence(memory_order_seq_cst); }
void DCFlushRange(void* address, u32 size)
{ (void)address; (void)size; atomic_thread_fence(memory_order_seq_cst); }
void DCFlushRangeNoSync(void* address, u32 size)
{ (void)address; (void)size; atomic_thread_fence(memory_order_release); }
