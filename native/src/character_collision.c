#include "melee_character_collision.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
struct MeleeCharacterCollision {
    struct ftData_x30 hurtboxes;
    ftHurtboxInit hurt[15];
    struct ftData_x34 shield;
    struct ftData_x38 bounds[2];
    UnkFloat6_Camera camera;
    itPickup pickup;
};
_Static_assert(sizeof(ftHurtboxInit)==40,"Hurtbox scalar schema");
_Static_assert(sizeof(struct ftData_x38)==20,"Character collision sphere schema");
void melee_character_collision_free(MeleeCharacterCollision* owner){free(owner);}
void melee_character_collision_bind(MeleeCharacterCollision* o,ftData* d)
{if(o&&d){d->x30=&o->hurtboxes;d->x34=&o->shield;d->x38=o->bounds;d->x3C=&o->camera;d->x40=&o->pickup;}}
static int ref(const MeleeArchive* a,u32 at,u32* out)
{MeleeHostBool p;return melee_archive_pointer(a,at,out,&p)&&p;}
static int words(const MeleeArchive* a,u32 at,void* out,unsigned count,unsigned integer_prefix)
{
    for(unsigned i=0;i<count;i++){
        u32 bits;if(!melee_archive_u32(a,at+4*i,&bits))return 0;
        if(i>=integer_prefix){float f;memcpy(&f,&bits,4);if(!isfinite(f))return 0;}
        memcpy((u8*)out+4*i,&bits,4);
    }return 1;
}
MeleeCharacterCollision* melee_character_collision_decode(const MeleeArchive* a,u32 root,unsigned joints)
{
    if(!a||!joints||joints>140)return NULL;
    u32 refs[5];for(unsigned i=0;i<5;i++)if(!ref(a,root+0x30+4*i,refs+i))return NULL;
    MeleeCharacterCollision* o=calloc(1,sizeof(*o));if(!o)return NULL;
    u32 count,at;if(!melee_archive_u32(a,refs[0],&count)||count>15||!ref(a,refs[0]+4,&at))goto fail;
    o->hurtboxes.count=count;o->hurtboxes.inits=o->hurt;
    for(unsigned i=0;i<count;i++){
        ftHurtboxInit* h=o->hurt+i;
        if(!words(a,at+40*i,h,10,3)||(unsigned)h->bone_idx>=joints||
           (unsigned)h->height>HurtHeight_High||h->scale<0)goto fail;
    }
    if(!words(a,refs[1],&o->shield,2,1)||(unsigned)o->shield.x0>=joints||o->shield.scale<0)goto fail;
    for(unsigned i=0;i<2;i++)if(!words(a,refs[2]+20*i,&o->bounds[i],5,1)||
        (unsigned)o->bounds[i].x0>=joints||o->bounds[i].x10<0)goto fail;
    if(!words(a,refs[3],&o->camera,6,0)||!words(a,refs[4],&o->pickup,12,0))goto fail;
    return o;
fail:melee_character_collision_free(o);return NULL;
}
