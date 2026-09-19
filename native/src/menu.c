#include "melee_menu.h"
#include "melee_scene.h"
#include "melee_environment.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define MODELS 64
struct Model { char name[96];MeleeScene* owner; };
typedef struct {
    HSD_Archive bridge;
    int trophy;
    MeleeArchive archive;
    unsigned char* bytes;
    MeleeEnvironment* environment;
    struct Model models[MODELS];unsigned count;
} Menu;
static void destroy(HSD_Archive* bridge){
    if(!bridge)return;Menu* m=bridge->top_ptr;
    for(unsigned i=0;i<m->count;i++)melee_scene_free(m->models[i].owner);
    melee_environment_free(m->environment);
    free(bridge->public_info);free(m->bytes);free(m);
}
static MeleeScene* model(Menu* m,struct Model* entry){
    if(entry->owner)return entry->owner;
    char symbol[128];uint32_t root;
    snprintf(symbol,sizeof(symbol),"%s_joint",entry->name);
    if(!melee_archive_find(&m->archive,symbol,&root))return NULL;
    MeleeScene* scene=melee_scene_decode(&m->archive,root);if(!scene)return NULL;
    const char* suffixes[]={"animjoint","matanim_joint","shapeanim_joint"};
    for(unsigned i=0;i<3;i++){
        snprintf(symbol,sizeof(symbol),"%s_%s",entry->name,suffixes[i]);
        if(!melee_archive_find(&m->archive,symbol,&root)){if(m->trophy)continue;goto fail;}
        if(!(i==0?melee_scene_bind_joints(scene,root):i==1?
             melee_scene_bind_materials(scene,root):melee_scene_bind_empty_shapes(scene,root)))goto fail;
    }
    melee_scene_release_objects(scene);entry->owner=scene;return scene;
fail:melee_scene_free(scene);return NULL;
}
static void* lookup(HSD_Archive* bridge,const char* symbol){
    Menu* m=bridge->top_ptr;if(!symbol)return NULL;
    if(!strcmp(symbol,"ScMenMain_cam_int1_camera"))return melee_environment_camera(m->environment);
    if(!strcmp(symbol,"ScMenMain_scene_lights"))return melee_environment_lights(m->environment);
    if(!strcmp(symbol,"ScMenMain_fog"))return melee_environment_fog(m->environment);
    const char* suffixes[]={"_joint","_animjoint","_matanim_joint","_shapeanim_joint"};
    for(unsigned i=0;i<m->count;i++){
        size_t n=strlen(m->models[i].name);
        if(strncmp(symbol,m->models[i].name,n))continue;
        for(unsigned k=0;k<4;k++)if(!strcmp(symbol+n,suffixes[k])){
            MeleeScene* scene=model(m,&m->models[i]);if(!scene)return NULL;
            switch(k){case 0:return melee_scene_joint_descriptor(scene);
                case 1:return melee_scene_animation_descriptor(scene);
                case 2:return melee_scene_material_descriptor(scene);
                default:return melee_scene_shape_descriptor(scene);}
        }
    }
    return NULL;
}
static HSD_Archive* decode(const MeleeArchive* archive,int trophy){
    if(!archive||archive->extern_count||archive->public_count>4096)return NULL;
    Menu* m=calloc(1,sizeof(*m));if(!m)return NULL;
    m->bridge.top_ptr=m;m->trophy=trophy;
    m->bytes=malloc(archive->size);if(!m->bytes)goto fail;
    memcpy(m->bytes,archive->bytes,archive->size);
    if(!melee_archive_open(&m->archive,m->bytes,archive->size))goto fail;
    uint32_t camera,lights,fog;
    if(!trophy&&(!melee_archive_find(&m->archive,"ScMenMain_cam_int1_camera",&camera)||
       !melee_archive_find(&m->archive,"ScMenMain_scene_lights",&lights)||
       !melee_archive_find(&m->archive,"ScMenMain_fog",&fog)||
       !(m->environment=melee_environment_decode(&m->archive,camera,lights,fog))))goto fail;
    m->bridge.public_info=calloc(archive->public_count,sizeof(*m->bridge.public_info));
    if(!m->bridge.public_info)goto fail;
    m->bridge.symbols=(char*)m->bytes+m->archive.strings_start;
    for(unsigned i=0;i<archive->public_count;i++){
        const char* name;uint32_t root;
        if(!melee_archive_public(&m->archive,i,&name,&root))goto fail;
        m->bridge.public_info[i].offset=root;
        m->bridge.public_info[i].symbol=name-m->bridge.symbols;
        size_t n=strlen(name);
        if((n>10&&!strcmp(name+n-10,"_Top_joint"))||(trophy&&n>11&&!strcmp(name+n-11,"_TopN_joint"))){
            if(m->count==MODELS||n-6>=sizeof(m->models[0].name))goto fail;
            memcpy(m->models[m->count++].name,name,n-6);
        }
    }
    if(!m->count)goto fail;
    m->bridge.header.file_size=archive->size;m->bridge.header.data_size=archive->data_size;
    m->bridge.header.nb_public=archive->public_count;
    m->bridge.flags=HSD_ARCHIVE_NATIVE;
    m->bridge.native_public_lookup=lookup;m->bridge.native_destroy=destroy;
    return &m->bridge;
fail:destroy(&m->bridge);return NULL;
}

