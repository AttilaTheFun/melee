#include "melee_vertex.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void near(float a,float b){assert(fabsf(a-b)<0.00001f);}
int main(void)
{
    const uint8_t positions[64]={0,2,0xff,0xfc,0,6,0,8,0,10,0,12};
    const uint8_t normals[64]={0x40,0,0,0,0,0};
    const uint8_t uv[64]={0x3f,0,0,0,0x3f,0x80,0,0};
    /* Deliberately unsorted descriptors: GX attribute order determines bytes. */
    MeleeVertexAttribute attrs[]={
        {.attribute=13,.mode=3,.components=1,.format=4,.stride=8,.array=uv,.array_size=sizeof(uv)},
        {.attribute=9,.mode=2,.components=1,.format=3,.fraction=1,.stride=6,.array=positions,.array_size=sizeof(positions)},
        {.attribute=11,.mode=1,.components=1,.format=3},
        {.attribute=0,.mode=1},
        {.attribute=10,.mode=2,.components=0,.format=3,.stride=6,.array=normals,.array_size=sizeof(normals)},
    };
    uint8_t list[]={0,0x90,0,2, 0x83,0,0,0xf8,0x04,0,0, 6,1,0,0x12,0x34,0,0, 0};
    MeleeVertexBatch* b=melee_vertex_decode(list,sizeof(list),attrs,5,0);assert(b);
    assert(melee_vertex_count(b)==2 && melee_draw_count(b)==1);
    assert(melee_vertex_attribute_mask(b)==((1u<<0)|(1u<<9)|(1u<<10)|(1u<<11)|(1u<<13)));
    assert(melee_vertex_array_bytes(b,9)==12 && melee_vertex_array_bytes(b,10)==6 && melee_vertex_array_bytes(b,13)==8);
    assert(!melee_vertex_array_bytes(b,11) && !melee_vertex_array_bytes(b,99) && !melee_vertex_array_bytes(NULL,9));
    const MeleeVertex* v=melee_vertex_data(b);
    near(v[0].position[0],1);near(v[0].position[1],-2);near(v[0].position[2],3);
    near(v[1].position[0],4);near(v[1].position[2],6);
    near(v[0].normal[0][0],1);near(v[0].texcoord[0][0],0.5f);near(v[0].texcoord[0][1],1);
    assert(v[0].matrix_index[0]==3 && v[1].matrix_index[0]==6);
    assert(v[0].color[0][0]==255 && v[0].color[0][1]==136 && v[0].color[0][3]==68);
    assert(v[0].color[1][3]==255 && melee_draw_data(b)[0].primitive==0x90);
    melee_vertex_free(b);
    for(size_t n=2;n<sizeof(list)-1;n++) assert(!melee_vertex_decode(list,n,attrs,5,0));
    uint8_t saved=list[5];list[5]=255;assert(!melee_vertex_decode(list,sizeof(list),attrs,5,0));list[5]=saved;
    attrs[1].array_size=5;assert(!melee_vertex_decode(list,sizeof(list),attrs,5,0));attrs[1].array_size=sizeof(positions);
    assert(!melee_vertex_decode(list,sizeof(list),attrs,5,1));
    attrs[4].attribute=9;assert(!melee_vertex_decode(list,sizeof(list),attrs,5,0));attrs[4].attribute=10;

    MeleeVertexAttribute pos={.attribute=9,.mode=1,.components=0,.format=0};
    uint8_t primitives[]={0x80,0,1,2,3, 0x98,0,1,4,5, 0xa0,0,1,6,7, 0xa8,0,1,8,9, 0xb0,0,1,10,11, 0xb8,0,1,12,13};
    b=melee_vertex_decode(primitives,sizeof(primitives),&pos,1,0);assert(b && melee_draw_count(b)==6 && melee_vertex_count(b)==6);
    assert(melee_draw_data(b)[5].first==5);near(melee_vertex_data(b)[5].position[2],0);melee_vertex_free(b);
    uint8_t unknown[]={0x61,0,0,0,0};assert(!melee_vertex_decode(unknown,sizeof(unknown),&pos,1,0));
    /* NBT3: three indices, each selecting its vector at offset 0/3/6. */
    uint8_t nbt[64]={0};nbt[0]=64;nbt[1]=192;nbt[12]=64;nbt[26]=64;
    MeleeVertexAttribute multi[]={pos,{.attribute=25,.mode=2,.components=2,.format=1,.stride=9,.array=nbt,.array_size=sizeof(nbt)}};
    uint8_t dl[]={0xb8,0,1,0,0,0,1,2};
    b=melee_vertex_decode(dl,sizeof(dl),multi,2,0);assert(b);v=melee_vertex_data(b);
    assert(melee_vertex_array_bytes(b,25)==27 && melee_vertex_array_bytes(b,10)==27);
    near(v->normal[0][0],1);near(v->normal[0][1],-1);near(v->normal[1][0],1);near(v->normal[2][2],1);melee_vertex_free(b);
    /* RGB565, RGB8, RGBX8, RGBA4, RGBA6, RGBA8. */
    const unsigned widths[]={2,3,4,2,3,4};
    for(unsigned format=0;format<6;format++) {
        MeleeVertexAttribute ca[]={pos,{.attribute=11,.mode=1,.components=1,.format=format}};
        uint8_t data[]={0xb8,0,1,0,0,255,255,255,255};
        b=melee_vertex_decode(data,5+widths[format],ca,2,0);assert(b);
        for(unsigned i=0;i<4;i++)assert(melee_vertex_data(b)->color[0][i]==255);
        melee_vertex_free(b);
    }
    MeleeVertexAttribute packed[]={pos,{.attribute=11,.mode=1,.components=1,.format=4}};
    uint8_t rgba6[]={0xb8,0,1,0,0,0x04,0x20,0xc4};
    b=melee_vertex_decode(rgba6,sizeof(rgba6),packed,2,0);assert(b);
    for(unsigned i=0;i<4;i++)assert(melee_vertex_data(b)->color[0][i]==(i+1)*4);
    melee_vertex_free(b);
    MeleeVertexAttribute fp={.attribute=9,.mode=1,.components=0,.format=4};
    uint8_t nonfinite[]={0xb8,0,1,0x7f,0xc0,0,0,0,0,0,0};
    assert(!melee_vertex_decode(nonfinite,sizeof(nonfinite),&fp,1,0));
    melee_vertex_free(NULL);
    puts("GX vertex decoding: indexed/direct data, endian conversion, fixed point, NBT3, colors, matrix indices, draw boundaries and malformed input passed");
}
