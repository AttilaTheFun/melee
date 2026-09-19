#include "melee_player_params.h"
#include <math.h>
#include <string.h>
_Static_assert(sizeof(pl_804D6470_t)==0x184,"Player parameter scalar layout");
_Static_assert(offsetof(pl_804D6470_t,x0)==0x0,"x0 offset");
_Static_assert(offsetof(pl_804D6470_t,x4)==0x4,"x4 offset");
_Static_assert(offsetof(pl_804D6470_t,x8)==0x8,"x8 offset");
_Static_assert(offsetof(pl_804D6470_t,xC)==0xC,"xC offset");
_Static_assert(offsetof(pl_804D6470_t,x10)==0x10,"x10 offset");
_Static_assert(offsetof(pl_804D6470_t,x14)==0x14,"x14 offset");
_Static_assert(offsetof(pl_804D6470_t,x18)==0x18,"x18 offset");
_Static_assert(offsetof(pl_804D6470_t,x1C)==0x1C,"x1C offset");
_Static_assert(offsetof(pl_804D6470_t,x20)==0x20,"x20 offset");
_Static_assert(offsetof(pl_804D6470_t,x24)==0x24,"x24 offset");
_Static_assert(offsetof(pl_804D6470_t,x28)==0x28,"x28 offset");
_Static_assert(offsetof(pl_804D6470_t,x2C)==0x2C,"x2C offset");
_Static_assert(offsetof(pl_804D6470_t,x30)==0x30,"x30 offset");
_Static_assert(offsetof(pl_804D6470_t,x34)==0x34,"x34 offset");
_Static_assert(offsetof(pl_804D6470_t,x38)==0x38,"x38 offset");
_Static_assert(offsetof(pl_804D6470_t,x3C)==0x3C,"x3C offset");
_Static_assert(offsetof(pl_804D6470_t,x40)==0x40,"x40 offset");
_Static_assert(offsetof(pl_804D6470_t,x44)==0x44,"x44 offset");
_Static_assert(offsetof(pl_804D6470_t,x48)==0x48,"x48 offset");
_Static_assert(offsetof(pl_804D6470_t,x4C)==0x4C,"x4C offset");
_Static_assert(offsetof(pl_804D6470_t,x50)==0x50,"x50 offset");
_Static_assert(offsetof(pl_804D6470_t,x54)==0x54,"x54 offset");
_Static_assert(offsetof(pl_804D6470_t,x58)==0x58,"x58 offset");
_Static_assert(offsetof(pl_804D6470_t,x5C)==0x5C,"x5C offset");
_Static_assert(offsetof(pl_804D6470_t,x60)==0x60,"x60 offset");
_Static_assert(offsetof(pl_804D6470_t,x64)==0x64,"x64 offset");
_Static_assert(offsetof(pl_804D6470_t,x68)==0x68,"x68 offset");
_Static_assert(offsetof(pl_804D6470_t,x6C)==0x6C,"x6C offset");
_Static_assert(offsetof(pl_804D6470_t,x70)==0x70,"x70 offset");
_Static_assert(offsetof(pl_804D6470_t,x74)==0x74,"x74 offset");
_Static_assert(offsetof(pl_804D6470_t,x78)==0x78,"x78 offset");
_Static_assert(offsetof(pl_804D6470_t,x7C)==0x7C,"x7C offset");
_Static_assert(offsetof(pl_804D6470_t,x80)==0x80,"x80 offset");
_Static_assert(offsetof(pl_804D6470_t,x84)==0x84,"x84 offset");
_Static_assert(offsetof(pl_804D6470_t,x88)==0x88,"x88 offset");
_Static_assert(offsetof(pl_804D6470_t,x8C)==0x8C,"x8C offset");
_Static_assert(offsetof(pl_804D6470_t,x90)==0x90,"x90 offset");
_Static_assert(offsetof(pl_804D6470_t,x94)==0x94,"x94 offset");
_Static_assert(offsetof(pl_804D6470_t,x98)==0x98,"x98 offset");
_Static_assert(offsetof(pl_804D6470_t,x9C)==0x9C,"x9C offset");
_Static_assert(offsetof(pl_804D6470_t,xA0)==0xA0,"xA0 offset");
_Static_assert(offsetof(pl_804D6470_t,xA4)==0xA4,"xA4 offset");
_Static_assert(offsetof(pl_804D6470_t,xA8)==0xA8,"xA8 offset");
_Static_assert(offsetof(pl_804D6470_t,xAC)==0xAC,"xAC offset");
_Static_assert(offsetof(pl_804D6470_t,xB0)==0xB0,"xB0 offset");
_Static_assert(offsetof(pl_804D6470_t,xB4)==0xB4,"xB4 offset");
_Static_assert(offsetof(pl_804D6470_t,xB8)==0xB8,"xB8 offset");
_Static_assert(offsetof(pl_804D6470_t,xBC)==0xBC,"xBC offset");
_Static_assert(offsetof(pl_804D6470_t,xC4)==0xC4,"xC4 offset");
_Static_assert(offsetof(pl_804D6470_t,xC8)==0xC8,"xC8 offset");
_Static_assert(offsetof(pl_804D6470_t,xCC)==0xCC,"xCC offset");
_Static_assert(offsetof(pl_804D6470_t,xD0)==0xD0,"xD0 offset");
_Static_assert(offsetof(pl_804D6470_t,xD4)==0xD4,"xD4 offset");
_Static_assert(offsetof(pl_804D6470_t,xD8)==0xD8,"xD8 offset");
_Static_assert(offsetof(pl_804D6470_t,xDC)==0xDC,"xDC offset");
_Static_assert(offsetof(pl_804D6470_t,xE0)==0xE0,"xE0 offset");
_Static_assert(offsetof(pl_804D6470_t,xE4)==0xE4,"xE4 offset");
_Static_assert(offsetof(pl_804D6470_t,xE8)==0xE8,"xE8 offset");
_Static_assert(offsetof(pl_804D6470_t,xEC)==0xEC,"xEC offset");
_Static_assert(offsetof(pl_804D6470_t,xF0)==0xF0,"xF0 offset");
_Static_assert(offsetof(pl_804D6470_t,xF4)==0xF4,"xF4 offset");
_Static_assert(offsetof(pl_804D6470_t,xF8)==0xF8,"xF8 offset");
_Static_assert(offsetof(pl_804D6470_t,xFC)==0xFC,"xFC offset");
_Static_assert(offsetof(pl_804D6470_t,x100)==0x100,"x100 offset");
_Static_assert(offsetof(pl_804D6470_t,x104)==0x104,"x104 offset");
_Static_assert(offsetof(pl_804D6470_t,x108)==0x108,"x108 offset");
_Static_assert(offsetof(pl_804D6470_t,x10C)==0x10C,"x10C offset");
_Static_assert(offsetof(pl_804D6470_t,x110)==0x110,"x110 offset");
_Static_assert(offsetof(pl_804D6470_t,x114)==0x114,"x114 offset");
_Static_assert(offsetof(pl_804D6470_t,x118)==0x118,"x118 offset");
_Static_assert(offsetof(pl_804D6470_t,x11C)==0x11C,"x11C offset");
_Static_assert(offsetof(pl_804D6470_t,x120)==0x120,"x120 offset");
_Static_assert(offsetof(pl_804D6470_t,x124)==0x124,"x124 offset");
_Static_assert(offsetof(pl_804D6470_t,x128)==0x128,"x128 offset");
_Static_assert(offsetof(pl_804D6470_t,x12C)==0x12C,"x12C offset");
_Static_assert(offsetof(pl_804D6470_t,x130)==0x130,"x130 offset");
_Static_assert(offsetof(pl_804D6470_t,x134)==0x134,"x134 offset");
_Static_assert(offsetof(pl_804D6470_t,x138)==0x138,"x138 offset");
_Static_assert(offsetof(pl_804D6470_t,x13C)==0x13C,"x13C offset");
_Static_assert(offsetof(pl_804D6470_t,x140)==0x140,"x140 offset");
_Static_assert(offsetof(pl_804D6470_t,x144)==0x144,"x144 offset");
_Static_assert(offsetof(pl_804D6470_t,x148)==0x148,"x148 offset");
_Static_assert(offsetof(pl_804D6470_t,x14C)==0x14C,"x14C offset");
_Static_assert(offsetof(pl_804D6470_t,x150)==0x150,"x150 offset");
_Static_assert(offsetof(pl_804D6470_t,x154)==0x154,"x154 offset");
_Static_assert(offsetof(pl_804D6470_t,x158)==0x158,"x158 offset");
_Static_assert(offsetof(pl_804D6470_t,x15C)==0x15C,"x15C offset");
_Static_assert(offsetof(pl_804D6470_t,x160)==0x160,"x160 offset");
_Static_assert(offsetof(pl_804D6470_t,x164)==0x164,"x164 offset");
_Static_assert(offsetof(pl_804D6470_t,x168)==0x168,"x168 offset");
_Static_assert(offsetof(pl_804D6470_t,x16C)==0x16C,"x16C offset");
_Static_assert(offsetof(pl_804D6470_t,x170)==0x170,"x170 offset");
_Static_assert(offsetof(pl_804D6470_t,x174)==0x174,"x174 offset");
_Static_assert(offsetof(pl_804D6470_t,x178)==0x178,"x178 offset");
_Static_assert(offsetof(pl_804D6470_t,x17C)==0x17C,"x17C offset");
_Static_assert(offsetof(pl_804D6470_t,x180)==0x180,"x180 offset");
MeleeHostBool melee_player_params_decode(const MeleeArchive* a,pl_804D6470_t* out){
    uint32_t root,data;MeleeHostBool present;
    if(!a||!out||a->extern_count||!melee_archive_find(a,"plLoadCommonData",&root)||
       !melee_archive_pointer(a,root,&data,&present)||!present||
       data>a->data_size||a->data_size-data<sizeof(*out))return false;
    pl_804D6470_t value;
    /* The untyped four-byte gap stays in its original byte order. */
    memcpy(&value,a->bytes+32+data,sizeof(value));
    if(!melee_archive_f32(a,data+0x0,&value.x0)||!isfinite(value.x0))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x4,&bits))return false;memcpy(&value.x4,&bits,4);}
    if(!melee_archive_f32(a,data+0x8,&value.x8)||!isfinite(value.x8))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0xC,&bits))return false;memcpy(&value.xC,&bits,4);}
    if(!melee_archive_f32(a,data+0x10,&value.x10)||!isfinite(value.x10))return false;
    if(!melee_archive_f32(a,data+0x14,&value.x14)||!isfinite(value.x14))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x18,&bits))return false;memcpy(&value.x18,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x1C,&bits))return false;memcpy(&value.x1C,&bits,4);}
    if(!melee_archive_f32(a,data+0x20,&value.x20)||!isfinite(value.x20))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x24,&bits))return false;memcpy(&value.x24,&bits,4);}
    if(!melee_archive_f32(a,data+0x28,&value.x28)||!isfinite(value.x28))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x2C,&bits))return false;memcpy(&value.x2C,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x30,&bits))return false;memcpy(&value.x30,&bits,4);}
    if(!melee_archive_f32(a,data+0x34,&value.x34)||!isfinite(value.x34))return false;
    if(!melee_archive_f32(a,data+0x38,&value.x38)||!isfinite(value.x38))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x3C,&bits))return false;memcpy(&value.x3C,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x40,&bits))return false;memcpy(&value.x40,&bits,4);}
    if(!melee_archive_f32(a,data+0x44,&value.x44)||!isfinite(value.x44))return false;
    if(!melee_archive_f32(a,data+0x48,&value.x48)||!isfinite(value.x48))return false;
    if(!melee_archive_f32(a,data+0x4C,&value.x4C)||!isfinite(value.x4C))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x50,&bits))return false;memcpy(&value.x50,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x54,&bits))return false;memcpy(&value.x54,&bits,4);}
    if(!melee_archive_f32(a,data+0x58,&value.x58)||!isfinite(value.x58))return false;
    if(!melee_archive_f32(a,data+0x5C,&value.x5C)||!isfinite(value.x5C))return false;
    if(!melee_archive_f32(a,data+0x60,&value.x60)||!isfinite(value.x60))return false;
    if(!melee_archive_f32(a,data+0x64,&value.x64)||!isfinite(value.x64))return false;
    if(!melee_archive_f32(a,data+0x68,&value.x68)||!isfinite(value.x68))return false;
    if(!melee_archive_f32(a,data+0x6C,&value.x6C)||!isfinite(value.x6C))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x70,&bits))return false;memcpy(&value.x70,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x74,&bits))return false;memcpy(&value.x74,&bits,4);}
    if(!melee_archive_f32(a,data+0x78,&value.x78)||!isfinite(value.x78))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x7C,&bits))return false;memcpy(&value.x7C,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x80,&bits))return false;memcpy(&value.x80,&bits,4);}
    if(!melee_archive_f32(a,data+0x84,&value.x84)||!isfinite(value.x84))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x88,&bits))return false;memcpy(&value.x88,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x8C,&bits))return false;memcpy(&value.x8C,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x90,&bits))return false;memcpy(&value.x90,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x94,&bits))return false;memcpy(&value.x94,&bits,4);}
    if(!melee_archive_f32(a,data+0x98,&value.x98)||!isfinite(value.x98))return false;
    if(!melee_archive_f32(a,data+0x9C,&value.x9C)||!isfinite(value.x9C))return false;
    if(!melee_archive_f32(a,data+0xA0,&value.xA0)||!isfinite(value.xA0))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0xA4,&bits))return false;memcpy(&value.xA4,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0xA8,&bits))return false;memcpy(&value.xA8,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0xAC,&bits))return false;memcpy(&value.xAC,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0xB0,&bits))return false;memcpy(&value.xB0,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0xB4,&bits))return false;memcpy(&value.xB4,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0xB8,&bits))return false;memcpy(&value.xB8,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0xBC,&bits))return false;memcpy(&value.xBC,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0xC4,&bits))return false;memcpy(&value.xC4,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0xC8,&bits))return false;memcpy(&value.xC8,&bits,4);}
    if(!melee_archive_f32(a,data+0xCC,&value.xCC)||!isfinite(value.xCC))return false;
    if(!melee_archive_f32(a,data+0xD0,&value.xD0)||!isfinite(value.xD0))return false;
    if(!melee_archive_f32(a,data+0xD4,&value.xD4)||!isfinite(value.xD4))return false;
    if(!melee_archive_f32(a,data+0xD8,&value.xD8)||!isfinite(value.xD8))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0xDC,&bits))return false;memcpy(&value.xDC,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0xE0,&bits))return false;memcpy(&value.xE0,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0xE4,&bits))return false;memcpy(&value.xE4,&bits,4);}
    if(!melee_archive_f32(a,data+0xE8,&value.xE8)||!isfinite(value.xE8))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0xEC,&bits))return false;memcpy(&value.xEC,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0xF0,&bits))return false;memcpy(&value.xF0,&bits,4);}
    if(!melee_archive_f32(a,data+0xF4,&value.xF4)||!isfinite(value.xF4))return false;
    if(!melee_archive_f32(a,data+0xF8,&value.xF8)||!isfinite(value.xF8))return false;
    if(!melee_archive_f32(a,data+0xFC,&value.xFC)||!isfinite(value.xFC))return false;
    if(!melee_archive_f32(a,data+0x100,&value.x100)||!isfinite(value.x100))return false;
    if(!melee_archive_f32(a,data+0x104,&value.x104)||!isfinite(value.x104))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x108,&bits))return false;memcpy(&value.x108,&bits,4);}
    if(!melee_archive_f32(a,data+0x10C,&value.x10C)||!isfinite(value.x10C))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x110,&bits))return false;memcpy(&value.x110,&bits,4);}
    if(!melee_archive_f32(a,data+0x114,&value.x114)||!isfinite(value.x114))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x118,&bits))return false;memcpy(&value.x118,&bits,4);}
    if(!melee_archive_f32(a,data+0x11C,&value.x11C)||!isfinite(value.x11C))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x120,&bits))return false;memcpy(&value.x120,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x124,&bits))return false;memcpy(&value.x124,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x128,&bits))return false;memcpy(&value.x128,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x12C,&bits))return false;memcpy(&value.x12C,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x130,&bits))return false;memcpy(&value.x130,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x134,&bits))return false;memcpy(&value.x134,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x138,&bits))return false;memcpy(&value.x138,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x13C,&bits))return false;memcpy(&value.x13C,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x140,&bits))return false;memcpy(&value.x140,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x144,&bits))return false;memcpy(&value.x144,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x148,&bits))return false;memcpy(&value.x148,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x14C,&bits))return false;memcpy(&value.x14C,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x150,&bits))return false;memcpy(&value.x150,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x154,&bits))return false;memcpy(&value.x154,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x158,&bits))return false;memcpy(&value.x158,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x15C,&bits))return false;memcpy(&value.x15C,&bits,4);}
    if(!melee_archive_f32(a,data+0x160,&value.x160)||!isfinite(value.x160))return false;
    {uint32_t bits;if(!melee_archive_u32(a,data+0x164,&bits))return false;memcpy(&value.x164,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x168,&bits))return false;memcpy(&value.x168,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x16C,&bits))return false;memcpy(&value.x16C,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x170,&bits))return false;memcpy(&value.x170,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x174,&bits))return false;memcpy(&value.x174,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x178,&bits))return false;memcpy(&value.x178,&bits,4);}
    {uint32_t bits;if(!melee_archive_u32(a,data+0x17C,&bits))return false;memcpy(&value.x17C,&bits,4);}
    if(!melee_archive_f32(a,data+0x180,&value.x180)||!isfinite(value.x180))return false;
    *out=value;return true;
}
