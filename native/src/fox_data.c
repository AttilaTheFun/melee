#include "melee_fox_data.h"
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
#include "melee_fox_items.h"
#include <stdlib.h>
struct MeleeFoxData {
 ftData header;ftCo_DatAttrs common;struct ftFox_DatAttrs special;
 MeleeCharacterAux* aux;MeleeCharacterCollision* collision;MeleeCharacterDynamics* dynamics;
 MeleeCharacterModels* models;MeleeCharacterMotions* motions;MeleeCharacterPartAnims* part_anims;
 MeleeCharacterParts* parts;MeleeCharacterSounds* sounds;MeleeCharacterWait* wait;MeleeFoxItems* items;
};
ftData* melee_fox_data_header(MeleeFoxData*o){return o?&o->header:NULL;}
void melee_fox_data_free(MeleeFoxData*o){if(!o)return;melee_character_aux_free(o->aux);melee_character_collision_free(o->collision);melee_character_dynamics_free(o->dynamics);melee_character_models_free(o->models);melee_character_motions_free(o->motions);melee_character_part_anims_free(o->part_anims);melee_character_parts_free(o->parts);melee_character_sounds_free(o->sounds);melee_character_wait_free(o->wait);melee_fox_items_free(o->items);free(o);}
MeleeFoxData* melee_fox_data_decode(const MeleeArchive*a,const void*aj,size_t size,MeleeHostBool falco)
{
 u32 root;if(!a||!melee_archive_find(a,falco?"ftDataFalco":"ftDataFox",&root))return NULL;
 MeleeFoxData*o=calloc(1,sizeof(*o));if(!o)return NULL;const unsigned counts[]={4,4,3,4,4};
 if(!melee_character_attributes_decode(a,root,&o->common)||!melee_fox_attributes_decode(a,root,&o->special))goto fail;
 o->header.x0=&o->common;o->header.ext_attr=&o->special;
 if(!(o->aux=melee_character_aux_decode(a,root))||!(o->collision=melee_character_collision_decode(a,root,140))||
    !(o->dynamics=melee_character_dynamics_decode(a,root,140,327,14))||!(o->models=melee_character_models_decode(a,root))||
    !(o->motions=melee_character_motions_decode(a,root,327,aj,size))||!(o->part_anims=melee_character_part_anims_decode(a,root,140,counts,5))||
    !(o->parts=melee_character_parts_decode(a,root,4,140))||!(o->sounds=melee_character_sounds_decode(a,root))||
    !(o->wait=melee_character_wait_decode(a,root,327))||!(o->items=melee_fox_items_decode(a,root,falco)))goto fail;
 melee_character_aux_bind(o->aux,&o->header);melee_character_collision_bind(o->collision,&o->header);melee_character_dynamics_bind(o->dynamics,&o->header);
 melee_character_models_bind(o->models,&o->header);melee_character_part_anims_bind(o->part_anims,&o->header);melee_character_parts_bind(o->parts,&o->header);melee_character_wait_bind(o->wait,&o->header);
 o->header.xC=melee_character_motions_records(o->motions);o->header.x4C_sfx=melee_character_sounds_header(o->sounds);o->header.x48_items=melee_fox_items_entries(o->items);return o;
fail:melee_fox_data_free(o);return NULL;
}
