#include <sysdolphin/baselib/sislib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
SIS* HSD_SisLib_804D1124[5];
void* HSD_SisLib_Alloc(s32 n){void* p=calloc(1,n);assert(p);return p;}
void HSD_SisLib_Free(void* p){free(p);}
static void check(u8* stream,float expected,float height)
{
    HSD_Text text={0};text.x80.x=1;text.x80.y=1;text.kerning=1;
    text.string_buffer=calloc(1,32);assert(text.string_buffer);text.x6E=32;
    float width=0,h=0;HSD_SisLib_803A8134(stream,&text,&width,&h);
    assert(fabsf(width-expected)<.001f&&fabsf(h-height)<.001f);
    assert(text.x80.x==1&&text.x80.y==1&&text.x78.x==0&&text.x6C==0);
    free(text.string_buffer);
}
int main(void)
{
    assert((uintptr_t)HSD_SisLib_8040CB00>UINT32_MAX);
    float a=34-HSD_SisLib_8040CB00[48]-HSD_SisLib_8040CB00[49];
    float b=34-HSD_SisLib_8040CB00[58]-HSD_SisLib_8040CB00[59];
    u8 glyphs[]={22,0x20,24,0x20,29,0};check(glyphs,a+b,32);
    u8 scale[]={14,2,0,1,0,0x20,24,15,0x20,24,0};check(scale,3*a,32);
    u8 spacing[]={10,0xfe,0x80,0,0,0x20,24,11,0};check(spacing,a-1.5f,32);
    TextGlyphTexture* texture=calloc(1,sizeof(*texture));assert(texture);
    texture->data[0]=8;texture->data[1]=9;
    SIS font={.textures=texture};HSD_SisLib_804D1124[0]=&font;
    u8 custom[]={0x40,0,0};check(custom,17,32);
    free(texture);HSD_SisLib_804D1124[0]=NULL;
    puts("Original SIS measurement: full-width kerning, big-endian glyphs, scale/spacing stack and custom metrics passed");
}
