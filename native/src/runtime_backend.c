/* Host implementation of the runtime helper explicitly called by the game.
 * Match src/Runtime/runtime.c's PowerPC __cvt_dbl_usll, including its signed
 * saturation despite the unsigned result type. Do not execute undefined host
 * floating-to-integer conversions for infinities, NaNs, or overflow. */
#include <Runtime/runtime.h>
#include <stdint.h>
#include <string.h>

u64 __cvt_dbl_usll(double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    unsigned exponent = (unsigned) ((bits >> 52) & 0x7FF);
    unsigned negative = (unsigned) (bits >> 63);
    if (exponent < 1023) {
        return 0;
    }
    if (exponent >= 1086) {
        return negative ? UINT64_C(0x8000000000000000)
                        : UINT64_C(0x7FFFFFFFFFFFFFFF);
    }
    uint64_t magnitude = (bits & UINT64_C(0x000FFFFFFFFFFFFF)) |
                         UINT64_C(0x0010000000000000);
    if (exponent < 1075) {
        magnitude >>= 1075 - exponent;
    } else {
        magnitude <<= exponent - 1075;
    }
    return negative ? UINT64_C(0) - magnitude : magnitude;
}
