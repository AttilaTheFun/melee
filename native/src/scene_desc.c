#include "melee_scene_desc.h"
#include "melee_scene.h"
#include "melee_dynamic_model.h"
#include "melee_camera.h"
#include "melee_environment.h"
#include "melee_lights.h"
#include "melee_animation.h"
#include <math.h>
#include <string.h>
#include <melee/sc/types.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <stdlib.h>

#define LIMIT 32
#define NONE UINT32_MAX
struct MeleeSceneDesc {
    SceneDesc desc;
    MeleeEnvironment* fog_owner;
    MeleeEnvironment* light_environment;
    MeleeAnimationTracks* fog_tracks[LIMIT];
    HSD_AObjDesc fog_aobjs[LIMIT];
    HSD_CameraAnim fog_anims[LIMIT];
    struct SceneFogDesc fog;
    MeleeDynamicModel* model_owners[LIMIT];
    DynamicModelDesc* model_list[LIMIT+1];

    MeleeCamera* camera_owners[LIMIT];
    MeleeCameraAnimation* camera_anims[LIMIT][LIMIT];
    struct SceneCameraDesc cameras[LIMIT+1];
    void** allocations;
    size_t count;
};
static int ref(const MeleeArchive* a,uint32_t slot,uint32_t* out)
{
    MeleeHostBool present;
    if(!melee_archive_pointer(a,slot,out,&present))return 0;
    if(!present)*out=NONE;
    return 1;
}
static void* own(MeleeSceneDesc* s,size_t count,size_t size)
{
    if(!count||size>SIZE_MAX/count)return NULL;
    void* p=calloc(count,size);if(!p)return NULL;
    void** list=realloc(s->allocations,(s->count+1)*sizeof(*list));
    if(!list){free(p);return NULL;}
    s->allocations=list;list[s->count++]=p;return p;
}
void melee_scene_desc_free(MeleeSceneDesc* s)
{
    if(!s)return;
    melee_environment_free(s->fog_owner);
    melee_environment_free(s->light_environment);
    for(unsigned i=0;i<LIMIT;i++){
        melee_animation_tracks_free(s->fog_tracks[i]);
        melee_dynamic_model_free(s->model_owners[i]);
        melee_camera_free(s->camera_owners[i]);
        for(unsigned j=0;j<LIMIT;j++)melee_camera_animation_free(s->camera_anims[i][j]);
    }
    for(size_t i=0;i<s->count;i++)free(s->allocations[i]);
    free(s->allocations);free(s);
}
SceneDesc* melee_scene_desc_data(MeleeSceneDesc* s){return s?&s->desc:NULL;}
static MeleeSceneDesc* scene_desc_decode(const MeleeArchive* a,uint32_t root,int single_camera)
{
    uint32_t models,cameras,lights,fogs;
    if(!a||root>a->data_size||a->data_size-root<16||
       !ref(a,root,&models)||!ref(a,root+4,&cameras)||
       !ref(a,root+8,&lights)||!ref(a,root+12,&fogs))return NULL;
    MeleeSceneDesc* s=calloc(1,sizeof(*s));if(!s)return NULL;
    if(fogs!=NONE){
        u32 desc,anim;
        if(fogs>a->data_size||a->data_size-fogs<8||!ref(a,fogs,&desc)||desc==NONE||
           !ref(a,fogs+4,&anim))goto fail;
        s->fog_owner=melee_environment_decode(a,NONE,NONE,desc);
        if(!s->fog_owner)goto fail;
        s->fog.desc=melee_environment_fog(s->fog_owner);s->desc.fogs=&s->fog;
        if(anim!=NONE){
            HSD_CameraAnim** list=own(s,LIMIT+1,sizeof(*list));if(!list)goto fail;
            s->fog.anims=list;
            for(unsigned i=0;;i++){
                u32 entry,aobj,adjust,tracks,path,bits;
                if((uint64_t)anim+4*i>UINT32_MAX||!ref(a,anim+4*i,&entry))goto fail;
                if(entry==NONE)break;
                if(i==LIMIT||!ref(a,entry,&aobj)||!ref(a,entry+4,&adjust)||adjust!=NONE)goto fail;
                list[i]=&s->fog_anims[i];
                if(aobj==NONE)continue;
                HSD_AObjDesc* d=&s->fog_aobjs[i];
                if(!melee_archive_u32(a,aobj,&d->flags)||!melee_archive_u32(a,aobj+4,&bits)||
                   !ref(a,aobj+8,&tracks)||!ref(a,aobj+12,&path)||path!=NONE)goto fail;
                memcpy(&d->end_frame,&bits,4);if(!isfinite(d->end_frame)||d->end_frame<0)goto fail;
                if(tracks!=NONE){
                    s->fog_tracks[i]=melee_animation_tracks_decode(a,tracks);if(!s->fog_tracks[i])goto fail;
                    d->fobjdesc=melee_animation_tracks_descriptors(s->fog_tracks[i]);
                    for(HSD_FObjDesc* f=d->fobjdesc;f;f=f->next)if(f->type!=1&&f->type!=2&&(f->type<5||f->type>8)&&f->type!=20)goto fail;
                }
                list[i]->aobjdesc=d;
            }
        }
    }
    if(models!=NONE){
        s->desc.models=s->model_list;
        for(unsigned i=0;;i++){
            uint32_t at;
            if((uint64_t)models+4*i>UINT32_MAX||!ref(a,models+4*i,&at))goto fail;
            if(at==NONE)break;
            if(i==LIMIT)goto fail;
            s->model_owners[i]=melee_dynamic_model_decode_at(a,at);
            if(!s->model_owners[i])goto fail;
            s->model_list[i]=melee_dynamic_model_descriptor(s->model_owners[i]);
        }
    }
    if(cameras!=NONE){
        s->desc.cameras=s->cameras;
        for(unsigned i=0;;i++){
            /* Ending exports one camera record without an array terminator. */
            if(single_camera&&i==1)break;
            uint32_t at,anim;uint64_t slot=(uint64_t)cameras+8*i;
            if(slot+4>a->data_size||!ref(a,slot,&at))goto fail;
            if(at==NONE)break;
            if(slot+8>a->data_size||!ref(a,slot+4,&anim))goto fail;
            if(i==LIMIT)goto fail;
            s->camera_owners[i]=melee_camera_decode(a,at);if(!s->camera_owners[i])goto fail;
            s->cameras[i].desc=melee_camera_descriptor(s->camera_owners[i]);
            if(anim!=NONE){
                HSD_CameraAnim** list=own(s,LIMIT+1,sizeof(*list));if(!list)goto fail;
                s->cameras[i].anims=list;
                for(unsigned j=0;;j++){
                    u32 entry;
                    if((uint64_t)anim+4*j>UINT32_MAX||!ref(a,anim+4*j,&entry))goto fail;
                    if(entry==NONE)break;
                    if(j==LIMIT)goto fail;
                    s->camera_anims[i][j]=melee_camera_animation_decode(a,entry);
                    if(!s->camera_anims[i][j])goto fail;
                    list[j]=melee_camera_animation_descriptor(s->camera_anims[i][j]);
                }
            }
        }
    }
    if(lights!=NONE){
        s->light_environment=melee_environment_decode(a,NONE,lights,NONE);
        if(!s->light_environment)goto fail;
        s->desc.lights=melee_environment_lights(s->light_environment);
    }
    return s;
fail:
    melee_scene_desc_free(s);return NULL;
}

