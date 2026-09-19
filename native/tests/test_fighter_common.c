#include "melee_fighter_common.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void word(u8* p, u32 value)
{
    p[0] = value >> 24;
    p[1] = value >> 16;
    p[2] = value >> 8;
    p[3] = value;
}

int main(int argc, char** argv)
{
    /* One relocated root pointer, the complete scalar block, one public. */
    size_t size = 32 + 4 + 0x818 + 4 + 8 + sizeof("ftLoadCommonData");
    u8* bytes = calloc(1, size);
    assert(bytes);
    word(bytes, size);
    word(bytes + 4, 4 + 0x818);
    word(bytes + 8, 1);
    word(bytes + 12, 1);
    word(bytes + 32, 4);
    for (unsigned i = 0; i < 0x818; i += 4)
        word(bytes + 36 + i, 0x3f800000 + i * 8192);
    memcpy(bytes + size - sizeof("ftLoadCommonData"), "ftLoadCommonData",
           sizeof("ftLoadCommonData"));
    if (argc == 2) {
        free(bytes);
        FILE* file = fopen(argv[1], "rb");
        assert(file && !fseek(file, 0, SEEK_END));
        long length = ftell(file);
        assert(length > 0);
        rewind(file);
        size = length;
        bytes = malloc(size);
        assert(bytes && fread(bytes, 1, size, file) == size);
        fclose(file);
    } else {
        assert(argc == 1);
    }
    MeleeArchive archive;
    assert(melee_archive_open(&archive, bytes, size));
    u32 root, data;
    MeleeHostBool present;
    assert(melee_archive_find(&archive, "ftLoadCommonData", &root));
    assert(melee_archive_pointer(&archive, root, &data, &present) && present);
    ftCommonData result;
    assert(melee_fighter_common_decode(&archive, &result));
    for (unsigned i = 0; i < 0x818; i += 4) {
        if ((i >= 0x6dc && i < 0x6f0) || i == 0x7d8) {
            assert(!memcmp((u8*)&result + i, bytes + 32 + data + i, 4));
        } else {
            u32 expected, actual;
            assert(melee_archive_u32(&archive, data + i, &expected));
            memcpy(&actual, (u8*)&result + i, 4);
            assert(actual == expected);
        }
    }
    if (argc == 2) {
        assert(result.x23C == 100 && result.x500 == 60);
        assert(result.horizontal_stick_deadzone > 0 &&
               result.horizontal_stick_deadzone < 1);
    }
    ftCommonData saved = result;
    word(bytes + 32 + data, 0x7fc00000);
    assert(!melee_fighter_common_decode(&archive, &result));
    assert(!memcmp(&saved, &result, sizeof(result)));
    word(bytes + 32 + data, 0x3f800000);
    /* A relocation in a numeric field is an unsupported archive schema. */
    word(bytes + archive.reloc_start, data);
    assert(!melee_fighter_common_decode(&archive, &result));
    assert(!memcmp(&saved, &result, sizeof(result)));
    word(bytes + archive.reloc_start, root);
    archive.data_size = data + 0x817;
    assert(!melee_fighter_common_decode(&archive, &result));
    memset(bytes, 0xa5, size);
    free(bytes);
    assert(!memcmp(&saved, &result, sizeof(result)));
    puts("Fighter common parameters: scalar/byte values, numeric unknowns, source disposal and invalid input passed");
    return 0;
}
