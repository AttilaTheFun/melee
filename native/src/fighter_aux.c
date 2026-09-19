#include "melee_fighter_aux.h"
#include <math.h>
#include <string.h>
_Static_assert(sizeof(CrowdConfig)==0x44,"Crowd configuration schema");
MeleeHostBool melee_fighter_aux_decode(const MeleeArchive* a,MeleeFighterAux* out)
{
    u32 root;MeleeHostBool present;
    if(!a||!out||!melee_archive_find(a,"ftLoadCommonData",&root))return false;
    MeleeFighterAux value;
    for(unsigned i=0;i<3;i++){
        u32 at;
        if(!melee_archive_pointer(a,root+(17+i)*4,&at,&present)||!present||
           at>a->data_size||20>a->data_size-at)return false;
        memcpy(value.colors[i],a->bytes+32+at,20);
    }
    u32 at;
    if(!melee_archive_pointer(a,root+21*4,&at,&present)||!present||
       at>a->data_size||sizeof(value.crowd)>a->data_size-at)return false;
    for(unsigned i=0;i<sizeof(value.crowd);i+=4){
        u32 bits;if(!melee_archive_u32(a,at+i,&bits))return false;
        if(!(i>=0x1c&&i<=0x28)&&i!=0x3c){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value.crowd+i,&bits,4);
    }
    *out=value;return true;
}