/* Scene archives retain serialized fighter demo motion DAT files. */
#include <sysdolphin/baselib/archive.h>
#include <string.h>
typedef struct {
    HSD_Archive bridge;
    char scene_name[4][128];
    unsigned scene_count;
    MeleeCamera* route_cameras[12];
    MeleeCameraAnimation* route_anims[12][LIMIT];
    HSD_CameraAnim* route_anim_lists[12][LIMIT+1];
    struct SceneCameraDesc route_descs[12];
    MeleeDynamicModel* staff_models[10];
    DynamicModelDesc* staff_list[11];
    MeleeArchive archive;
    u8* bytes;
    MeleeSceneDesc* scene[4];
} IntroArchive;
static void intro_destroy(HSD_Archive* bridge)
{
    if(!bridge)return;
    IntroArchive* owner=bridge->top_ptr;
    for(unsigned i=0;i<owner->scene_count;i++)melee_scene_desc_free(owner->scene[i]);
    for(unsigned i=0;i<10;i++)melee_dynamic_model_free(owner->staff_models[i]);
    for(unsigned i=0;i<12;i++){
        melee_camera_free(owner->route_cameras[i]);
        for(unsigned j=0;j<LIMIT;j++)melee_camera_animation_free(owner->route_anims[i][j]);
    }
    free(owner->bytes);free(owner);
}
static void* intro_lookup(HSD_Archive* bridge,const char* name)
{
    IntroArchive* owner=bridge->top_ptr;
    if(!name)return NULL;
    for(unsigned i=0;i<12;i++){
        char symbol[5]={'m','c','0'+(i+1)/10,'0'+(i+1)%10,0};
        if(owner->route_cameras[i]&&!strcmp(name,symbol))return &owner->route_descs[i];
    }
    if(owner->staff_models[0]&&!strcmp(name,"ScGamRegStaffrollNames_scene_modelset"))return owner->staff_list;
    for(unsigned i=0;i<owner->scene_count;i++)if(!strcmp(name,owner->scene_name[i]))return melee_scene_desc_data(owner->scene[i]);
    if(strncmp(name,"ftDemoResultMotionFile",22)&&strncmp(name,"ftDemoIntroMotionFile",21)&&strncmp(name,"ftDemoEndingMotionFile",22)&&strncmp(name,"ftDemoVi",8))return NULL;
    u32 root,length;
    if(!melee_archive_find(&owner->archive,name,&root)||
       !melee_archive_u32(&owner->archive,root,&length)||
       root>owner->archive.data_size||length>owner->archive.data_size-root)return NULL;
    MeleeArchive nested;
    u8* bytes=owner->bytes+32+root;
    if(!melee_archive_open(&nested,bytes,length))return NULL;
    return bytes;
}
static HSD_Archive* scene_archive_decode(const MeleeArchive* archive,const char* const* names,unsigned count,int single_camera)
{
    if(!archive||archive->extern_count||(count&&!names)||count>4)return NULL;
    for(unsigned i=0;i<count;i++)if(!names[i]||strlen(names[i])>=128)return NULL;
    IntroArchive* owner=calloc(1,sizeof(*owner));if(!owner)return NULL;
    owner->bridge.top_ptr=owner;
    owner->scene_count=count;
    for(unsigned i=0;i<count;i++)strcpy(owner->scene_name[i],names[i]);
    owner->bytes=malloc(archive->size);if(!owner->bytes)goto fail;
    memcpy(owner->bytes,archive->bytes,archive->size);
    if(!melee_archive_open(&owner->archive,owner->bytes,archive->size))goto fail;
    u32 root;
    for(unsigned i=0;i<count;i++)if(!melee_archive_find(&owner->archive,names[i],&root)||
       !(owner->scene[i]=scene_desc_decode(&owner->archive,root,single_camera)))goto fail;
    owner->bridge.flags=HSD_ARCHIVE_NATIVE;
    owner->bridge.native_public_lookup=intro_lookup;
    owner->bridge.native_destroy=intro_destroy;
    return &owner->bridge;
fail:intro_destroy(&owner->bridge);return NULL;
}

