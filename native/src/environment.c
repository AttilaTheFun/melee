#include "melee_environment.h"
#include "melee_camera.h"
#include "melee_lights.h"
#include "melee_animation.h"
#include "melee_scene.h"
#include <sysdolphin/baselib/wobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define LIMIT 32
#define NONE UINT32_MAX
typedef struct WorldAnimation {struct WorldAnimation* next;HSD_WObjAnim desc;HSD_AObjDesc aobj;MeleeAnimationTracks* tracks;MeleeScene* path;} WorldAnimation;
struct MeleeEnvironment {
    WorldAnimation* world_animations;
    u32 animation_offsets[LIMIT][LIMIT];
    struct { HSD_CObjDesc* camera;LightList** lights;HSD_FogDesc fog; } data;
    int has_fog;
    MeleeCamera* camera;
    MeleeLights* lights[LIMIT];
    LightList entries[LIMIT];LightList* list[LIMIT+1];
    HSD_AObjDesc light_aobjs[LIMIT][LIMIT];
    MeleeAnimationTracks* light_tracks[LIMIT][LIMIT];
    HSD_LightAnim empty[LIMIT][LIMIT];HSD_LightAnim* animations[LIMIT][LIMIT+1];
    HSD_FogAdjDesc adjustment;
};
static int ref(const MeleeArchive* a,uint32_t at,uint32_t* out){
    MeleeHostBool present;if(!melee_archive_pointer(a,at,out,&present))return 0;
    if(!present)*out=NONE;return 1;
}
static int range(const MeleeArchive* a,uint32_t at,size_t n){return at<=a->data_size&&n<=a->data_size-at;}
static int scalar(const MeleeArchive* a,uint32_t at,float* out){return melee_archive_f32(a,at,out)&&isfinite(*out);}
static HSD_WObjAnim* world_animation(MeleeEnvironment* t,const MeleeArchive* a,u32 at){
    u32 anim,robj;if(!range(a,at,8)||!ref(a,at,&anim)||!ref(a,at+4,&robj)||robj!=NONE)return NULL;
    WorldAnimation* w=calloc(1,sizeof(*w));if(!w)return NULL;
    w->next=t->world_animations;t->world_animations=w;
    if(anim!=NONE){
        u32 tracks,path;
        if(!range(a,anim,16)||!melee_archive_u32(a,anim,&w->aobj.flags)||
           !scalar(a,anim+4,&w->aobj.end_frame)||w->aobj.end_frame<0||
           !ref(a,anim+8,&tracks)||!ref(a,anim+12,&path))return NULL;
        if(tracks!=NONE){
            w->tracks=melee_animation_tracks_decode(a,tracks);if(!w->tracks)return NULL;
            w->aobj.fobjdesc=melee_animation_tracks_descriptors(w->tracks);
            for(HSD_FObjDesc* f=w->aobj.fobjdesc;f;f=f->next)
                if(f->type<4||f->type>7||(f->type==4&&path==NONE))return NULL;
        }
        if(path!=NONE){
            w->path=melee_scene_decode(a,path);if(!w->path)return NULL;
            HSD_Joint* joint=melee_scene_joint_descriptor(w->path);
            if(!(joint->flags&JOBJ_SPLINE))return NULL;
            w->aobj.obj_id=(HSD_IDKey)joint;melee_scene_release_objects(w->path);
        }
        w->desc.aobjdesc=&w->aobj;
    }
    return &w->desc;
}
static int lighting(MeleeEnvironment* t,const MeleeArchive* a,uint32_t root){
    t->data.lights=t->list;
    for(unsigned i=0;i<=LIMIT;i++){
        uint32_t entry,desc,anims;
        if((uint64_t)root+4*i+4>a->data_size||!ref(a,root+4*i,&entry))return 0;
        if(entry==NONE)return 1;
        if(i==LIMIT||!range(a,entry,8)||!ref(a,entry,&desc)||desc==NONE||!ref(a,entry+4,&anims))return 0;
        t->lights[i]=melee_lights_decode(a,desc);if(!t->lights[i])return 0;
        t->list[i]=&t->entries[i];t->entries[i].desc=melee_lights_descriptor(t->lights[i]);
        if(anims==NONE)continue;
        t->entries[i].anims=t->animations[i];
        for(unsigned k=0;k<=LIMIT;k++){
            uint32_t anim;
            if((uint64_t)anims+4*k+4>a->data_size||!ref(a,anims+4*k,&anim))return 0;
            if(anim==NONE)break;
            if(k==LIMIT||!range(a,anim,16))return 0;
            for(unsigned field=0;field<4;field++){
                uint32_t pointer;if(!ref(a,anim+4*field,&pointer))return 0;
                if(pointer==NONE)continue;
                if(field==0)return 0;
                if(field==1){
                    HSD_AObjDesc* desc=&t->light_aobjs[i][k];u32 tracks,path;
                    if(!range(a,pointer,16)||!melee_archive_u32(a,pointer,&desc->flags)||
                       !scalar(a,pointer+4,&desc->end_frame)||desc->end_frame<0||
                       !ref(a,pointer+8,&tracks)||!ref(a,pointer+12,&path)||path!=NONE)return 0;
                    if(tracks!=NONE){
                        t->light_tracks[i][k]=melee_animation_tracks_decode(a,tracks);if(!t->light_tracks[i][k])return 0;
                        desc->fobjdesc=melee_animation_tracks_descriptors(t->light_tracks[i][k]);
                        for(HSD_FObjDesc* f=desc->fobjdesc;f;f=f->next)if(f->type<9||f->type>12)return 0;
                    }
                    t->empty[i][k].aobjdesc=desc;continue;
                }
                HSD_WObjAnim* world=world_animation(t,a,pointer);if(!world)return 0;
                if(field==2)t->empty[i][k].position_anim=world;
                else t->empty[i][k].interest_anim=world;
            }
            t->animations[i][k]=&t->empty[i][k];t->animation_offsets[i][k]=anim;
        }
    }
    return 0;
}
static int fog(MeleeEnvironment* t,const MeleeArchive* a,uint32_t root){
    HSD_FogDesc* d=&t->data.fog;uint32_t adjust;
    if(!range(a,root,20)||!melee_archive_u32(a,root,&d->type)||!ref(a,root+4,&adjust)||
       !scalar(a,root+8,&d->start)||!scalar(a,root+12,&d->end)||d->end<d->start)return 0;
    switch(d->type){case 0:case 2:case 4:case 5:case 6:case 7:case 10:case 12:case 13:case 14:case 15:break;default:return 0;}
    memcpy(&d->color,a->bytes+32+root+16,4);
    if(adjust!=NONE){
        uint32_t dimensions;if(!range(a,adjust,68)||!melee_archive_u32(a,adjust,&dimensions))return 0;
        t->adjustment.center=dimensions>>16;t->adjustment.width=dimensions&65535;
        if(!t->adjustment.width)return 0;
        for(unsigned row=0;row<4;row++)for(unsigned col=0;col<4;col++)
            if(!scalar(a,adjust+4+16*row+4*col,&t->adjustment.mtx[row][col]))return 0;
        d->fogadjdesc=&t->adjustment;
    }
    return 1;
}