HSD_Archive* melee_menu_decode(const MeleeArchive* a){return decode(a,0);}
HSD_Archive* melee_trophy_decode(const MeleeArchive* a){return decode(a,1);}

typedef struct {HSD_Archive bridge;unsigned char* tables[2];} TrophyFiles;
static void trophy_files_destroy(HSD_Archive* bridge){if(bridge){TrophyFiles* o=bridge->top_ptr;free(o->tables[0]);free(o->tables[1]);free(o);}}
static void* trophy_files_lookup(HSD_Archive* bridge,const char* name){TrophyFiles* o=bridge->top_ptr;if(!strcmp(name,"tyModelFileTbl"))return o->tables[0];if(!strcmp(name,"tyModelFileUsTbl"))return o->tables[1];return NULL;}
HSD_Archive* melee_trophy_files_decode(const MeleeArchive* a)
{
    if(!a||a->reloc_count||a->extern_count)return NULL;
    TrophyFiles* o=calloc(1,sizeof(*o));if(!o)return NULL;o->bridge.top_ptr=o;
    const char* names[]={"tyModelFileTbl","tyModelFileUsTbl"};const unsigned counts[]={293,5};
    for(unsigned i=0;i<2;i++){
        u32 at;if(!melee_archive_find(a,names[i],&at)||at>a->data_size||84*counts[i]>a->data_size-at)goto fail;
        o->tables[i]=malloc(84*counts[i]);if(!o->tables[i])goto fail;
        memcpy(o->tables[i],a->bytes+32+at,84*counts[i]);
        for(unsigned j=0;j<counts[i];j++){
            u32 id;unsigned char* row=o->tables[i]+84*j;
            if(!melee_archive_u32(a,at+84*j,&id)||id>=293||!memchr(row+4,0,32)||!memchr(row+36,0,48))goto fail;
            memcpy(row,&id,4);
        }
    }
    o->bridge.flags=HSD_ARCHIVE_NATIVE;o->bridge.native_public_lookup=trophy_files_lookup;o->bridge.native_destroy=trophy_files_destroy;return &o->bridge;
fail:trophy_files_destroy(&o->bridge);return NULL;
}

MeleeHostBool melee_trophy_files_contains(HSD_Archive* bridge,const char* filename)
{
    if(!bridge||bridge->native_public_lookup!=trophy_files_lookup||!filename)return false;
    TrophyFiles* o=bridge->top_ptr;const unsigned counts[]={293,5};
    for(unsigned i=0;i<2;i++)for(unsigned j=0;j<counts[i];j++)
        if(!strcmp((char*)o->tables[i]+84*j+4,filename))return true;
    return false;
}
