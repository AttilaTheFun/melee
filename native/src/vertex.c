#include "melee_vertex.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
struct MeleeVertexBatch { MeleeVertex* vertices; MeleeDraw* draws; size_t vertices_count,draws_count; uint32_t attribute_mask; size_t array_bytes[21]; };
static uint32_t read_be(const uint8_t* p,unsigned n)
{ uint32_t v=0;while(n--)v=(v<<8)|*p++;return v; }
static unsigned width(uint32_t format) { return format<2 ? 1 : format<4 ? 2 : format==4 ? 4 : 0; }
static unsigned color_width(uint32_t format)
{ static const unsigned sizes[]={2,3,4,2,3,4};return format<6 ? sizes[format] : 0; }
static MeleeHostBool floats(const uint8_t* bytes,unsigned count,uint32_t format,unsigned fraction,float* out)
{
    unsigned w=width(format);if(!w || fraction>31)return false;
    for(unsigned i=0;i<count;i++) {
        uint32_t bits=read_be(bytes+i*w,w);
        if(format==4) memcpy(out+i,&bits,4);
        else {
            int32_t v=(int32_t)bits;
            if(format==1 && (bits&128))v-=256;
            if(format==3 && (bits&32768))v-=65536;
            out[i]=ldexpf((float)v,-(int)fraction);
        }
        if(!isfinite(out[i]))return false;
    }
    return true;
}
static uint8_t expand(unsigned value,unsigned bits)
{ return (uint8_t)((value<<(8-bits)) | (value>>(2*bits-8))); }
static void color(const uint8_t* p,unsigned format,uint8_t* out)
{
    uint32_t v=read_be(p,color_width(format));out[3]=255;
    switch(format) {
    case 0:out[0]=expand(v>>11,5);out[1]=expand((v>>5)&63,6);out[2]=expand(v&31,5);break;
    case 1:case 2:case 5:memcpy(out,p,3);if(format==5)out[3]=p[3];break;
    case 3:for(unsigned i=0;i<4;i++)out[i]=expand((v>>(12-i*4))&15,4);break;
    case 4:for(unsigned i=0;i<4;i++)out[i]=expand((v>>(18-i*6))&63,6);break;
    }
}
static unsigned components(const MeleeVertexAttribute* a)
{
    unsigned attr=a->attribute==25 ? 10 : a->attribute;
    if(attr<9)return 1;
    if(attr==9)return a->components<=1 ? a->components+2 : 0;
    if(attr==10)return a->components<=2 ? (a->components ? 9 : 3) : 0;
    if(attr==11 || attr==12)return a->components<=1 ? 1 : 0;
    if(attr>=13 && attr<=20)return a->components<=1 ? a->components+1 : 0;
    return 0;
}
static size_t direct_size(const MeleeVertexAttribute* a)
{
    if(a->attribute<9)return 1;
    if(a->attribute==11 || a->attribute==12)return color_width(a->format);
    return components(a)*width(a->format);
}
static MeleeHostBool load(const MeleeVertexAttribute* a,const uint8_t** cursor,const uint8_t* end,MeleeVertex* v,size_t* used)
{
    unsigned attr=a->attribute==25 ? 10 : a->attribute;
    unsigned n=components(a), w=width(a->format), groups=1;
    size_t bytes=direct_size(a); const uint8_t* source;
    if(attr<9) { if(*cursor==end)return false;v->matrix_index[attr]=*(*cursor)++ & 63;return true; }
    if(attr==10 && a->components==2 && a->mode!=1) { groups=3;n=3;bytes=3*w; }
    for(unsigned group=0;group<groups;group++) {
        if(a->mode==1) {
            if((size_t)(end-*cursor)<bytes)return false;
            source=*cursor;*cursor+=bytes;
        } else {
            unsigned index_width=a->mode==2 ? 1 : 2;
            if((size_t)(end-*cursor)<index_width)return false;
            uint32_t index=read_be(*cursor,index_width);*cursor+=index_width;
            if(attr==9 && index==(index_width==1 ? 255u : 65535u))return false;
            if(a->stride && index>SIZE_MAX/a->stride)return false;
            size_t offset=index*a->stride, vector_offset=group*3*w;
            if(offset>a->array_size || vector_offset>a->array_size-offset)return false;
            offset+=vector_offset;
            if(!a->array || bytes>a->array_size-offset)return false;
            source=a->array+offset;
            if(offset+bytes>*used)*used=offset+bytes;
        }
        if(attr==11 || attr==12)color(source,a->format,v->color[attr-11]);
        else {
            unsigned fraction=a->fraction;
            if(attr==10 && a->format<4)fraction=(w*8)-(a->format&1)-1;
            if(attr==10) {
                /* Keep each row within its own C array object. */
                unsigned vectors=n/3;
                for(unsigned j=0;j<vectors;j++) if(!floats(source+j*3*w,3,a->format,fraction,v->normal[group+j]))return false;
            } else if(!floats(source,n,a->format,fraction,attr==9 ? v->position : v->texcoord[attr-13]))return false;
        }
    }
    return true;
}
void melee_vertex_free(MeleeVertexBatch* b) { if(b){free(b->vertices);free(b->draws);free(b);} }
size_t melee_vertex_count(const MeleeVertexBatch* b){return b?b->vertices_count:0;}
size_t melee_draw_count(const MeleeVertexBatch* b){return b?b->draws_count:0;}
size_t melee_vertex_array_bytes(const MeleeVertexBatch* b,uint32_t attr){
    if(attr==25)attr=10;return b&&attr<21?b->array_bytes[attr]:0;
}
uint32_t melee_vertex_attribute_mask(const MeleeVertexBatch* b){return b?b->attribute_mask:0;}
const MeleeVertex* melee_vertex_data(const MeleeVertexBatch* b){return b?b->vertices:NULL;}
const MeleeDraw* melee_draw_data(const MeleeVertexBatch* b){return b?b->draws:NULL;}
MeleeVertexBatch* melee_vertex_decode(const uint8_t* bytes,size_t size,const MeleeVertexAttribute* attrs,size_t count,uint8_t vat)
{
    if(!bytes || !attrs || count>26 || vat>7)return NULL;
    const MeleeVertexAttribute* ordered[21]={0};size_t vertex_size=0;
    for(size_t i=0;i<count;i++) {
        const MeleeVertexAttribute* a=attrs+i;unsigned attr=a->attribute==25?10:a->attribute;
        if(a->mode==0)continue;
        if(attr>20 || ordered[attr] || a->mode>3 || !components(a) || !direct_size(a) || a->fraction>31)return NULL;
        if(attr<9 && a->mode!=1)return NULL;
        ordered[attr]=a;
        vertex_size+=a->mode==1 ? direct_size(a) : (a->mode==2?1:2)*(attr==10 && a->components==2?3:1);
    }
    if(!ordered[9])return NULL;
    MeleeVertexBatch* b=calloc(1,sizeof(*b));if(!b)return NULL;
    for(unsigned attr=0;attr<=20;attr++)if(ordered[attr])b->attribute_mask|=1u<<attr;
    const uint8_t* cursor=bytes;const uint8_t* end=bytes+size;
    while(cursor<end) {
        uint8_t command=*cursor++;if(!command)continue;
        uint8_t primitive=command&0xf8;
        if((command&7)!=vat || (primitive!=0x80 && primitive!=0x90 && primitive!=0x98 && primitive!=0xa0 && primitive!=0xa8 && primitive!=0xb0 && primitive!=0xb8) || end-cursor<2)goto fail;
        size_t n=read_be(cursor,2);cursor+=2;
        if(n>(size_t)(end-cursor)/vertex_size || n>SIZE_MAX-b->vertices_count)goto fail;
        size_t total=b->vertices_count+n;
        if(total>SIZE_MAX/sizeof(*b->vertices) || b->draws_count>=SIZE_MAX/sizeof(*b->draws))goto fail;
        if(n) {
            MeleeVertex* vertices=realloc(b->vertices,total*sizeof(*vertices));if(!vertices)goto fail;b->vertices=vertices;
        }
        MeleeDraw* draws=realloc(b->draws,(b->draws_count+1)*sizeof(*draws));if(!draws)goto fail;b->draws=draws;
        b->draws[b->draws_count++]=(MeleeDraw){primitive,vat,b->vertices_count,n};
        for(size_t i=0;i<n;i++) {
            MeleeVertex* v=b->vertices+b->vertices_count++;memset(v,0,sizeof(*v));memset(v->color,255,sizeof(v->color));
            for(unsigned attr=0;attr<=20;attr++)if(ordered[attr] && !load(ordered[attr],&cursor,end,v,&b->array_bytes[attr]))goto fail;
        }
    }
    return b;
fail:melee_vertex_free(b);return NULL;
}