void melee_environment_free(MeleeEnvironment* t){
    if(!t)return;
    while(t->world_animations){WorldAnimation* w=t->world_animations;t->world_animations=w->next;melee_animation_tracks_free(w->tracks);melee_scene_free(w->path);free(w);}
    melee_camera_free(t->camera);
    for(unsigned i=0;i<LIMIT;i++){melee_lights_free(t->lights[i]);for(unsigned k=0;k<LIMIT;k++)melee_animation_tracks_free(t->light_tracks[i][k]);}
    free(t);
}
MeleeEnvironment* melee_environment_decode(const MeleeArchive* a,uint32_t camera,uint32_t lights,uint32_t fog_root){
    if(!a)return NULL;MeleeEnvironment* t=calloc(1,sizeof(*t));if(!t)return NULL;
    if(camera!=NONE){t->camera=melee_camera_decode(a,camera);if(!t->camera)goto fail;
        t->data.camera=melee_camera_descriptor(t->camera);}
    if(lights!=NONE&&!lighting(t,a,lights))goto fail;
    if(fog_root!=NONE){if(!fog(t,a,fog_root))goto fail;t->has_fog=1;}
    return t;
fail:melee_environment_free(t);return NULL;
}
HSD_CObjDesc* melee_environment_camera(MeleeEnvironment* t){return t?t->data.camera:NULL;}
LightList** melee_environment_lights(MeleeEnvironment* t){return t?t->data.lights:NULL;}
HSD_FogDesc* melee_environment_fog(MeleeEnvironment* t){return t&&t->has_fog?&t->data.fog:NULL;}

MeleeEnvironment* melee_environment_decode_camera_fog(const MeleeArchive* a,uint32_t camera,uint32_t fog_root){
    if(!a)return NULL;MeleeEnvironment* t=calloc(1,sizeof(*t));if(!t)return NULL;
    t->camera=melee_camera_decode(a,camera);if(!t->camera)goto fail;
    t->data.camera=melee_camera_descriptor(t->camera);
    if(!fog(t,a,fog_root))goto fail;t->has_fog=1;
    return t;
fail:melee_environment_free(t);return NULL;
}

HSD_LightDesc* melee_environment_find_light(MeleeEnvironment* t,uint32_t offset){
    if(t)for(unsigned i=0;i<LIMIT;i++){HSD_LightDesc* d=melee_lights_find(t->lights[i],offset);if(d)return d;}
    return NULL;
}

HSD_LightAnim* melee_environment_find_light_animation(MeleeEnvironment* t,u32 offset){
    if(!t)return NULL;
    for(unsigned i=0;i<LIMIT&&t->list[i];i++)
        for(unsigned k=0;k<LIMIT&&t->animations[i][k];k++)
            if(t->animation_offsets[i][k]==offset)return t->animations[i][k];
    return NULL;
}
