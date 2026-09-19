#include <dolphin/gx.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
extern void aurora_abi_sizes(size_t* sizes);
extern size_t aurora_fifo_copy(unsigned char* out,size_t capacity);
int main(void)
{
    size_t backend[7],client[]={sizeof(GXTexObj),sizeof(GXTlutObj),sizeof(GXLightObj),sizeof(GXColor),sizeof(GXRenderModeObj),sizeof(GXVtxAttrFmtList),sizeof(GXFogAdjTable)};
    aurora_abi_sizes(backend);assert(!memcmp(client,backend,sizeof(client)));
    assert(sizeof(void*)==8 && _Alignof(GXTexObj)>=8 && _Alignof(GXTlutObj)>=8);
    struct {uint64_t before;GXTexObj texture;uint64_t after;} t={0};
    struct {uint64_t before;GXTlutObj palette;uint64_t after;} p={0};
    t.before=t.after=p.before=p.after=UINT64_C(0x12345678ABCDEF90);
    uint8_t pixels[256]={0},palette[32]={0};int userdata=42;
    assert((uintptr_t)pixels>UINT32_MAX && (uintptr_t)&userdata>UINT32_MAX);
    GXInitTexObj(&t.texture,pixels,8,4,GX_TF_RGBA8,GX_REPEAT,GX_MIRROR,1);
    GXInitTexObjLOD(&t.texture,GX_LIN_MIP_LIN,GX_NEAR,1,5,-0.5f,1,0,GX_ANISO_4);
    GXInitTexObjUserData(&t.texture,&userdata);
    assert(GXGetTexObjData(&t.texture)==pixels && GXGetTexObjUserData(&t.texture)==&userdata);
    assert(GXGetTexObjWidth(&t.texture)==8 && GXGetTexObjHeight(&t.texture)==4 && GXGetTexObjFmt(&t.texture)==GX_TF_RGBA8);
    assert(GXGetTexObjWrapS(&t.texture)==GX_REPEAT && GXGetTexObjWrapT(&t.texture)==GX_MIRROR && GXGetTexObjMipMap(&t.texture));
    assert(GXGetTexObjMinFilt(&t.texture)==GX_LIN_MIP_LIN && GXGetTexObjMagFilt(&t.texture)==GX_NEAR);
    assert(GXGetTexObjMinLOD(&t.texture)==1 && GXGetTexObjMaxLOD(&t.texture)==5 && GXGetTexObjLODBias(&t.texture)==-0.5f);
    assert(GXGetTexObjBiasClamp(&t.texture) && !GXGetTexObjEdgeLOD(&t.texture) && GXGetTexObjMaxAniso(&t.texture)==GX_ANISO_4);
    /* Negative bias must not spill into anisotropy/clamp bits. Exercise every
       legal filter and anisotropy, both booleans, and bias quantization. */
    const float biases[]={-8,-4,-0.51f,-0.5f,0,0.51f,3.99f,8};
    for (int f=0;f<6;f++) for (int a=0;a<3;a++)
    for (int c=0;c<2;c++) for (int e=0;e<2;e++)
    for (size_t b=0;b<sizeof(biases)/sizeof(biases[0]);b++) {
        float clamped=biases[b]<-4 ? -4 : biases[b]>3.99f ? 3.99f : biases[b];
        float expected=(int)(clamped*32)/32.0f;
        GXInitTexObjLOD(&t.texture,f,GX_LINEAR,-2,12,biases[b],c,e,a);
        assert(GXGetTexObjMinFilt(&t.texture)==f && GXGetTexObjMagFilt(&t.texture)==GX_LINEAR);
        assert(GXGetTexObjMinLOD(&t.texture)==0 && GXGetTexObjMaxLOD(&t.texture)==10);
        assert(GXGetTexObjLODBias(&t.texture)==expected && GXGetTexObjBiasClamp(&t.texture)==c);
        assert(GXGetTexObjEdgeLOD(&t.texture)==e && GXGetTexObjMaxAniso(&t.texture)==a);
        GXInitTexObjLODBias(&t.texture,-0.5f);
        assert(GXGetTexObjLODBias(&t.texture)==-0.5f && GXGetTexObjMaxAniso(&t.texture)==a);
    }
    GXInitTexObjCI(&t.texture,pixels,8,4,GX_TF_C8,GX_CLAMP,GX_CLAMP,0,7);
    assert(GXGetTexObjTlut(&t.texture)==7 && GXGetTexObjFmt(&t.texture)==GX_TF_C8);
    GXInitTlutObj(&p.palette,palette,GX_TL_RGB5A3,16);
    assert(GXGetTlutObjData(&p.palette)==palette && GXGetTlutObjNumEntries(&p.palette)==16 && GXGetTlutObjFmt(&p.palette)==GX_TL_RGB5A3);
    assert(t.before==UINT64_C(0x12345678ABCDEF90) && t.after==t.before && p.before==t.before && p.after==t.before);
    GXLightObj light={0};GXColor color={12,34,56,78},out={0};float x,y,z;
    GXInitLightColor(&light,color);GXGetLightColor(&light,&out);assert(!memcmp(&color,&out,sizeof(color)));
    GXInitLightPos(&light,1.25f,-2.5f,3.75f);GXGetLightPos(&light,&x,&y,&z);assert(x==1.25f && y==-2.5f && z==3.75f);
    /* Independent geometric fixtures: near plane one unit from the origin,
       half-widths one (orthographic) and one-half (perspective). */
    GXFogAdjTable fog;
    float projection[4][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
    const u16 orthographic[10]={257,261,267,275,286,298,312,327,344,362};
    const u16 perspective[10]={256,257,258,261,263,267,271,275,280,286};
    GXInitFogAdjTable(&fog,640,projection);
    assert(!memcmp(fog.r,orthographic,sizeof(orthographic)));
    projection[0][0]=2;projection[2][2]=-1;projection[2][3]=-2;projection[3][3]=0;
    GXInitFogAdjTable(&fog,640,projection);
    assert(!memcmp(fog.r,perspective,sizeof(perspective)));
    uint8_t fifo[44];
    GXSETARRAY(GX_VA_POS,pixels,48,6,GX_FALSE);
    GXSETARRAY(GX_VA_NRM,pixels+64,24,12,GX_TRUE);
    assert(aurora_fifo_copy(fifo,sizeof(fifo))==44);
    for(unsigned command=0;command<2;command++){
        const uint8_t* b=fifo+command*22;uintptr_t pointer=(uintptr_t)(pixels+command*64);
        assert(b[0]==0x50&&b[1]==0&&b[2]==0x10+command);
        for(unsigned i=0;i<8;i++)assert(b[3+i]==(uint8_t)(pointer>>(56-i*8)));
        assert(b[11]==0&&b[12]==0&&b[13]==0&&b[14]==(command?24:48));
        assert(b[15]==command&&b[16]==8&&b[17]==0xB0+command);
        assert(b[18]==0&&b[19]==0&&b[20]==0&&b[21]==(command?12:6));
    }
    puts("Aurora GX ABI: object sizes, full-width pointers, texture/palette getters, LOD, colors and light positions passed (no window or GPU initialized)");
}
