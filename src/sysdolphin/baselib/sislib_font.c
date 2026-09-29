#include "sislib_font.h"

#ifdef __EMSCRIPTEN__
/* Browser builds obtain these bytes from the player's local disc. */
TextGlyphTexture HSD_SisLib_FontAtlas[287] ATTRIBUTE_ALIGN(32);
#else
TextGlyphTexture HSD_SisLib_FontAtlas[] ATTRIBUTE_ALIGN(32) = {
#include <sysdolphin/baselib/sislib_font.inc>
};
#endif
