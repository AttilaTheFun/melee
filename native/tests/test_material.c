#include "melee_material.h"
#include <sysdolphin/baselib/tobj.h>
#include <assert.h>
_Static_assert(sizeof(HSD_TexLODDesc)==16, "Disk LOD descriptor is 16 bytes");
_Static_assert(offsetof(HSD_TexLODDesc,bias_clamp)==8 && offsetof(HSD_TexLODDesc,edgeLODEnable)==9 && offsetof(HSD_TexLODDesc,max_anisotropy)==12, "LOD field offsets");
#include <math.h>
#include <stdio.h>
#include <string.h>
static uint8_t bytes[32+320+28];
static void word(uint8_t* p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void number(uint8_t* p,float v){uint32_t bits;memcpy(&bits,&v,4);word(p,bits);}
static void fixture(void)
{
    memset(bytes,0,sizeof(bytes));word(bytes,sizeof(bytes));word(bytes+4,320);word(bytes+8,7);uint8_t* d=bytes+32;
    word(d+4,0x14);word(d+8,44);word(d+12,24);memcpy(d+28,(uint8_t[]){128,64,32,255},4);number(d+36,1);number(d+40,8);
    word(d+56,4);number(d+72,1);number(d+76,1);number(d+80,1);word(d+104,0x01010000);word(d+108,0x50010);number(d+112,0.5f);word(d+116,1);word(d+120,136);word(d+128,160);word(d+132,180);
    word(d+136,224);word(d+140,0x00040004);word(d+144,6);word(d+160,1);number(d+164,0.25f);d[168]=1;d[169]=1;d[170]=0xAB;d[171]=0xCD;word(d+172,2);word(d+176,0xFFFFFFFF);d[180]=12;word(d+208,0x80000000);
    for(unsigned i=0;i<16;i++){d[224+i*2]=255;d[225+i*2]=128;d[256+i*2]=64;d[257+i*2]=32;}
    const uint32_t slots[]={8,12,120,128,132,136,8};for(unsigned i=0;i<7;i++)word(d+320+i*4,slots[i]);
}
static MeleeMaterial* decode(void){MeleeArchive a;assert(melee_archive_open(&a,bytes,sizeof(bytes)));return melee_material_decode(&a,0);}
int main(void)
{
    fixture();MeleeMaterial* m=decode();assert(m && m->texture_count==1 && m->render_mode==0x14);
    assert(m->diffuse[0]==128 && m->diffuse[1]==64 && m->alpha==1 && m->shininess==8);
    MeleeMaterialTexture* t=m->textures;assert(t->source==4 && t->flags==0x50010 && t->mip_count==1);
    assert(t->mips[0].width==4 && t->mips[0].size==64 && t->lod_bias==0.25f && t->anisotropy==2 && t->bias_clamp==1 && t->edge_lod==1);
    assert(t->has_tev && t->tev[0]==12 && t->tev_active==0x80000000);
    assert(t->matrix[0][0]==1 && t->matrix[1][1]==1 && t->matrix[0][3]==0);
    memset(bytes,0,sizeof(bytes));assert(!memcmp(t->mips[0].rgba,(uint8_t[]){128,64,32,255},4));melee_material_free(m);
    fixture();uint8_t* d=bytes+32;word(d+100,2);m=decode();assert(m && m->textures[0].matrix[1][3]==-1);melee_material_free(m);
    fixture();word(d+128,0);word(d+320+3*4,8);m=decode();assert(m && m->textures[0].min_filter==5 && m->textures[0].effective_min_filter==1);melee_material_free(m);
    fixture();word(d+144,0);word(d+148,1);number(d+156,1);word(d+160,5);m=decode();assert(m && m->textures[0].mip_count==2 && m->textures[0].mips[1].width==2 && m->textures[0].effective_min_filter==5);melee_material_free(m);
    fixture();word(d+144,8);word(d+148,1);number(d+156,1);word(d+124,160);word(d+128,0);word(d+160,288);word(d+164,1);word(d+172,16u<<16);word(d+320+3*4,160);word(d+320+6*4,124);
    m=decode();assert(m && m->textures[0].mip_count==2 && m->textures[0].effective_min_filter==3);melee_material_free(m);
    fixture();word(d+48,44);word(d+320+24,48);assert(!decode());
    fixture();word(d+136,316);assert(!decode());
    fixture();number(d+72,NAN);assert(!decode());
    fixture();word(d+160,7);assert(!decode());
    fixture();word(d+104,0);assert(!decode());
    assert(!melee_material_decode(NULL,0));melee_material_free(NULL);
    puts("Material decoding: owned RGBA pixels, sampler/TEV fields, original texture matrices, cycles and malformed descriptors passed");
}
