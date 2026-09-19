#include <Runtime/runtime.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern double fn_801855BC(double);
extern float MSL_TrigF_80400770[];
extern float MSL_TrigF_80400774[];

static double from_bits(uint64_t bits)
{
    double value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

int main(void)
{
    uint32_t bits;
    memcpy(&bits, MSL_TrigF_80400770, sizeof(bits));
    assert(bits == UINT32_C(0x7FFFFFFF));
    memcpy(&bits, MSL_TrigF_80400774, sizeof(bits));
    assert(bits == UINT32_C(0x7F800000));
    assert(isnan(MSL_TrigF_80400770[0]));
    assert(isinf(MSL_TrigF_80400774[0]));
    assert(isnan(fn_801855BC(-1.0)));
    assert(fn_801855BC(0.0) == 0.0);
    assert(fabs(fn_801855BC(9.0) - 3.0) < 1e-12);
    assert(__cvt_dbl_usll(0.0) == 0 && __cvt_dbl_usll(-0.0) == 0);
    assert(__cvt_dbl_usll(0.999) == 0 && __cvt_dbl_usll(-0.999) == 0);
    assert(__cvt_dbl_usll(123.875) == 123);
    assert(__cvt_dbl_usll(-123.875) == (uint64_t)-INT64_C(123));
    assert(__cvt_dbl_usll(0x1p63) == INT64_MAX);
    assert(__cvt_dbl_usll(-0x1p63) == UINT64_C(0x8000000000000000));
    assert(__cvt_dbl_usll(INFINITY) == INT64_MAX);
    assert(__cvt_dbl_usll(-INFINITY) == UINT64_C(0x8000000000000000));
    assert(__cvt_dbl_usll(from_bits(UINT64_C(0x7FF8000000000001))) == INT64_MAX);
    assert(__cvt_dbl_usll(from_bits(UINT64_C(0xFFF8000000000001))) == UINT64_C(0x8000000000000000));
    assert(__cvt_dbl_usll(from_bits(1)) == 0);
    /* Independent native cast oracle only where conversion is defined. */
    uint64_t state = 912381;
    unsigned checked = 0;
    for (unsigned i = 0; i < 100000; ++i) {
        state = state * UINT64_C(6364136223846793005) + 1;
        double value = from_bits(state);
        if (isfinite(value) && value >= -0x1p63 && value < 0x1p63) {
            assert(__cvt_dbl_usll(value) == (uint64_t)(int64_t)value);
            ++checked;
        }
    }
    assert(checked > 40000);
    puts("Native runtime: original NaN/infinity bits and PowerPC conversion rounding/saturation passed");
    return 0;
}
