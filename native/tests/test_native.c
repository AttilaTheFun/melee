#include "melee_archive.h"
#include "melee_input.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(u32) == 4, "Game word must remain 32 bits on ARM64");
_Static_assert(sizeof(s32) == 4, "Game signed word must remain 32 bits");
_Static_assert(sizeof(PADStatus) == 12, "PAD input layout changed");

static void put32(uint8_t *p, uint32_t v)
{ p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

static void test_archive(void)
{
    uint8_t file[61] = {0};
    put32(file, sizeof(file)); put32(file + 4, 12);
    put32(file + 8, 1); put32(file + 12, 1);
    put32(file + 32, 4); put32(file + 36, 0x3fc00000); /* 1.5f */
    put32(file + 44, 0); put32(file + 48, 4);
    memcpy(file + 56, "root", 5);
    uint8_t original[61]; memcpy(original, file, sizeof(file));
    MeleeArchive a;
    assert(melee_archive_open(&a, file, sizeof(file)));
    uint32_t offset, slot, target; float value;
    assert(melee_archive_find(&a, "root", &offset) && offset == 4);
    assert(melee_archive_f32(&a, offset, &value) && value == 1.5f);
    assert(melee_archive_relocation(&a, 0, &slot, &target) && slot == 0 && target == 4);
    assert(!melee_archive_find(&a, "missing", &offset));
    assert(!melee_archive_u32(&a, 9, &offset));
    assert(!melee_archive_u32(&a, UINT32_MAX, &offset));
    assert(!memcmp(file, original, sizeof(file)));
    MeleeArchive owned, shared, survivor;
    assert(melee_archive_acquire(&owned,&a));
    assert(owned.bytes!=file&&owned.storage);
    assert(melee_archive_acquire(&shared,&owned));
    assert(shared.bytes==owned.bytes);
    assert(melee_archive_acquire(&survivor,&shared));
    melee_archive_release(&owned);melee_archive_release(&shared);
    memset(file,0xa5,sizeof(file));
    assert(melee_archive_f32(&survivor,4,&value)&&value==1.5f);
    melee_archive_release(&survivor);melee_archive_release(&survivor);
    memcpy(file,original,sizeof(file));
    uint8_t* adopted=malloc(sizeof(file));assert(adopted);memcpy(adopted,file,sizeof(file));
    assert(melee_archive_adopt(&owned,adopted,sizeof(file))&&owned.bytes==adopted);
    melee_archive_release(&owned);
    adopted=calloc(1,sizeof(file));assert(adopted);
    assert(!melee_archive_adopt(&owned,adopted,sizeof(file)));free(adopted);

    for (size_t n = 0; n < sizeof(file); ++n) {
        assert(!melee_archive_open(&a, file, n));
        assert(!a.bytes);
    }
    file[60] = 'x'; assert(!melee_archive_open(&a, file, sizeof(file))); file[60] = 0;
    put32(file + 8, UINT32_MAX); assert(!melee_archive_open(&a, file, sizeof(file)));
    memcpy(file, original, sizeof(file));
    put32(file + 44, 1); assert(!melee_archive_open(&a, file, sizeof(file)));
    memcpy(file, original, sizeof(file));
    put32(file + 32, 12); assert(melee_archive_open(&a, file, sizeof(file)));
    assert(melee_archive_relocation(&a, 0, &slot, &target) && target == 12);
    assert(!melee_archive_u32(&a, target, &offset));
    put32(file + 32, 13); assert(!melee_archive_open(&a, file, sizeof(file)));
    memcpy(file, original, sizeof(file));
    put32(file + 34, 8); put32(file + 44, 2);
    assert(melee_archive_open(&a, file, sizeof(file)));
    assert(melee_archive_relocation(&a, 0, &slot, &target) && slot == 2 && target == 8);
    put32(file + 44, 10); assert(!melee_archive_open(&a, file, sizeof(file)));
    memcpy(file, original, sizeof(file));
    /* Reinterpret the symbol table as an external chain, test valid and cyclic. */
    put32(file + 12, 0); put32(file + 16, 1);
    put32(file + 36, UINT32_MAX);
    assert(melee_archive_open(&a, file, sizeof(file)));
    put32(file+36,8);put32(file+40,UINT32_MAX);
    assert(melee_archive_open(&a,file,sizeof(file)));
    size_t resolved_size=0;uint8_t* resolved=melee_archive_copy_null_externals(&a,&resolved_size);assert(resolved&&resolved_size==sizeof(file)-8);
    MeleeArchive view;assert(melee_archive_open(&view,resolved,resolved_size)&&view.extern_count==0);
    MeleeHostBool present;
    assert(melee_archive_pointer(&view,4,&target,&present)&&!present);
    assert(melee_archive_pointer(&view,8,&target,&present)&&!present);
    assert(melee_archive_pointer(&view,0,&target,&present)&&present&&target==4);
    assert(!melee_archive_pointer(&a,4,&target,&present));
    assert(!memcmp(resolved+view.strings_start,file+a.strings_start,5));free(resolved);
    put32(file + 36, 4); assert(!melee_archive_open(&a, file, sizeof(file)));
}

static void test_input(void)
{
    MeleeKeyboard k; melee_keyboard_defaults(&k);
    melee_keyboard_event(&k, 0, true); /* A */
    melee_keyboard_event(&k, 123, true); /* left arrow */
    melee_keyboard_event(&k, 0, false);
    assert(melee_keyboard_read(&k).stickX == -80); /* alternate key still held */
    melee_keyboard_event(&k, 2, true);
    assert(melee_keyboard_read(&k).stickX == 0); /* opposite directions neutral */
    melee_keyboard_clear(&k);
    melee_keyboard_event(&k, 13, true); melee_keyboard_event(&k, 2, true);
    PADStatus p = melee_keyboard_read(&k);
    assert(p.stickX == 57 && p.stickY == 57);
    melee_keyboard_clear(&k);
    assert(melee_keyboard_bind(&k, 0, 1u << MELEE_A));
    assert(!melee_keyboard_bind(&k, 128, 1));
    assert(!melee_keyboard_bind(&k, 0, UINT32_MAX));
    melee_keyboard_event(&k, 0, true);
    p = melee_keyboard_read(&k);
    assert(p.button == PAD_BUTTON_A && p.analogA == 255 && p.stickX == 0);
    PADStatus controller = melee_analog_pad(PAD_BUTTON_B, 0.5f, 0, 0, -1, 0.5f, 2);
    p = melee_pad_merge(p, controller);
    assert(p.button == (PAD_BUTTON_A | PAD_BUTTON_B));
    assert(p.stickX == 40 && p.substickY == -80);
    assert(p.triggerLeft == 70 && p.triggerRight == 140);
    controller.err = PAD_ERR_NO_CONTROLLER;
    assert(melee_pad_merge(p, controller).button == p.button);
    p = melee_analog_pad(0, NAN, INFINITY, 0, 0, NAN, -1);
    assert(p.stickX == 0 && p.stickY == 0 && p.triggerLeft == 0 && p.triggerRight == 0);
    melee_keyboard_clear(&k); assert(melee_keyboard_read(&k).button == 0);
}

int main(void)
{
    test_archive(); test_input();
    puts("Native archive and PAD input tests passed.");
    return 0;
}
