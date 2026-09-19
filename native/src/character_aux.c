#include "melee_character_aux.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
struct MeleeCharacterAux {ftData_x44_t ledge;Vec2 offset;int bones[5];struct ftData_x58_t limbs;MeleeHostBool has_limbs;};
_Static_assert(sizeof(ftData_x44_t)==28,"Ledge parameter schema");
_Static_assert(sizeof(struct ftData_x58_t)==28,"Limb parameter schema");
void melee_character_aux_free(MeleeCharacterAux* o){free(o);}
void melee_character_aux_bind(MeleeCharacterAux* o,ftData* d)
{if(o&&d){d->x44=&o->ledge;d->x50=&o->offset;d->x54=o->bones;d->x58=o->has_limbs?&o->limbs:NULL;}}
static int scalar(const MeleeArchive*a,u32 at,void* out,int floating)
{u32 bits;if(!melee_archive_u32(a,at,&bits))return 0;if(floating){float f;memcpy(&f,&bits,4);if(!isfinite(f))return 0;}memcpy(out,&bits,4);return 1;}
MeleeCharacterAux* melee_character_aux_decode(const MeleeArchive* a,u32 root)
{
    unsigned slots[]={0x44,0x50,0x54,0x58};u32 refs[4];MeleeHostBool p;
    if(!a)return NULL;
    for(unsigned i=0;i<4;i++)if(!melee_archive_pointer(a,root+slots[i],refs+i,&p)||(!p&&i!=3))return NULL;
    MeleeCharacterAux* o=calloc(1,sizeof(*o));if(!o)return NULL;o->has_limbs=p;
    if(refs[0]>a->data_size||28>a->data_size-refs[0]||(o->has_limbs&&(refs[3]>a->data_size||28>a->data_size-refs[3])))goto fail;
    for(unsigned i=0;i<6;i++){const u8* bytes=a->bytes+32+refs[0]+2*i;u16 bits=((u16)bytes[0]<<8)|bytes[1];memcpy((u8*)&o->ledge+2*i,&bits,2);}
    for(unsigned i=12;i<28;i+=4)if(!scalar(a,refs[0]+i,(u8*)&o->ledge+i,1))goto fail;
    if(!scalar(a,refs[1],&o->offset.x,1)||!scalar(a,refs[1]+4,&o->offset.y,1))goto fail;
    for(unsigned i=0;i<5;i++)if(!scalar(a,refs[2]+4*i,o->bones+i,0))goto fail;
    if(o->has_limbs){
    memcpy(&o->limbs,a->bytes+32+refs[3],28);
    if(!scalar(a,refs[3]+4,&o->limbs.x4,1)||!scalar(a,refs[3]+12,&o->limbs.xC,1)||!scalar(a,refs[3]+24,&o->limbs.x18,1))goto fail;
    }
    return o;
fail:melee_character_aux_free(o);return NULL;
}
