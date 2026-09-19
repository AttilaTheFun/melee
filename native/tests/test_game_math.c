/* Link and execute original Melee code. The linker removes unused routines
 * that still depend on the unported engine; no engine stubs are supplied. */
#include <melee/lb/lbvector.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <placeholder.h>

static void close_to(float actual, float expected, float tolerance)
{
    if (!(fabsf(actual - expected) < tolerance))
        fprintf(stderr, "Expected %.9g, got %.9g (tolerance %.9g)\n", expected, actual, tolerance);
    assert(fabsf(actual - expected) < tolerance);
}

int main(void)
{
    Vec3 v = {3, 4, 0};
    close_to(lbVector_Normalize(&v), 5, 0.000001f);
    close_to(v.x, 0.6f, 0.000001f); close_to(v.y, 0.8f, 0.000001f);
    Vec3 zero = {0};
    assert(lbVector_Normalize(&zero) == 0 && zero.x == 0 && zero.y == 0);
    Vec3 a = {2, 4, 6}, b = {6, 8, 10}, result;
    assert(lbVector_Lerp(&a, &b, &result, 0.25f) == &result);
    assert(result.x == 3 && result.y == 5 && result.z == 7);
    Vec3 x = {1, 0, 0}, y = {0, 1, 0};
    close_to(lbVector_Angle(&x, &y), M_PI_2, 0.000001f);
    lbVector_Rotate(&x, 4, M_PI_2);
    /* The original quintic sine approximation has about 0.016 absolute
     * error at pi (used to approximate cos(pi/2)). Preserve that behavior. */
    close_to(x.x, 0, 0.02f); close_to(x.y, 1, 0.02f);
    close_to((float)__frsqrte(4.0), 0.5f, 0.000001f);
    puts("Original Melee vector normalization, interpolation, angle and rotation run on ARM64.");
    return 0;
}
