#include "melee_character_select.h"
#include "melee_scene.h"
#include "melee_environment.h"
#include "melee_lights.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define NONE UINT32_MAX
typedef struct {
    HSD_Archive bridge;
    union { MeleeCSSData css;MeleeStageSelectionData stage; } data;
    const char* symbol;
    unsigned count;
    MeleeScene* scenes[12];
    MeleeEnvironment* environment;
    MeleeLights* lights[2];
} CharacterSelect;
static void destroy(HSD_Archive* bridge){
    if(!bridge)return;CharacterSelect* c=bridge->top_ptr;
    for(unsigned i=0;i<12;i++)melee_scene_free(c->scenes[i]);
    for(unsigned i=0;i<2;i++)melee_lights_free(c->lights[i]);
    melee_environment_free(c->environment);free(c);
}
static void* lookup(HSD_Archive* bridge,const char* name){
    CharacterSelect* c=bridge->top_ptr;
    return name&&!strcmp(name,c->symbol)?(c->count==9?(void*)&c->data.css:(void*)&c->data.stage):NULL;
}
static int pointer(const MeleeArchive* a,uint32_t slot,uint32_t* value){
    MeleeHostBool present;if(!melee_archive_pointer(a,slot,value,&present))return 0;
    if(!present)*value=NONE;return 1;
}
static HSD_Archive* decode(const MeleeArchive* a,const char* symbol,unsigned count){
    uint32_t root,refs[52];
    if(!a||a->extern_count||!melee_archive_find(a,symbol,&root)||
       root>a->data_size||a->data_size-root<16+16*count)return NULL;
    for(unsigned i=0;i<4+4*count;i++)if(!pointer(a,root+4*i,&refs[i]))return NULL;
    for(unsigned i=0;i<4;i++)if(refs[i]==NONE)return NULL;
    CharacterSelect* c=calloc(1,sizeof(*c));if(!c)return NULL;c->bridge.top_ptr=c;c->symbol=symbol;c->count=count;
    MeleeCSSModels* models=count==9?&c->data.css.models:&c->data.stage.models;
    MeleeCSSAnimation* animations=count==9?c->data.css.animations:c->data.stage.animations;
    c->environment=melee_environment_decode_camera_fog(a,refs[0],refs[3]);if(!c->environment){fprintf(stderr,"Selection environment failed\n");goto fail;}
    models->cam=melee_environment_camera(c->environment);
    models->fog=melee_environment_fog(c->environment);
    for(unsigned i=0;i<2;i++){
        c->lights[i]=melee_lights_decode(a,refs[1+i]);if(!c->lights[i]){fprintf(stderr,"Selection light %u failed\n",i);goto fail;}
        if(i==0)models->light0=melee_lights_descriptor(c->lights[i]);
        else models->light1=melee_lights_descriptor(c->lights[i]);
    }
    for(unsigned i=0;i<count;i++){
        uint32_t* r=refs+4+4*i;if(r[0]==NONE)goto fail;
        MeleeScene* s=c->scenes[i]=melee_scene_decode(a,r[0]);if(!s){fprintf(stderr,"Selection model %u failed\n",i);goto fail;}
        if(r[1]!=NONE&&!melee_scene_bind_joints(s,r[1])){fprintf(stderr,"Selection joint animation %u failed\n",i);goto fail;}
        if(r[2]!=NONE&&!melee_scene_bind_materials(s,r[2])){fprintf(stderr,"Selection material animation %u failed\n",i);goto fail;}
        if(r[3]!=NONE&&!melee_scene_bind_shapes(s,r[3])){fprintf(stderr,"Selection shape animation %u failed\n",i);goto fail;}
        animations[i]=(MeleeCSSAnimation){melee_scene_joint_descriptor(s),
            melee_scene_animation_descriptor(s),melee_scene_material_descriptor(s),melee_scene_shape_descriptor(s)};
        melee_scene_release_objects(s);
    }
    c->bridge.flags=HSD_ARCHIVE_NATIVE;c->bridge.native_destroy=destroy;c->bridge.native_public_lookup=lookup;
    return &c->bridge;
fail:destroy(&c->bridge);return NULL;
}

HSD_Archive* melee_character_select_decode(const MeleeArchive* a){return decode(a,"MnSelectChrDataTable",9);}
HSD_Archive* melee_stage_selection_decode(const MeleeArchive* a){return decode(a,"MnSelectStageDataTable",12);}
