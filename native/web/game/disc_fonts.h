#ifndef MELEE_BROWSER_DISC_FONTS_H
#define MELEE_BROWSER_DISC_FONTS_H
#include "melee_disc.h"
#include <string.h>

static uint32_t browser_font_be32(const uint8_t* p)
{
    return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];
}

/* Translate original virtual addresses through the player's DOL section table.
 * Neither generated font includes nor font bytes belong in hosted Wasm. */
static MeleeHostBool melee_browser_load_fonts(const MeleeDisc* disc,
                                               void* text, void* debug)
{
    const uint8_t* id = melee_disc_id(disc);
    uint8_t position[4], header[256];
    if (!id || memcmp(id, "GALE01", 6) || id[7] != 2 ||
        !melee_disc_read_at(disc, position, sizeof(position), 0x420)) return false;
    uint64_t dol = browser_font_be32(position);
    if (dol < 0x440 || !melee_disc_read_at(disc, header, sizeof(header), dol)) return false;
    const uint32_t addresses[2] = {0x8040CD40, 0x804088B8};
    const uint32_t lengths[2] = {287*512, 128*56};
    void* outputs[2] = {text, debug};
    for (unsigned font = 0; font < 2; ++font) {
        uint64_t source = 0;
        unsigned matches = 0;
        for (unsigned section = 0; section < 18; ++section) {
            uint32_t offset = browser_font_be32(header + 4*section);
            uint32_t base = browser_font_be32(header + 0x48 + 4*section);
            uint32_t size = browser_font_be32(header + 0x90 + 4*section);
            if (base <= addresses[font] &&
                (uint64_t)addresses[font] + lengths[font] <= (uint64_t)base + size) {
                if (offset < sizeof(header)) return false;
                source = dol + offset + addresses[font] - base;
                ++matches;
            }
        }
        if (matches != 1 || !outputs[font] ||
            !melee_disc_read_at(disc, outputs[font], lengths[font], source)) return false;
    }
    return true;
}
#endif