const void* melee_intro_motion_bytes(HSD_Archive* bridge,const char* name,size_t* size)
{
    if(!bridge||bridge->native_public_lookup!=intro_lookup||!size)return NULL;
    IntroArchive* owner=bridge->top_ptr;
    u32 root;
    if(!intro_lookup(bridge,name)||!melee_archive_find(&owner->archive,name,&root))return NULL;
    u32 end=owner->archive.data_size;
    for(unsigned i=0;i<owner->archive.public_count;i++){
        const char* symbol;u32 next;
        if(!melee_archive_public(&owner->archive,i,&symbol,&next))return NULL;
        if(next>root&&next<end)end=next;
    }
    *size=end-root;
    return owner->bytes+32+root;
}

HSD_Archive* melee_cutscene_decode(const MeleeArchive* a)
{
    if(!a)return NULL;
    const char* names[4];unsigned count=0;
    for(unsigned i=0;i<a->public_count;i++){
        const char* name;u32 root;if(!melee_archive_public(a,i,&name,&root))return NULL;
        size_t n=strlen(name);
        if(!strncmp(name,"visual",6)&&n>=5&&!strcmp(name+n-5,"Scene")){
            if(count==4)return NULL;names[count++]=name;
        }
    }
    return count?scene_archive_decode(a,names,count,0):NULL;
}
static HSD_Archive* demo_motion_decode(const MeleeArchive* a,const char* prefix)
{
    if(!a||a->public_count!=1)return NULL;
    const char* name;u32 root;
    if(!melee_archive_public(a,0,&name,&root)||strncmp(name,prefix,strlen(prefix)))return NULL;
    HSD_Archive* bridge=scene_archive_decode(a,NULL,0,0);if(!bridge)return NULL;
    if(!intro_lookup(bridge,name)){intro_destroy(bridge);return NULL;}
    return bridge;
}
HSD_Archive* melee_demo_wait_decode(const MeleeArchive* a)
{return demo_motion_decode(a,"ftDemoViWaitMotionFile");}
HSD_Archive* melee_demo_result_decode(const MeleeArchive* a)
{return demo_motion_decode(a,"ftDemoResultMotionFile");}

