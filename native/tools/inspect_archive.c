#include "melee_archive.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "Usage: %s file.dat\n", argv[0]); return 2; }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }
    if (fseek(f, 0, SEEK_END)) { fclose(f); return 1; }
    long length = ftell(f);
    if (length < 32 || (unsigned long)length > UINT32_MAX || fseek(f, 0, SEEK_SET)) {
        fprintf(stderr, "Invalid archive size\n"); fclose(f); return 1;
    }
    void *bytes = malloc((size_t)length);
    if (!bytes) { fclose(f); return 1; }
    size_t read = fread(bytes, 1, (size_t)length, f);
    fclose(f);
    MeleeArchive a;
    if (read != (size_t)length || !melee_archive_open(&a, bytes, read)) {
        fprintf(stderr, "Invalid HSD archive\n"); free(bytes); return 1;
    }
    printf("Data: %u bytes; relocations: %u; public: %u; external: %u\n",
           a.data_size, a.reloc_count, a.public_count, a.extern_count);
    for (uint32_t i = 0; i < a.public_count; ++i) {
        const char *name;
        uint32_t offset;
        if (!melee_archive_public(&a, i, &name, &offset)) { free(bytes); return 1; }
        printf("%08x %s\n", offset, name);
    }
    free(bytes);
    return 0;
}
