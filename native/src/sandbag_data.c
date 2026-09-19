#include "melee_sandbag_data.h"
#include "melee_character_attributes.h"
#include "melee_character_aux.h"
#include "melee_character_collision.h"
#include "melee_character_dynamics.h"
#include "melee_character_models.h"
#include "melee_character_motions.h"
#include "melee_character_parts.h"
#include "melee_character_sounds.h"
#include "melee_character_wait.h"
#include <stdlib.h>
#include <stdio.h>
struct MeleeSandbagData {
 ftData header;ftCo_DatAttrs common;u32 special[2];
 MeleeCharacterAux* aux;MeleeCharacterCollision* collision;MeleeCharacterDynamics* dynamics;
 MeleeCharacterModels* models;MeleeCharacterMotions* motions;struct ftData_x1C* part_anims[1];
 MeleeCharacterParts* parts;MeleeCharacterSounds* sounds;MeleeCharacterWait* wait;
};
ftData* melee_sandbag_data_header(MeleeSandbagData*o){return o?&o->header:NULL;}
void melee_sandbag_data_free(MeleeSandbagData*o){if(!o)return;melee_character_aux_free(o->aux);melee_character_collision_free(o->collision);melee_character_dynamics_free(o->dynamics);melee_character_models_free(o->models);melee_character_motions_free(o->motions);melee_character_parts_free(o->parts);melee_character_sounds_free(o->sounds);melee_character_wait_free(o->wait);free(o);}
MeleeSandbagData* melee_sandbag_data_decode(const MeleeArchive*a,const void*aj,size_t size)
{
 u32 root;if(!a||!melee_archive_find(a,"ftDataSandbag",&root))return NULL;
 MeleeSandbagData*o=calloc(1,sizeof(*o));if(!o)return NULL;
 u32 special;MeleeHostBool present;
 if(!melee_character_attributes_decode(a,root,&o->common)||!melee_archive_pointer(a,root+4,&special,&present)||!present||!melee_archive_u32(a,special,&o->special[0])||!melee_archive_u32(a,special+4,&o->special[1]))goto fail;
 u32 unused_items;MeleeHostBool items_present;
 if(!melee_archive_pointer(a,root+0x48,&unused_items,&items_present)||items_present)goto fail;
 o->header.x0=&o->common;o->header.ext_attr=&o->special;
 if(!(o->aux=melee_character_aux_decode(a,root))||!(o->collision=melee_character_collision_decode(a,root,140))||
    !(o->dynamics=melee_character_dynamics_decode(a,root,140,296,14))||!(o->models=melee_character_models_decode(a,root))||
    !(o->motions=melee_character_motions_decode(a,root,296,aj,size))||
    !(o->parts=melee_character_parts_decode(a,root,1,140))||!(o->sounds=melee_character_sounds_decode(a,root))||
    !(o->wait=melee_character_wait_decode(a,root,296)))goto fail;
 u32 empty_table,empty;
 if(!melee_archive_pointer(a,root+0x1c,&empty_table,&present)||!present||!melee_archive_pointer(a,empty_table,&empty,&present)||present)goto fail;
 o->header.x1C=o->part_anims;
 melee_character_aux_bind(o->aux,&o->header);melee_character_collision_bind(o->collision,&o->header);melee_character_dynamics_bind(o->dynamics,&o->header);
 melee_character_models_bind(o->models,&o->header);melee_character_parts_bind(o->parts,&o->header);melee_character_wait_bind(o->wait,&o->header);
 o->header.xC=melee_character_motions_records(o->motions);o->header.x4C_sfx=melee_character_sounds_header(o->sounds);return o;
fail:fprintf(stderr,"Sandbag decode failure: aux=%d collision=%d dynamics=%d models=%d motions=%d parts=%d sounds=%d wait=%d\n",!!o->aux,!!o->collision,!!o->dynamics,!!o->models,!!o->motions,!!o->parts,!!o->sounds,!!o->wait);melee_sandbag_data_free(o);return NULL;
}
