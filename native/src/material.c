#include "melee_material.h"
#include "melee_texture.h"
#include <sysdolphin/baselib/tobj.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static MeleeHostBool range(const MeleeArchive* a,uint32_t at,size_t size){return at<=a->data_size && size<=a->data_size-at;}
static MeleeHostBool ref(const MeleeArchive* a,uint32_t at,uint32_t* target){MeleeHostBool present;if(!melee_archive_pointer(a,at,target,&present))return false;if(!present)*target=UINT32_MAX;return true;}
static MeleeHostBool scalar(const MeleeArchive* a,uint32_t at,float* value){return melee_archive_f32(a,at,value) && isfinite(*value);}
static MeleeHostBool vector(const MeleeArchive* a,uint32_t at,float v[3]){return scalar(a,at,v) && scalar(a,at+4,v+1) && scalar(a,at+8,v+2);}
void melee_material_free(MeleeMaterial* m)
{
    if(!m)return;
    for(size_t i=0;i<m->texture_count;i++){for(size_t j=0;j<m->textures[i].mip_count;j++)free(m->textures[i].mips[j].rgba);free(m->textures[i].mips);}
    free(m->textures);free(m);
}
static MeleeHostBool image(const MeleeArchive* a,uint32_t desc,uint32_t palette,MeleeMaterialTexture* t)
{
    if(desc==UINT32_MAX)return true;
    uint32_t data,dimensions,format,mipmap,paldata=UINT32_MAX,palformat=0,palcount=0;
    if(!range(a,desc,24) || !ref(a,desc,&data) || !melee_archive_u32(a,desc+4,&dimensions) || !melee_archive_u32(a,desc+8,&format) || !melee_archive_u32(a,desc+12,&mipmap) || !scalar(a,desc+16,&t->min_lod) || !scalar(a,desc+20,&t->max_lod) || t->min_lod<0 || t->max_lod<t->min_lod || t->max_lod>16)return false;
    if(palette!=UINT32_MAX){
        if(!range(a,palette,16) || !ref(a,palette,&paldata) || !melee_archive_u32(a,palette+4,&palformat) || !melee_archive_u32(a,palette+12,&palcount))return false;
        palcount=(palcount>>16)*2;if(!range(a,paldata,palcount))return false;
    }
    t->image_format=format;t->has_mipmap=!!mipmap;
    uint16_t width=dimensions>>16,height=dimensions;
    t->mip_count=mipmap ? (size_t)t->max_lod+1:1;t->mips=calloc(t->mip_count,sizeof(*t->mips));if(!t->mips){t->mip_count=0;return false;}
    for(size_t i=0;i<t->mip_count;i++){
        size_t encoded=melee_texture_level_size(width,height,format);if(!encoded || !range(a,data,encoded))return false;
        size_t pixels=(size_t)width*height;if(pixels>SIZE_MAX/4)return false;
        MeleeMaterialMip* mip=t->mips+i;*mip=(MeleeMaterialMip){width,height,pixels*4,NULL};mip->rgba=malloc(mip->size);if(!mip->rgba)return false;
        if(!melee_texture_decode(a->bytes+32+data,encoded,width,height,format,paldata==UINT32_MAX?NULL:a->bytes+32+paldata,palcount,palformat,mip->rgba,mip->size))return false;
        data+=(uint32_t)encoded;width=width>1?width/2:1;height=height>1?height/2:1;
    }
    return true;
}
static MeleeHostBool texture(const MeleeArchive* a,uint32_t at,MeleeMaterialTexture* t,uint32_t* next)
{
    uint32_t classname,image_desc,palette,lod,tev,repeat;
    if(!range(a,at,92) || !ref(a,at,&classname) || classname!=UINT32_MAX || !ref(a,at+4,next) || !melee_archive_u32(a,at+8,&t->id) || !melee_archive_u32(a,at+12,&t->source) || !vector(a,at+16,t->rotation) || !vector(a,at+28,t->scale) || !vector(a,at+40,t->translation) || !melee_archive_u32(a,at+52,&t->wrap_s) || !melee_archive_u32(a,at+56,&t->wrap_t) || !melee_archive_u32(a,at+60,&repeat) || !melee_archive_u32(a,at+64,&t->flags) || !scalar(a,at+68,&t->blend) || !melee_archive_u32(a,at+72,&t->mag_filter) || !ref(a,at+76,&image_desc) || !ref(a,at+80,&palette) || !ref(a,at+84,&lod) || !ref(a,at+88,&tev))return false;
    t->offset=at;t->repeat_s=repeat>>24;t->repeat_t=repeat>>16;
    if(!t->repeat_s || !t->repeat_t || t->wrap_s>2 || t->wrap_t>2 || t->mag_filter>1)return false;
    t->min_filter=5;
    if(lod!=UINT32_MAX){
        if(!range(a,lod,16) || !melee_archive_u32(a,lod,&t->min_filter) || !scalar(a,lod+4,&t->lod_bias) || !melee_archive_u32(a,lod+12,&t->anisotropy) || t->min_filter>5 || t->anisotropy>2)return false;
        t->bias_clamp=a->bytes[32+lod+8];t->edge_lod=a->bytes[32+lod+9];
    }
    if(tev!=UINT32_MAX){if(!range(a,tev,32) || !melee_archive_u32(a,tev+28,&t->tev_active))return false;memcpy(t->tev,a->bytes+32+tev,28);t->has_tev=true;}
    HSD_TObj native={0};native.repeat_s=t->repeat_s;native.repeat_t=t->repeat_t;native.wrap_t=t->wrap_t;
    native.rotate=(Quaternion){t->rotation[0],t->rotation[1],t->rotation[2],0};native.scale=(Vec3){t->scale[0],t->scale[1],t->scale[2]};native.translate=(Vec3){t->translation[0],t->translation[1],t->translation[2]};
    HSD_TObjMakeTextureMtx(&native);memcpy(t->matrix,native.mtx,sizeof(t->matrix));
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<4;j++)if(!isfinite(t->matrix[i][j]))return false;
    if(!image(a,image_desc,palette,t))return false;
    t->effective_min_filter=t->min_filter;
    if(t->image_format>=8 && t->image_format<=10 && t->effective_min_filter==5)t->effective_min_filter=3;
    if(!t->has_mipmap)t->effective_min_filter&=1;
    return true;
}
MeleeMaterial* melee_material_decode(const MeleeArchive* a,uint32_t at)
{
    if(!a || !a->bytes || !range(a,at,24))return NULL;
    uint32_t classname,tex,mat,pe;
    MeleeMaterial* m=calloc(1,sizeof(*m));if(!m)return NULL;m->offset=at;
    if(!ref(a,at,&classname) || classname!=UINT32_MAX || !melee_archive_u32(a,at+4,&m->render_mode) || !ref(a,at+8,&tex) || !ref(a,at+12,&mat) || !ref(a,at+16,&m->render_desc_offset) || !ref(a,at+20,&pe) || !range(a,mat,20))goto fail;
    memcpy(m->ambient,a->bytes+32+mat,4);memcpy(m->diffuse,a->bytes+32+mat+4,4);memcpy(m->specular,a->bytes+32+mat+8,4);
    if(!scalar(a,mat+12,&m->alpha) || !scalar(a,mat+16,&m->shininess))goto fail;
    if(pe!=UINT32_MAX){if(!range(a,pe,12))goto fail;memcpy(m->pixel_engine,a->bytes+32+pe,12);m->has_pixel_engine=true;}
    while(tex!=UINT32_MAX){
        for(size_t i=0;i<m->texture_count;i++)if(m->textures[i].offset==tex)goto fail;
        if(m->texture_count>=SIZE_MAX/sizeof(MeleeMaterialTexture))goto fail;
        MeleeMaterialTexture* ts=realloc(m->textures,(m->texture_count+1)*sizeof(*ts));if(!ts)goto fail;m->textures=ts;
        MeleeMaterialTexture* t=ts+m->texture_count++;memset(t,0,sizeof(*t));
        if(!texture(a,tex,t,&tex))goto fail;
    }
    return m;
fail:melee_material_free(m);return NULL;
}
