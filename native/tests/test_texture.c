#include "melee_texture.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void pixel(const uint8_t* p, unsigned r, unsigned g, unsigned b, unsigned a)
{ assert(p[0] == r && p[1] == g && p[2] == b && p[3] == a); }
int main(void)
{
    uint8_t data[256] = {0}, rgba[1024], palette[64] = {0};
    assert(melee_texture_level_size(9, 9, 0) == 128);
    assert(melee_texture_level_size(5, 5, 6) == 256);
    assert(!melee_texture_level_size(0, 8, 0));
    assert(!melee_texture_level_size(8, 8, 7));
    data[0] = 0x3f;
    assert(melee_texture_decode(data, 32, 2, 1, 0, NULL, 0, 0, rgba, sizeof(rgba)));
    pixel(rgba, 51, 51, 51, 51); pixel(rgba+4, 255, 255, 255, 255);
    data[0] = 47;
    assert(melee_texture_decode(data, 32, 1, 1, 1, NULL, 0, 0, rgba, sizeof(rgba)));
    pixel(rgba, 47, 47, 47, 47);
    data[0] = 0x35;
    assert(melee_texture_decode(data, 32, 1, 1, 2, NULL, 0, 0, rgba, sizeof(rgba)));
    pixel(rgba, 85, 85, 85, 51);
    data[0] = 79; data[1] = 123;
    assert(melee_texture_decode(data, 32, 1, 1, 3, NULL, 0, 0, rgba, sizeof(rgba)));
    pixel(rgba, 123, 123, 123, 79);
    data[0] = 0xf8; data[1] = 0;
    assert(melee_texture_decode(data, 32, 1, 1, 4, NULL, 0, 0, rgba, sizeof(rgba)));
    pixel(rgba, 255, 0, 0, 255);
    data[0] = 0x3f; data[1] = 0x08;
    assert(melee_texture_decode(data, 32, 1, 1, 5, NULL, 0, 0, rgba, sizeof(rgba)));
    pixel(rgba, 255, 0, 136, 109);
    data[0] = 0x83; data[1] = 0xe0;
    assert(melee_texture_decode(data, 32, 1, 1, 5, NULL, 0, 0, rgba, sizeof(rgba)));
    pixel(rgba, 0, 255, 0, 255);
    data[0] = 7; data[1] = 11; data[32] = 13; data[33] = 17;
    assert(melee_texture_decode(data, 64, 1, 1, 6, NULL, 0, 0, rgba, sizeof(rgba)));
    pixel(rgba, 11, 13, 17, 7);
    memset(data, 0, sizeof(data));
    data[0] = 0x10; palette[2] = 0xf8;
    assert(melee_texture_decode(data, 32, 1, 1, 8, palette, 4, 1, rgba, sizeof(rgba)));
    pixel(rgba, 255, 0, 0, 255);
    data[0] = 1; palette[2] = 37; palette[3] = 83;
    assert(melee_texture_decode(data, 32, 1, 1, 9, palette, 4, 0, rgba, sizeof(rgba)));
    pixel(rgba, 83, 83, 83, 37);
    data[0] = 0xc0; data[1] = 1; palette[2] = 0x83; palette[3] = 0xe0;
    assert(melee_texture_decode(data, 32, 1, 1, 10, palette, 4, 2, rgba, sizeof(rgba)));
    pixel(rgba, 0, 255, 0, 255);
    assert(!melee_texture_decode(data, 32, 1, 1, 10, palette, 2, 2, rgba, sizeof(rgba)));
    memset(data, 0, sizeof(data)); data[0] = 0xf8; data[3] = 0x1f; data[4] = 0x1b;
    assert(melee_texture_decode(data, 32, 8, 8, 14, NULL, 0, 0, rgba, sizeof(rgba)));
    pixel(rgba, 255, 0, 0, 255); pixel(rgba+4, 0, 0, 255, 255);
    pixel(rgba+8, 159, 0, 95, 255); pixel(rgba+12, 95, 0, 159, 255);
    data[0] = 0; data[1] = 0x1f; data[2] = 0xf8; data[3] = 0;
    assert(melee_texture_decode(data, 32, 8, 8, 14, NULL, 0, 0, rgba, sizeof(rgba)));
    pixel(rgba+12, 127, 0, 127, 0);
    memset(data, 0, sizeof(data));
    data[0] = 0xf8; data[8] = 0x07; data[9] = 0xe0;
    data[17] = 0x1f; data[24] = 0xff; data[25] = 0xff;
    assert(melee_texture_decode(data, 32, 8, 8, 14, NULL, 0, 0, rgba, sizeof(rgba)));
    pixel(rgba, 255, 0, 0, 255); pixel(rgba+4*4, 0, 255, 0, 255);
    pixel(rgba+4*8*4, 0, 0, 255, 255); pixel(rgba+(4*8+4)*4, 255, 255, 255, 255);
    memset(data, 0x11, 32); memset(data+32, 0x22, 32);
    memset(data+64, 0x33, 32); memset(data+96, 0x44, 32);
    memset(rgba, 0xa5, sizeof(rgba));
    assert(melee_texture_decode(data, 128, 9, 9, 0, NULL, 0, 0, rgba, sizeof(rgba)));
    pixel(rgba,17,17,17,17); pixel(rgba+8*4,34,34,34,34);
    pixel(rgba+8*9*4,51,51,51,51); pixel(rgba+(8*9+8)*4,68,68,68,68);
    assert(rgba[9*9*4] == 0xa5);
    assert(!melee_texture_decode(data, 127, 9, 9, 0, NULL, 0, 0, rgba, sizeof(rgba)));
    assert(!melee_texture_decode(data, 128, 9, 9, 0, NULL, 0, 0, rgba, 9*9*4-1));
    puts("GX tiled texture formats, palettes, CMPR colors/alpha and clipped tiles passed");
}
