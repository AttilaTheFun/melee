#include "melee_hand_data.h"
#include "melee_character_attributes.h"
#include "melee_character_aux.h"
#include "melee_character_collision.h"
#include "melee_character_dynamics.h"
#include "melee_character_models.h"
#include "melee_character_motions.h"
#include "melee_character_parts.h"
#include "melee_character_sounds.h"
#include "melee_character_wait.h"
#include "melee_hand_items.h"
#include <stdlib.h>
#include <stdio.h>
struct MeleeHandData {
 ftData header;ftCo_DatAttrs common;union {struct ftMasterHand_SpecialAttrs master;ftCrazyHand_DatAttrs crazy;} special;
 struct ftData_x1C* part_groups[1];
 MeleeCharacterAux* aux;MeleeCharacterCollision* collision;MeleeCharacterDynamics* dynamics;
 MeleeCharacterModels* models;MeleeCharacterMotions* motions;
 MeleeCharacterParts* parts;MeleeCharacterSounds* sounds;MeleeCharacterWait* wait;MeleeHandItems* items;
};
ftData* melee_hand_data_header(MeleeHandData*o){return o?&o->header:NULL;}
void melee_hand_data_free(MeleeHandData*o){if(!o)return;melee_character_aux_free(o->aux);melee_character_collision_free(o->collision);melee_character_dynamics_free(o->dynamics);melee_character_models_free(o->models);melee_character_motions_free(o->motions);melee_character_parts_free(o->parts);melee_character_sounds_free(o->sounds);melee_character_wait_free(o->wait);melee_hand_items_free(o->items);free(o);}
MeleeHandData* melee_hand_data_decode(const MeleeArchive*a,const void*aj,size_t size,MeleeHostBool crazy)
{
 u32 root;if(!a||!melee_archive_find(a,crazy?"ftDataCrazyhand":"ftDataMasterhand",&root))return NULL;
 MeleeHandData*o=calloc(1,sizeof(*o));if(!o)return NULL;unsigned motions=crazy?344:345;
 u32 groups,unused;MeleeHostBool present;
 if(!melee_archive_pointer(a,root+0x1c,&groups,&present)||!present||!melee_archive_pointer(a,groups,&unused,&present)||present)goto fail;
 if(!melee_character_attributes_decode(a,root,&o->common)||!(crazy?melee_crazyhand_attributes_decode(a,root,&o->special.crazy):melee_masterhand_attributes_decode(a,root,&o->special.master)))goto fail;
 o->header.x0=&o->common;o->header.ext_attr=&o->special;
 if(!(o->aux=melee_character_aux_decode(a,root))||!(o->collision=melee_character_collision_decode(a,root,140))||
    !(o->dynamics=melee_character_dynamics_decode(a,root,140,motions,14))||!(o->models=melee_character_models_decode(a,root))||
    !(o->motions=melee_character_motions_decode(a,root,motions,aj,size))||
    !(o->parts=melee_character_parts_decode(a,root,1,140))||!(o->sounds=melee_character_sounds_decode(a,root))||
    !(o->wait=melee_character_wait_decode(a,root,motions))||!(o->items=melee_hand_items_decode(a,root,crazy)))goto fail;
 melee_character_aux_bind(o->aux,&o->header);melee_character_collision_bind(o->collision,&o->header);melee_character_dynamics_bind(o->dynamics,&o->header);
 melee_character_models_bind(o->models,&o->header);o->header.x1C=o->part_groups;melee_character_parts_bind(o->parts,&o->header);melee_character_wait_bind(o->wait,&o->header);
 o->header.xC=melee_character_motions_records(o->motions);o->header.x4C_sfx=melee_character_sounds_header(o->sounds);o->header.x48_items=melee_hand_items_entries(o->items);return o;
fail:fprintf(stderr,"Hand data decode: aux=%d collision=%d dynamics=%d models=%d motions=%d parts=%d sounds=%d wait=%d items=%d\n",!!o->aux,!!o->collision,!!o->dynamics,!!o->models,!!o->motions,!!o->parts,!!o->sounds,!!o->wait,!!o->items);melee_hand_data_free(o);return NULL;
}
