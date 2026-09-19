#include <melee/ft/types.h>
#include <assert.h>
#include <stdio.h>
int main(void)
{
    Fighter fighter = {0};
    uint32_t word = 0xc0000001;
    for(unsigned i=0;i<10000;++i){
        fighter.x594_s32 = word;
        assert(fighter.x597_bits == (word & 63));
        assert(fighter.x594_bits == ((word >> 9) & 8191));
        assert(fighter.x596_bits.x7 == ((word >> 6) & 7));
        assert(fighter.x596_bits.x0 == ((word >> 9) & 127));
        assert(fighter.x594_b0 == ((word >> 31) & 1));
        assert(fighter.x594_b1_loop == ((word >> 30) & 1));
        assert(fighter.x594_b2 == ((word >> 29) & 1));
        assert(fighter.x594_b3 == ((word >> 28) & 1));
        assert(fighter.x594_b4 == ((word >> 27) & 1));
        assert(fighter.x594_b5 == ((word >> 26) & 1));
        assert(fighter.x594_b6 == ((word >> 25) & 1));
        assert(fighter.x594_b7 == ((word >> 24) & 1));
        fighter.x597_bits = 8;
        assert((uint32_t)fighter.x594_s32 == ((word & ~63u) | 8));
        fighter.x594_s32 = word;
        fighter.x596_bits.x7 = 3;
        assert((uint32_t)fighter.x594_s32 == ((word & ~(7u<<6)) | (3u<<6)));
        word = word * 1664525u + 1013904223u;
    }
    puts("Animation flag word: 10,000 overlapping bitfield reads and masked writes passed");
}
