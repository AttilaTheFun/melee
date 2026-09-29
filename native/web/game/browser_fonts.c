#include "disc_fonts.h"
#include <sysdolphin/baselib/sislib_font.h>
#include <sysdolphin/baselib/hsd_3915.h>

MeleeHostBool melee_browser_init_fonts(const MeleeDisc* disc)
{
    return melee_browser_load_fonts(disc, HSD_SisLib_FontAtlas, HSD_DebugFontAtlas);
}
