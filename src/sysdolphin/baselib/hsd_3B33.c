#include "hsd_3B33.h"

#include <setjmp.h> // IWYU pragma: keep
#include <string.h>
#ifdef MELEE_NATIVE
#include <stdint.h>
#endif

#include "hsd_3A94.h"

void hsd_803B3344(u8 byte)
{
    u8* temp_r5;

    temp_r5 = hsd_804D79A0;
#ifdef MELEE_NATIVE
    uintptr_t base = (uintptr_t) hsd_804D79A4;
    uintptr_t cursor = (uintptr_t) temp_r5;
    if (hsd_804D79A4 && hsd_804D79A8 > 0 && cursor >= base &&
        cursor - base < (size_t) hsd_804D79A8) {
#else
    if ((u32) temp_r5 < (u32) hsd_804D79A4 + (u32) hsd_804D79A8) {
#endif
        hsd_804D79A0 = temp_r5 + 1;
        *temp_r5 = byte;
        return;
    }

    HSD_LONGJMP(&hsd_804D2648, true);
}

void hsd_803B3398(void* src, size_t size)
{
    void* temp_r3 = hsd_804D79A0;

#ifdef MELEE_NATIVE
    uintptr_t base = (uintptr_t) hsd_804D79A4;
    uintptr_t cursor = (uintptr_t) temp_r3;
    size_t used = cursor >= base ? cursor - base : SIZE_MAX;
    /* Preserve the original block writer's reserved final byte. */
    if (hsd_804D79A4 && hsd_804D79A8 > 0 && used < (size_t) hsd_804D79A8 &&
        size < (size_t) hsd_804D79A8 - used && (src || !size)) {
#else
    if ((u32) temp_r3 < (u32) hsd_804D79A4 + (u32) hsd_804D79A8 - size) {
#endif
#ifdef MELEE_NATIVE
        if (size) memcpy(temp_r3, src, size);
        hsd_804D79A0 += size;
#else
        memcpy(temp_r3, src, size);
        *((u32*) &hsd_804D79A0) += size;
#endif
        return;
    }

    HSD_LONGJMP(&hsd_804D2648, true);
}
