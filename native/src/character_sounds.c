#include "melee_character_sounds.h"
#include <stdlib.h>
#include <string.h>
struct MeleeCharacterSounds {FtSFX header;FtSFXArr lists[3];};
void melee_character_sounds_free(MeleeCharacterSounds* o){if(o){for(unsigned i=0;i<3;i++)free(o->lists[i].sfx_ids);free(o);}}
FtSFX* melee_character_sounds_header(MeleeCharacterSounds* o){return o?&o->header:NULL;}
MeleeCharacterSounds* melee_character_sounds_decode(const MeleeArchive* a,u32 root)
{
    u32 at;MeleeHostBool present;
    if(!a||!melee_archive_pointer(a,root+0x4c,&at,&present)||!present)return NULL;
    MeleeCharacterSounds* o=calloc(1,sizeof(*o));if(!o)return NULL;
    unsigned disk[]={4,8,12,16,20,24,36,40,44,48,52};
    int* fields[]={&o->header.x4,&o->header.x8,&o->header.xC,&o->header.x10,
        &o->header.x14,&o->header.x18,&o->header.x24,&o->header.x28,&o->header.x2C,
        &o->header.x30,&o->header.x34};
    for(unsigned i=0;i<11;i++){u32 bits;if(!melee_archive_u32(a,at+disk[i],&bits))goto fail;memcpy(fields[i],&bits,4);}
    unsigned list_offsets[]={0,28,32};FtSFXArr** destinations[]={&o->header.smash,&o->header.x1C,&o->header.x20};
    for(unsigned i=0;i<3;i++){
        u32 list,data,count;
        if(!melee_archive_pointer(a,at+list_offsets[i],&list,&present))goto fail;
        if(!present)continue;
        if(!melee_archive_u32(a,list,&count)||!count||count>256||
           !melee_archive_pointer(a,list+4,&data,&present)||!present||
           data>a->data_size||count*4>a->data_size-data)goto fail;
        o->lists[i].sfx_ids=calloc(count,4);if(!o->lists[i].sfx_ids)goto fail;
        o->lists[i].num=count;*destinations[i]=&o->lists[i];
        for(unsigned j=0;j<count;j++){u32 bits;if(!melee_archive_u32(a,data+4*j,&bits))goto fail;memcpy(o->lists[i].sfx_ids+j,&bits,4);}
    }
    return o;
fail:melee_character_sounds_free(o);return NULL;
}
