#include "melee_donkey_data.h"
#include "melee_character_attributes.h"
#include "melee_character_aux.h"
#include "melee_character_collision.h"
#include "melee_character_dynamics.h"
#include "melee_character_models.h"
#include "melee_character_motions.h"
#include "melee_character_part_anims.h"
#include "melee_character_parts.h"
#include "melee_character_sounds.h"
#include "melee_character_wait.h"
#include <stdlib.h>
#include <stdio.h>
struct MeleeDonkeyData {
 ftData header;ftCo_DatAttrs common;ftDonkeyAttributes special;
 MeleeCharacterAux* aux;MeleeCharacterCollision* collision;MeleeCharacterDynamics* dynamics;
 MeleeCharacterModels* models;MeleeCharacterMotions* motions;MeleeCharacterPartAnims* part_anims;
 MeleeCharacterParts* parts;MeleeCharacterSounds* sounds;MeleeCharacterWait* wait;
};
ftData* melee_donkey_data_header(MeleeDonkeyData*o){return o?&o->header:NULL;}
void melee_donkey_data_free(MeleeDonkeyData*o){if(!o)return;melee_character_aux_free(o->aux);melee_character_collision_free(o->collision);melee_character_dynamics_free(o->dynamics);melee_character_models_free(o->models);melee_character_motions_free(o->motions);melee_character_part_anims_free(o->part_anims);melee_character_parts_free(o->parts);melee_character_sounds_free(o->sounds);melee_character_wait_free(o->wait);free(o);}
MeleeDonkeyData* melee_donkey_data_decode(const MeleeArchive*a,const void*aj,size_t size)
{
 u32 root;if(!a||!melee_archive_find(a,"ftDataDonkey",&root))return NULL;
 MeleeDonkeyData*o=calloc(1,sizeof(*o));if(!o)return NULL;const unsigned counts[]={4,4,3};
 if(!melee_character_attributes_decode(a,root,&o->common)||!melee_donkey_attributes_decode(a,root,&o->special))goto fail;
 u32 unused_items;MeleeHostBool items_present;
 if(!melee_archive_pointer(a,root+0x48,&unused_items,&items_present)||items_present)goto fail;
 o->header.x0=&o->common;o->header.ext_attr=&o->special;
 if(!(o->aux=melee_character_aux_decode(a,root))||!(o->collision=melee_character_collision_decode(a,root,140))||
    !(o->dynamics=melee_character_dynamics_decode(a,root,140,337,14))||!(o->models=melee_character_models_decode(a,root))||
    !(o->motions=melee_character_motions_decode(a,root,337,aj,size))||!(o->part_anims=melee_character_part_anims_decode(a,root,140,counts,3))||
    !(o->parts=melee_character_parts_decode(a,root,5,140))||!(o->sounds=melee_character_sounds_decode(a,root))||
    !(o->wait=melee_character_wait_decode(a,root,337)))goto fail;
 melee_character_aux_bind(o->aux,&o->header);melee_character_collision_bind(o->collision,&o->header);melee_character_dynamics_bind(o->dynamics,&o->header);
 melee_character_models_bind(o->models,&o->header);melee_character_part_anims_bind(o->part_anims,&o->header);melee_character_parts_bind(o->parts,&o->header);melee_character_wait_bind(o->wait,&o->header);
 o->header.xC=melee_character_motions_records(o->motions);o->header.x4C_sfx=melee_character_sounds_header(o->sounds);return o;
fail:fprintf(stderr,"Donkey decode failure: aux=%d collision=%d dynamics=%d models=%d motions=%d parts_anim=%d parts=%d sounds=%d wait=%d\n",!!o->aux,!!o->collision,!!o->dynamics,!!o->models,!!o->motions,!!o->part_anims,!!o->parts,!!o->sounds,!!o->wait);melee_donkey_data_free(o);return NULL;
}
