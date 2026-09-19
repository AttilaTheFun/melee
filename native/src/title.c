#include "melee_title.h"
#include "melee_scene.h"
#include "melee_camera.h"
#include "melee_environment.h"
#include "melee_texture.h"
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/tobj.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LIMIT 32
#define NONE UINT32_MAX
struct MeleeTitle {
    MeleeTitleData data;
    MeleeScene* models[2];
    MeleeEnvironment* environment;
    HSD_ImageDesc image;
    void* pixels;
};
static int ref(const MeleeArchive* a,uint32_t at,uint32_t* out){
    MeleeHostBool present;if(!melee_archive_pointer(a,at,out,&present))return 0;
    if(!present)*out=NONE;return 1;
}
static int range(const MeleeArchive* a,uint32_t at,size_t n){return at<=a->data_size&&n<=a->data_size-at;}
static int scalar(const MeleeArchive* a,uint32_t at,float* out){return melee_archive_f32(a,at,out)&&isfinite(*out);}
void melee_title_free(MeleeTitle* t){
    if(!t)return;
    for(unsigned i=0;i<2;i++)melee_scene_free(t->models[i]);
    melee_environment_free(t->environment);free(t->pixels);free(t);
}
MeleeTitleData* melee_title_data(MeleeTitle* t){return t?&t->data:NULL;}
static int model(MeleeTitle* t,const MeleeArchive* a,unsigned i,const char* name){
    char symbol[80];uint32_t root;
    snprintf(symbol,sizeof(symbol),"%s_joint",name);
    if(!melee_archive_find(a,symbol,&root)||!(t->models[i]=melee_scene_decode(a,root)))return 0;
    MeleeScene* m=t->models[i];StaticModelDesc* d=&t->data.models[i];
    snprintf(symbol,sizeof(symbol),"%s_animjoint",name);
    if(!melee_archive_find(a,symbol,&root)||!melee_scene_bind_joints(m,root))return 0;
    snprintf(symbol,sizeof(symbol),"%s_matanim_joint",name);
    if(!melee_archive_find(a,symbol,&root)||!melee_scene_bind_materials(m,root))return 0;
    snprintf(symbol,sizeof(symbol),"%s_shapeanim_joint",name);
    if(!melee_archive_find(a,symbol,&root)||!melee_scene_bind_empty_shapes(m,root))return 0;
    d->joint=melee_scene_joint_descriptor(m);d->animjoint=melee_scene_animation_descriptor(m);
    d->matanim_joint=melee_scene_material_descriptor(m);d->shapeanim_joint=melee_scene_shape_descriptor(m);
    melee_scene_release_objects(m);return 1;
}
static int mark(MeleeTitle* t,const MeleeArchive* a,uint32_t root){
    uint32_t image,palette,pixels,dimensions,format,mipmap;float min,max;
    if(!range(a,root,8)||!ref(a,root,&image)||image==NONE||!ref(a,root+4,&palette)||palette!=NONE||
       !range(a,image,24)||!ref(a,image,&pixels)||pixels==NONE||
       !melee_archive_u32(a,image+4,&dimensions)||!melee_archive_u32(a,image+8,&format)||
       !melee_archive_u32(a,image+12,&mipmap)||mipmap||!scalar(a,image+16,&min)||min!=0||
       !scalar(a,image+20,&max)||max!=0)return 0;
    unsigned width=dimensions>>16,height=dimensions&65535;
    /* This export is a single direct-color/intensity image, not a palette. */
    if(format==8||format==9||format==10)return 0;
    size_t n=melee_texture_level_size(width,height,format);
    if(!n||!range(a,pixels,n)||!(t->pixels=malloc(n)))return 0;
    memcpy(t->pixels,a->bytes+32+pixels,n);
    t->image=(HSD_ImageDesc){.image_ptr=t->pixels,.width=width,.height=height,.format=format};
    t->data.mark.image=&t->image;return 1;
}
MeleeTitle* melee_title_decode(const MeleeArchive* a){
    if(!a)return NULL;MeleeTitle* t=calloc(1,sizeof(*t));if(!t)return NULL;uint32_t root;
    if(!model(t,a,0,"TtlMoji_Top")||!model(t,a,1,"TtlBg_Top"))goto fail;
    uint32_t camera,lights,fog;
    if(!melee_archive_find(a,"ScTitle_cam_int1_camera",&camera)||
       !melee_archive_find(a,"ScTitle_scene_lights",&lights)||
       !melee_archive_find(a,"ScTitle_fog",&fog)||
       !(t->environment=melee_environment_decode(a,camera,lights,fog)))goto fail;
    t->data.camera=melee_environment_camera(t->environment);
    t->data.lights=melee_environment_lights(t->environment);
    t->data.fog=*melee_environment_fog(t->environment);
    if(!melee_archive_find(a,"TitleMark_sobjdesc",&root)||!mark(t,a,root))goto fail;
    return t;
fail:melee_title_free(t);return NULL;
}
