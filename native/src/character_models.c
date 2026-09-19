#include "melee_character_models.h"
#include <math.h>
#include <stdlib.h>
struct MeleeCharacterModels {MeleeScene* scenes[2];struct ftData_x20 guard;};
void melee_character_models_free(MeleeCharacterModels* o){if(o){for(unsigned i=0;i<2;i++)melee_scene_free(o->scenes[i]);free(o);}}
void melee_character_models_bind(MeleeCharacterModels* o,ftData* d){if(o&&d){d->x20=&o->guard;d->x5C=melee_scene_joint_descriptor(o->scenes[1]);}}
static int ref(const MeleeArchive*a,u32 at,u32* target){MeleeHostBool p;return melee_archive_pointer(a,at,target,&p)&&p;}
MeleeCharacterModels* melee_character_models_decode(const MeleeArchive*a,u32 root)
{
 u32 guard,models[2];float scale;MeleeHostBool guard_model,aux_model;
 if(!a||!ref(a,root+0x20,&guard)||!melee_archive_pointer(a,guard,&models[0],&guard_model)||!melee_archive_f32(a,guard+4,&scale)||!isfinite(scale)||!melee_archive_pointer(a,root+0x5c,&models[1],&aux_model))return NULL;
 MeleeCharacterModels*o=calloc(1,sizeof(*o));if(!o)return NULL;o->guard.x8=scale;
 /* Yoshi uses his fighter model for guarding; his shield joint is null. */
 for(unsigned i=0;i<2;i++){if((i==0&&!guard_model)||(i==1&&!aux_model))continue;o->scenes[i]=melee_scene_decode(a,models[i]);if(!o->scenes[i])goto fail;melee_scene_release_objects(o->scenes[i]);}
 o->guard.x0=melee_scene_joint_descriptor(o->scenes[0]);return o;
fail:melee_character_models_free(o);return NULL;
}
