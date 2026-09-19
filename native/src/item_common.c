#include "melee_item_common.h"
#include <math.h>
#include <string.h>
_Static_assert(sizeof(ItemCommonData)==0x160,"Item common scalar layout");
_Static_assert(offsetof(ItemCommonData,x48_byte)==0x48,"Item byte field offset");
_Static_assert(offsetof(ItemCommonData,x80_float)==0x80,"Item float array offset");
_Static_assert(offsetof(ItemCommonData,x15C)==0x15c,"Item last scalar offset");
MeleeHostBool melee_item_common_decode(const MeleeArchive* a,ItemCommonData* out){
    u32 root,data;MeleeHostBool present;
    if(!a||!out||!melee_archive_find(a,"itPublicData",&root)||!melee_archive_pointer(a,root,&data,&present)||!present||
       data>a->data_size||a->data_size-data<sizeof(*out))return false;
    ItemCommonData value;memcpy(&value,a->bytes+32+data,sizeof(value));
    /* Integer fields with historic _float names are bit-preserved too; some
     * original readers reinterpret them as float. Byte fields/padding are raw. */
    for(unsigned i=0;i<sizeof(value);i+=4){
        if(i==0x48||i==0xe4||i==0xec)continue;
        u32 bits;if(!melee_archive_u32(a,data+i,&bits))return false;memcpy((u8*)&value+i,&bits,4);
    }
    static const u16 floats[]={0x4c,0x54,0x58,0x5c,0x60,0x68,0x6c,0x70,0x74,0x78,0x7c,
        0xb8,0xbc,0xc0,0xc4,0xc8,0xcc,0xd0,0xd4,0xe0,0xe8,0xf0,0xf4,0xf8,0x144,0x14c,0x150,0x154,0x158,0x15c};
    for(unsigned i=0;i<sizeof(floats)/sizeof(*floats);i++){
        float v;memcpy(&v,(u8*)&value+floats[i],4);if(!isfinite(v))return false;
    }
    for(unsigned i=0;i<13;i++)if(!isfinite(value.x80_float[i]))return false;
    *out=value;return true;
}
