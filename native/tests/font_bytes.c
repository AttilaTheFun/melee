#include <sysdolphin/baselib/sislib_font.h>
#include <sysdolphin/baselib/hsd_3915.h>
#include <stdio.h>
int main(void)
{
    if(fwrite(HSD_SisLib_FontAtlas,1,sizeof(HSD_SisLib_FontAtlas),stdout)!=sizeof(HSD_SisLib_FontAtlas))return 1;
    if(fwrite(HSD_DebugFontAtlas,sizeof(DebugFontGlyph),128,stdout)!=128)return 1;
    return fflush(stdout)!=0;
}
