#include <melee/gm/types.h>
#include <assert.h>
#include <stdio.h>
int main(void)
{
    _Static_assert(sizeof(UnkFlagStruct) == 1, "flag byte size");
    for (unsigned value = 0; value < 256; ++value) {
        UnkFlagStruct flags = {.u8 = value};
        assert(flags.b0 == ((value >> 7) & 1));
        assert(flags.b1 == ((value >> 6) & 1));
        assert(flags.b2 == ((value >> 5) & 1));
        assert(flags.b3 == ((value >> 4) & 1));
        assert(flags.b4 == ((value >> 3) & 1));
        assert(flags.b5 == ((value >> 2) & 1));
        assert(flags.b6 == ((value >> 1) & 1));
        assert(flags.b7 == (value & 1));
        flags.b7 = 1;
        assert(flags.u8 == (value | 1));
        flags.b0 = 0;
        assert(flags.u8 == ((value | 1) & 0x7f));
    }
    puts("PowerPC flag-byte layout: all 256 values and masked writes passed");
}