HSD_Archive* melee_adventure_intro_decode(const MeleeArchive* a)
{
    const char* name="ScItrNormal_scene_data";
    HSD_Archive* bridge=scene_archive_decode(a,&name,1,0);if(!bridge)return NULL;
    IntroArchive* owner=bridge->top_ptr;
    for(unsigned i=0;i<12;i++){
        char symbol[5]={'m','c','0'+(i+1)/10,'0'+(i+1)%10,0};
        u32 root,camera,anims;
        if(!melee_archive_find(a,symbol,&root)||!ref(a,root,&camera)||camera==NONE||!ref(a,root+4,&anims))goto fail;
        owner->route_cameras[i]=melee_camera_decode(a,camera);if(!owner->route_cameras[i])goto fail;
        owner->route_descs[i].desc=melee_camera_descriptor(owner->route_cameras[i]);
        if(anims==NONE)continue;
        owner->route_descs[i].anims=owner->route_anim_lists[i];
        for(unsigned j=0;;j++){
            u32 entry;if(!ref(a,anims+4*j,&entry))goto fail;
            if(entry==NONE)break;
            if(j==LIMIT)goto fail;
            owner->route_anims[i][j]=melee_camera_animation_decode(a,entry);if(!owner->route_anims[i][j])goto fail;
            owner->route_anim_lists[i][j]=melee_camera_animation_descriptor(owner->route_anims[i][j]);
        }
    }
    return bridge;
fail:intro_destroy(bridge);return NULL;
}

HSD_Archive* melee_intro_decode(const MeleeArchive* a){return melee_single_scene_decode(a,"ScItrAllstar_scene_data");}

HSD_Archive* melee_single_scene_decode(const MeleeArchive* a,const char* name)
{return scene_archive_decode(a,&name,1,0);}

typedef struct {
    HSD_Archive bridge;
    MeleeDynamicModel* owner;
    DynamicModelDesc* models[2];
    const char* symbol;
} TrainingArchive;
static void training_destroy(HSD_Archive* bridge)
{
    TrainingArchive* s=bridge->top_ptr;
    melee_dynamic_model_free(s->owner);free(s);
}
static void* training_lookup(HSD_Archive* bridge,const char* name)
{
    TrainingArchive* s=bridge->top_ptr;
    return !strcmp(name,s->symbol)?s->models:NULL;
}
static HSD_Archive* single_model_list_decode(const MeleeArchive* a,const char* symbol)
{
    u32 root,model,end;
    if(!a||!melee_archive_find(a,symbol,&root)||
       !ref(a,root,&model)||model==NONE||!ref(a,root+4,&end)||end!=NONE)return NULL;
    TrainingArchive* s=calloc(1,sizeof(*s));if(!s)return NULL;
    s->bridge.top_ptr=s;s->symbol=symbol;
    s->owner=melee_dynamic_model_decode_at(a,model);
    if(!s->owner){training_destroy(&s->bridge);return NULL;}
    s->models[0]=melee_dynamic_model_descriptor(s->owner);
    s->bridge.flags=HSD_ARCHIVE_NATIVE;
    s->bridge.native_public_lookup=training_lookup;
    s->bridge.native_destroy=training_destroy;
    return &s->bridge;
}
HSD_Archive* melee_training_decode(const MeleeArchive* a)
{return single_model_list_decode(a,"ScGamTraining_scene_models");}
HSD_Archive* melee_homerun_hud_decode(const MeleeArchive* a)
{return single_model_list_decode(a,"ScInfCnt_scene_models");}

HSD_Archive* melee_ending_decode(const MeleeArchive* a)
{
    static const char* names[]={"cut1CanimScene","cut2CanimScene","cut3CanimScene","cut3BgScene"};
    return scene_archive_decode(a,names,4,1);
}

MeleeSceneDesc* melee_scene_desc_decode(const MeleeArchive* a,uint32_t root)
{return scene_desc_decode(a,root,0);}

HSD_Archive* melee_staffroll_decode(const MeleeArchive* a)
{
    const char* name="ScGamRegStaffroll_scene_data";
    HSD_Archive* bridge=scene_archive_decode(a,&name,1,1);if(!bridge)return NULL;
    IntroArchive* owner=bridge->top_ptr;u32 root,entry;
    if(!melee_archive_find(&owner->archive,"ScGamRegStaffrollNames_scene_modelset",&root))goto fail;
    for(unsigned i=0;i<10;i++){
        if(!ref(&owner->archive,root+4*i,&entry)||entry==NONE)goto fail;
        owner->staff_models[i]=melee_dynamic_model_decode_at(&owner->archive,entry);
        if(!owner->staff_models[i])goto fail;
        owner->staff_list[i]=melee_dynamic_model_descriptor(owner->staff_models[i]);
    }
    return bridge;
fail:intro_destroy(bridge);return NULL;
}

HSD_Archive* melee_approach_decode(const MeleeArchive* a)
{
    const char* name="ScNtcApproach_scene_data";
    return scene_archive_decode(a,&name,1,1);
}
