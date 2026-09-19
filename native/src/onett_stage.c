#include "melee_onett_stage.h"
#include "melee_stage_models.h"
#include "melee_stage_collision.h"
#include "melee_stage_params.h"
#include "melee_onett_params.h"
#include "melee_dynamic_model.h"
#include "melee_item_scripts.h"
#include "melee_particle_bank.h"
#include "melee_environment.h"
#include <sysdolphin/baselib/particle.h>
#include <sysdolphin/baselib/debug.h>
#include <stdlib.h>
#include <string.h>
#define NONE UINT32_MAX
typedef struct {
    HSD_Archive bridge;
    MeleeStageModels* models;UnkStageDat* header;
    MapCollData* collision;GroundParam* params;struct grOnett_StageParam hazards;
    MeleeDynamicModel* quake;MeleeItemScripts* scripts;MeleeParticleBank* particles;
    MeleeEnvironment* lighting;void* empty_items[1];
} OnettStage;
static void destroy(HSD_Archive* b){
    if(!b)return;OnettStage* s=b->top_ptr;
    melee_environment_free(s->lighting);melee_particle_bank_free(s->particles);melee_item_scripts_free(s->scripts);
    melee_dynamic_model_free(s->quake);melee_stage_params_free(s->params);melee_stage_collision_free(s->collision);melee_stage_models_free(s->models);free(s);
}
static void* lookup(HSD_Archive* b,const char* name){
    OnettStage* s=b->top_ptr;if(!name)return NULL;
    if(!strcmp(name,"map_head"))return s->header;
    if(!strcmp(name,"coll_data"))return s->collision;
    if(!strcmp(name,"grGroundParam"))return s->params;
    if(!strcmp(name,"yakumono_param"))return &s->hazards;
    if(!strcmp(name,"itemdata"))return s->empty_items;
    if(!strcmp(name,"ALDYakuAll"))return melee_item_scripts_table(s->scripts);
    if(!strcmp(name,"quake_model_set"))return melee_dynamic_model_descriptor(s->quake);
    if(!strcmp(name,"map_plit"))return melee_environment_lights(s->lighting);
    /* Opaque non-NULL markers; bank registration uses the native owner API. */
    if(!strcmp(name,"map_ptcl")||!strcmp(name,"map_texg"))return s->particles;
    return NULL;
}
HSD_Archive* melee_onett_stage_decode(const MeleeArchive* a){
    if(!a)return NULL;OnettStage* s=calloc(1,sizeof(*s));if(!s)return NULL;s->bridge.top_ptr=s;
    u32 item,target,cmd,tex,light;MeleeHostBool present;
    if(!melee_archive_find(a,"itemdata",&item)||!melee_archive_pointer(a,item,&target,&present)||present)goto fail;
    s->models=melee_stage_models_decode(a);if(!s->models||!(s->header=melee_stage_models_header(s->models)))goto fail;
    s->collision=melee_stage_collision_decode(a);s->params=melee_stage_params_decode(a);
    if(!s->collision||!s->params||!melee_onett_params_decode(a,&s->hazards))goto fail;
    s->quake=melee_dynamic_model_decode(a,"quake_model_set");s->scripts=melee_item_scripts_decode(a);
    if(!s->quake||!s->scripts)goto fail;
    if(!melee_archive_find(a,"map_ptcl",&cmd)||!melee_archive_find(a,"map_texg",&tex)||cmd>=tex)goto fail;
    s->particles=melee_particle_bank_decode(a,cmd,tex-cmd,tex,a->data_size-tex);if(!s->particles)goto fail;
    if(!melee_archive_find(a,"map_plit",&light))goto fail;
    s->lighting=melee_environment_decode(a,NONE,light,NONE);if(!s->lighting)goto fail;
    LightList** list=melee_environment_lights(s->lighting);
    for(unsigned i=0;list&&list[i];i++){
        u32 entry,desc;if(!melee_archive_pointer(a,light+4*i,&entry,&present)||!present||!melee_archive_pointer(a,entry,&desc,&present)||!present)goto fail;
        HSD_LightDesc* canonical=melee_stage_models_find_light(s->models,desc);if(!canonical)goto fail;
        list[i]->desc=canonical;
    }
    s->bridge.flags=HSD_ARCHIVE_NATIVE;s->bridge.native_public_lookup=lookup;s->bridge.native_destroy=destroy;return &s->bridge;
fail:destroy(&s->bridge);return NULL;
}
void melee_onett_stage_register_particles(HSD_Archive* b,int bank){
    HSD_ASSERT(__LINE__,b&&b->native_public_lookup==lookup);OnettStage* s=b->top_ptr;
    psInitDataBankNative(bank,melee_particle_bank_commands(s->particles),melee_particle_bank_command_count(s->particles),
        melee_particle_bank_textures(s->particles),melee_particle_bank_texture_count(s->particles));
}
