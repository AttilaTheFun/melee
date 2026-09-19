#ifndef MELEE_NATIVE_MATERIAL_H
#define MELEE_NATIVE_MATERIAL_H
#include "melee_archive.h"
typedef struct { uint16_t width,height; size_t size; uint8_t* rgba; } MeleeMaterialMip;
typedef struct {
    uint32_t offset,id,source,flags,wrap_s,wrap_t,mag_filter,min_filter;
    uint32_t image_format,effective_min_filter;
    MeleeHostBool has_mipmap;
    uint8_t repeat_s,repeat_t;
    float rotation[3],scale[3],translation[3],matrix[3][4];
    float blend,lod_bias,min_lod,max_lod;
    uint32_t bias_clamp,edge_lod,anisotropy;
    MeleeHostBool has_tev;
    uint8_t tev[28]; uint32_t tev_active;
    size_t mip_count; MeleeMaterialMip* mips;
} MeleeMaterialTexture;
typedef struct MeleeMaterial {
    uint32_t offset,render_mode,render_desc_offset;
    uint8_t ambient[4],diffuse[4],specular[4];float alpha,shininess;
    MeleeHostBool has_pixel_engine;uint8_t pixel_engine[12];
    size_t texture_count;MeleeMaterialTexture* textures;
} MeleeMaterial;
/* Owns all decoded fields and pixels. Unresolved references, custom classes,
 * malformed descriptors and unsupported GX image formats fail explicitly.
 * Pixel-engine/TEV fields are retained; decoding does not execute those stages. */
MeleeMaterial* melee_material_decode(const MeleeArchive* archive,uint32_t offset);
void melee_material_free(MeleeMaterial* material);
#endif
