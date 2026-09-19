#include "melee_disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be32(const unsigned char* p)
{ return ((unsigned)p[0]<<24) | ((unsigned)p[1]<<16) | ((unsigned)p[2]<<8) | p[3]; }
int main(int argc, char** argv)
{
    if (argc < 2 || argc > 3) { fprintf(stderr, "Usage: inspect-disc IMAGE [FILE_PATH|--main-dol]\n"); return 2; }
    MeleeDisc* disc = melee_disc_open(argv[1]);
    if (!disc) { fprintf(stderr, "Invalid or unreadable GameCube disc image\n"); return 1; }
    int result = 0;
    if (argc == 2) {
        const unsigned char* id = melee_disc_id(disc);
        printf("Disc %.6s, revision %u, %u filesystem entries\n", id, id[7], melee_disc_entry_count(disc));
        for (uint32_t i = 0; i < melee_disc_entry_count(disc); ++i) {
            const MeleeDiscEntry* e = melee_disc_entry(disc, i);
            printf("%5u parent=%5u %c %10u %s\n", i, e->parent, e->directory ? 'd' : 'f', e->length, e->name);
        }
    } else {
        uint64_t offset = 0; size_t length = 0;
        if (!strcmp(argv[2], "--main-dol")) {
            unsigned char boot[0x440], dol[256];
            if (!melee_disc_read_at(disc, boot, sizeof(boot), 0)) { result = 1; goto done; }
            offset = be32(boot + 0x420);
            if (!melee_disc_read_at(disc, dol, sizeof(dol), offset)) { result = 1; goto done; }
            length = sizeof(dol);
            for (unsigned i = 0; i < 18; ++i) {
                uint64_t end = (uint64_t)be32(dol + 4*i) + be32(dol + 0x90 + 4*i);
                if (end > length) length = end;
            }
        } else {
            int32_t number = melee_disc_find(disc, 0, argv[2]);
            const MeleeDiscEntry* e = number < 0 ? NULL : melee_disc_entry(disc, number);
            if (!e || e->directory) { fprintf(stderr, "File not found\n"); result = 1; goto done; }
            offset = e->offset; length = e->length;
        }
        unsigned char buffer[65536];
        while (length) {
            size_t amount = length < sizeof(buffer) ? length : sizeof(buffer);
            if (!melee_disc_read_at(disc, buffer, amount, offset) || fwrite(buffer, 1, amount, stdout) != amount) { result = 1; break; }
            length -= amount; offset += amount;
        }
        if (fflush(stdout)) result = 1;
    }
done:
    melee_disc_close(disc);
    return result;
}
