#include "melee_disc.h"
#include "melee_archive.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static MeleeHostBool bundle(const uint8_t* bytes, size_t size, unsigned* members)
{
    size_t cursor = 0;
    unsigned count = 0;
    while (cursor < size) {
        if (size - cursor < 32) return false;
        const uint8_t* p = bytes + cursor;
        uint32_t length = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
                          ((uint32_t)p[2] << 8) | p[3];
        MeleeArchive a;
        if (length > size - cursor || !melee_archive_open(&a, p, length)) return false;
        ++count;
        size_t advance = ((size_t)length + 31) & ~(size_t)31;
        if (advance > size - cursor) return false;
        cursor += advance;
    }
    if (count < 2) return false;
    *members += count;
    return true;
}
int main(int argc, char** argv)
{
    if (argc != 2) { fprintf(stderr, "Usage: verify-disc IMAGE\n"); return 2; }
    MeleeDisc* disc = melee_disc_open(argv[1]);
    if (!disc) return 1;
    unsigned passed = 0, failed = 0, bundles = 0, members = 0;
    for (uint32_t i = 1; i < melee_disc_entry_count(disc); ++i) {
        const MeleeDiscEntry* e = melee_disc_entry(disc, i);
        const char* extension = strrchr(e->name, '.');
        if (e->directory || !extension || (strcmp(extension, ".dat") && strcmp(extension, ".usd"))) continue;
        void* bytes = malloc(e->length ? e->length : 1);
        MeleeArchive archive;
        MeleeHostBool loaded = bytes && melee_disc_read(disc, i, bytes, e->length, 0);
        if (loaded && melee_archive_open(&archive, bytes, e->length)) ++passed;
        else if (loaded && bundle(bytes, e->length, &members)) ++bundles;
        else { ++failed; printf("Not accepted as HSD archive: entry %u %s\n", i, e->name); }
        free(bytes);
    }
    printf("HSD archive scan: %u standalone, %u bundles (%u members), %u unrecognized\n", passed, bundles, members, failed);
    melee_disc_close(disc);
    return failed ? 1 : 0;
}
