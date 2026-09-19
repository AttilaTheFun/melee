#include <sysdolphin/baselib/sislib.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* No game objects or rendering should be created for context -1. */
HSD_GObj* GObj_Create(u16 a,u8 b,u8 c){(void)a;(void)b;(void)c;abort();}
void HSD_GObjFree(HSD_GObj* g){(void)g;abort();}
void GObj_InitUserData(HSD_GObj* g,u8 k,void (*d)(void*),void* p){(void)g;(void)k;(void)d;(void)p;abort();}
void GObj_SetupGXLink(HSD_GObj* g,GObj_RenderFunc f,u8 l,u32 p){(void)g;(void)f;(void)l;(void)p;abort();}
void HSD_SisLib_803A84BC(HSD_GObj* g,int pass){(void)g;(void)pass;abort();}
void* HSD_MemAlloc(size_t n){return malloc(n);}
void HSD_Free(void* p){free(p);}
/* Minimal glyph mapping fixture; formatting/encoding needs only A and 1. */
u8 lbl_8040C8C0[0x240]={0x82,0x60,0x82,0x50};
u8 HSD_SisLib_8040C680[0x240]={0x20,0x01,0x20,0x02};
int main(void){
    HSD_SisLib_803A6048(65536);
    void* arena=free_head;
    void* small=HSD_SisLib_Alloc(3);assert(!((uintptr_t)small%_Alignof(SisBlock)));
    memset(small,0xab,3);
    HSD_Text* texts[24];
    for(unsigned i=0;i<24;i++){
        texts[i]=HSD_SisLib_803A6754(0,-1);assert(texts[i]);
        sisLib_803A7664_t* a=(void*)texts[i]->alloc_data;
        assert(a->x8==128&&a->xC==0&&a->x0==a->x4);
        assert(!((uintptr_t)a%_Alignof(sisLib_803A7664_t)));
        assert(texts[i]->string_buffer&&texts[i]->x6E==16);
    }
    /* Alternating letters/digits exercises repeated kerning commands. Native
       formatting retains at most 127 input bytes, including on replacement. */
    char long_text[513];
    for(unsigned i=0;i<512;i++)long_text[i]=(i%2)?'1':'A';
    long_text[512]=0;
    u8 truncated[128],encoded[898];
    memcpy(truncated,long_text,127);truncated[127]=0;
    int length=HSD_SisLib_803A67EC(encoded,truncated);assert(length>128);
    int entry=HSD_SisLib_803A6B98(texts[0],12,24,"%s",long_text);assert(entry==0);
    u8* bytes=fn_803A6FEC((u8*)texts[0]->sis_buffer,entry,NULL);
    assert(bytes&&!memcmp(bytes+14,encoded,length)&&bytes[14+length]==0xF);
    assert(HSD_SisLib_803A6B98(texts[0],0,0,"tail")==1);
    assert(HSD_SisLib_803A70A0(texts[0],0,"%s","short"));
    assert(HSD_SisLib_803A70A0(texts[0],0,"%s",long_text));
    bytes=fn_803A6FEC((u8*)texts[0]->sis_buffer,0,NULL);
    assert(bytes&&!memcmp(bytes+14,encoded,length)&&bytes[14+length]==0xF);
    assert(fn_803A6FEC((u8*)texts[0]->sis_buffer,1,NULL));
    for(unsigned i=0;i<24;i++)HSD_SisLib_803A5A2C(texts[i]);
    HSD_SisLib_Free(small);assert(!used_head);
    free(arena);free_head=NULL;
    puts("SIS allocator: alignment, 24 text objects, bounded formatting and replacement passed");
}
