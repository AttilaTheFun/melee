#include "melee_hud.h"
#include "melee_dynamic_model.h"
#include "melee_scene_desc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define TABLES 10
#define MODELS 64
static const char* names[TABLES]={"DmgMrk_scene_models","DmgNum_scene_models","ScInfCnt_scene_models","ScInfPnm_scene_models","ScInfStc_scene_models","ScInfTim_scene_models","Stc_rarwmdls","Stc_scemdls","lupe","tdsce"};
/* The original HUD indexes all eight countdown slots and two stock models. */
static const unsigned model_counts[TABLES]={1,1,8,1,1,1,1,2,1,1};
typedef struct {HSD_Archive bridge;MeleeSceneDesc* scene;MeleeDynamicModel* owners[TABLES][MODELS];DynamicModelDesc* models[TABLES][MODELS+1];} Hud;
static void destroy(HSD_Archive* b){if(!b)return;Hud* h=b->top_ptr;melee_scene_desc_free(h->scene);for(unsigned i=0;i<TABLES;i++)for(unsigned j=0;j<MODELS;j++)melee_dynamic_model_free(h->owners[i][j]);free(h);}
static void* lookup(HSD_Archive* b,const char* name){if(!name)return NULL;Hud* h=b->top_ptr;if(!strcmp(name,"ScInfDmg_scene_data"))return melee_scene_desc_data(h->scene);for(unsigned i=0;i<TABLES;i++)if(!strcmp(name,names[i]))return h->models[i];return NULL;}
HSD_Archive* melee_hud_decode(const MeleeArchive* a){
 if(!a||a->extern_count)return NULL;Hud* h=calloc(1,sizeof(*h));if(!h)return NULL;h->bridge.top_ptr=h;
 u32 root;if(!melee_archive_find(a,"ScInfDmg_scene_data",&root)||!(h->scene=melee_scene_desc_decode(a,root))){fprintf(stderr,"HUD damage scene decode failed\n");goto fail;}
 for(unsigned i=0;i<TABLES;i++){
  if(!melee_archive_find(a,names[i],&root))goto fail;
  for(unsigned j=0;;j++){
   u32 model;MeleeHostBool p;if((uint64_t)root+4*j+4>a->data_size||!melee_archive_pointer(a,root+4*j,&model,&p))goto fail;
   if(!p){if(j!=model_counts[i])goto fail;break;}if(j>=model_counts[i]||j==MODELS)goto fail;
   h->owners[i][j]=melee_dynamic_model_decode_at(a,model);
   if(!h->owners[i][j]){fprintf(stderr,"HUD %s model %u decode failed\n",names[i],j);goto fail;}
   h->models[i][j]=melee_dynamic_model_descriptor(h->owners[i][j]);
  }
 }
 h->bridge.flags=HSD_ARCHIVE_NATIVE;h->bridge.native_public_lookup=lookup;h->bridge.native_destroy=destroy;return &h->bridge;
fail:destroy(&h->bridge);return NULL;
}
