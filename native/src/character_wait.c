#include "melee_character_wait.h"
#include <stdlib.h>
#include <string.h>
struct MeleeCharacterWait {WaitStruct* tables[2];};
_Static_assert(sizeof(WaitStruct)==8,"Native idle entries contain two integers");
void melee_character_wait_free(MeleeCharacterWait* o){if(o){free(o->tables[0]);free(o->tables[1]);free(o);}}
void melee_character_wait_bind(MeleeCharacterWait* o,ftData* d){if(o&&d){d->x24=o->tables[0];d->x28=o->tables[1];}}
MeleeCharacterWait* melee_character_wait_decode(const MeleeArchive* a,u32 root,unsigned animations)
{
    if(!a||!animations)return NULL;
    MeleeCharacterWait* o=calloc(1,sizeof(*o));if(!o)return NULL;
    for(unsigned k=0;k<2;k++){
        u32 at;MeleeHostBool present;if(!melee_archive_pointer(a,root+0x24+k*4,&at,&present))goto fail;
        if(!present)continue;
        o->tables[k]=calloc(101,sizeof(WaitStruct));if(!o->tables[k])goto fail;
        unsigned total=0,n=0;
        for(;;n++){
            u32 id,weight;
            if(n>100||!melee_archive_u32(a,at+n*8,&id)||!melee_archive_u32(a,at+n*8+4,&weight))goto fail;
            memcpy(&o->tables[k][n].u.i.x,&id,4);memcpy(&o->tables[k][n].u.i.y,&weight,4);
            if(id==UINT32_MAX){if(!n||total!=100)goto fail;break;}
            if(id>=animations||!weight||weight>100-total)goto fail;total+=weight;
        }
    }
    return o;
fail:melee_character_wait_free(o);return NULL;
}
