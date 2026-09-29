#include "melee_disc.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void put32(uint8_t* p, uint32_t v)
{ p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
static uint8_t image[16384], header[32768];
static MeleeHostBool memory_read(void* context, void* output, size_t size, uint64_t offset)
{
    if (offset > sizeof(image) || size > sizeof(image) - offset) return false;
    memcpy(output, (const uint8_t*)context + offset, size); return true;
}
static void check(MeleeDisc* d)
{
    assert(d && melee_disc_entry_count(d) == 4);
    assert(!memcmp(melee_disc_id(d), "GALE01\0\2", 8));
    assert(melee_disc_find(d, 0, "/anim/MOVE.DAT") == 2);
    assert(melee_disc_find(d, 1, "../Zero.dat") == 3);
    assert(melee_disc_find(d, 1, "./Move.dat") == 2);
    assert(melee_disc_find(d, 0, "../../") == 0);
    assert(melee_disc_find(d, 0, "Move.dat") == -1);
    assert(melee_disc_find(d, 0, "anim/Move.dat/anything") == -1);
    assert(melee_disc_find(d, 0, "anim/Move.dat/") == -1);
    uint8_t out[80];
    assert(melee_disc_read(d, 2, out, 64, 0));
    assert(!memcmp(out, image + 0x2ff0, 64));
    assert(!melee_disc_read(d, 2, out, 72, 0));
    assert(!melee_disc_read(d, 1, out, 1, 0));
    memset(out, 0xff, sizeof(out));
    assert(melee_disc_read_at(d, out, 80, 4090));
    assert(!memcmp(out, image + 4090, 80));
    assert(!melee_disc_read_at(d, out, 80, UINT64_MAX));
    melee_disc_close(d);
}
int main(void)
{
    memcpy(image, "GALE01\0\2", 8); put32(image + 0x1c, 0xc2339f3d);
    put32(image + 0x424, 0x800); put32(image + 0x428, 80);
    uint8_t* fst = image + 0x800;
    put32(fst, 0x01000000); put32(fst + 8, 4);
    put32(fst + 12, 0x01000000); put32(fst + 20, 3);
    put32(fst + 24, 5); put32(fst + 28, 0x2ff0); put32(fst + 32, 40);
    put32(fst + 36, 14); put32(fst + 40, 0x900); put32(fst + 44, 5);
    memcpy(fst + 48, "anim\0Move.dat\0Zero.dat", 23);
    for (int i = 0; i < 64; ++i) image[0x2ff0 + i] = (uint8_t)i;
    char path[] = "/tmp/melee-disc-XXXXXX";
    int fd = mkstemp(path); assert(fd >= 0);
    assert(write(fd, image, sizeof(image)) == sizeof(image));
    check(melee_disc_open_reader(sizeof(image), memory_read, image));
    check(melee_disc_open(path));
    memcpy(header, "CISO", 4); header[5] = 16; /* 4096, little-endian */
    header[8] = header[10] = header[11] = 1;
    assert(!ftruncate(fd, 0)); assert(lseek(fd, 0, SEEK_SET) == 0);
    assert(write(fd, header, sizeof(header)) == sizeof(header));
    assert(write(fd, image, 4096) == 4096);
    assert(write(fd, image + 8192, 8192) == 8192);
    check(melee_disc_open(path));
    /* Reject malformed maps, zero block size and truncated stored blocks. */
    uint8_t invalid = 2, valid = 0;
    assert(pwrite(fd, &invalid, 1, 9) == 1); assert(!melee_disc_open(path));
    assert(pwrite(fd, &valid, 1, 9) == 1);
    assert(pwrite(fd, &valid, 1, 5) == 1); assert(!melee_disc_open(path));
    invalid = 16; assert(pwrite(fd, &invalid, 1, 5) == 1);
    assert(!ftruncate(fd, 32768 + 12287)); assert(!melee_disc_open(path));
    assert(!ftruncate(fd, 0)); assert(lseek(fd, 0, SEEK_SET) == 0);
    put32(fst + 20, 5); /* child directory exceeds its parent */
    assert(write(fd, image, sizeof(image)) == sizeof(image));
    assert(!melee_disc_open(path));
    close(fd); unlink(path);
    puts("ISO/CISO filesystem, path lookup, sparse blocks and padded reads passed");
}
