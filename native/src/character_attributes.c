#include "melee_character_attributes.h"
#include <math.h>
#include <string.h>
_Static_assert(sizeof(ftCo_DatAttrs)==0x184,"Common character parameter layout");
_Static_assert(offsetof(ftCo_DatAttrs,weight_independent_throws_mask)==0x180,"Throw mask byte offset");
_Static_assert(sizeof(struct ftFox_DatAttrs)==0xd4,"Fox special attribute layout");
_Static_assert(sizeof(ftMario_DatAttrs)==0x84,"Mario special attribute layout");
_Static_assert(offsetof(ftMario_DatAttrs,cape_reflection)==0x60,"Mario cape reflector offset");
MeleeHostBool melee_mario_attributes_decode(const MeleeArchive* a,u32 root,ftMario_DatAttrs* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftMario_DatAttrs value;memcpy(&value,a->bytes+32+at,sizeof(value));
    /* The final reflector behavior byte and its padding are byte data. */
    for(unsigned offset=0;offset<0x80;offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0x14&&offset!=0x50&&offset!=0x5c&&offset!=0x60&&offset!=0x64){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}
MeleeHostBool melee_character_attributes_decode(const MeleeArchive* a,u32 root,ftCo_DatAttrs* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftCo_DatAttrs value;memcpy(&value,a->bytes+32+at,sizeof(value));
    for(unsigned offset=0;offset<0x180;offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0x58&&offset!=0x98&&offset!=0xa0&&offset!=0xa4&&offset!=0x16c){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}
MeleeHostBool melee_fox_attributes_decode(const MeleeArchive* a,u32 root,struct ftFox_DatAttrs* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    struct ftFox_DatAttrs value;memcpy(&value,a->bytes+32+at,sizeof(value));
    for(unsigned offset=0;offset<0xd0;offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0x1c&&offset!=0x20&&offset!=0x6c&&offset!=0xa4&&offset!=0xb0&&offset!=0xb4){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftNessAttributes)==0xdc,"Ness special attribute layout");
_Static_assert(offsetof(ftNessAttributes,xB8_BASEBALL_BAT)==0xb8,"Ness reflector offset");
MeleeHostBool melee_ness_attributes_decode(const MeleeArchive* a,u32 root,ftNessAttributes* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftNessAttributes value;memcpy(&value,a->bytes+32+at,sizeof(value));
    for(unsigned offset=0;offset<0xd8;offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0&&offset!=4&&offset!=8&&offset!=12&&offset!=0x40&&offset!=0x44&&offset!=0x48&&offset!=0x84&&offset!=0x98&&offset!=0xb8&&offset!=0xbc){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(struct ftCaptain_DatAttrs)==0x8c,"Captain/Ganon special attribute layout");
MeleeHostBool melee_captain_attributes_decode(const MeleeArchive* a,u32 root,struct ftCaptain_DatAttrs* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    struct ftCaptain_DatAttrs value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0x64&&offset!=0x6c&&offset!=0x78){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftDonkeyAttributes)==0x74,"Donkey special attribute layout");
MeleeHostBool melee_donkey_attributes_decode(const MeleeArchive* a,u32 root,ftDonkeyAttributes* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftDonkeyAttributes value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0&&offset!=4&&offset!=0x2c&&offset!=0x30){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftKoopaAttributes)==0xa0,"Koopa special attribute layout");
MeleeHostBool melee_koopa_attributes_decode(const MeleeArchive* a,u32 root,ftKoopaAttributes* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftKoopaAttributes value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=4&&offset!=0x20&&offset!=0x2c&&offset!=0x50){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftLuigiAttributes)==0x98,"Luigi special attribute layout");
MeleeHostBool melee_luigi_attributes_decode(const MeleeArchive* a,u32 root,ftLuigiAttributes* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftLuigiAttributes value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0x88&&offset!=0x94){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(MarsAttributes)==0x98,"Marth/Roy special attribute layout");
_Static_assert(offsetof(MarsAttributes,x78.x8)==0x80,"Sword byte settings offset");
_Static_assert(offsetof(MarsAttributes,x78.x14)==0x8c,"Sword integer setting offset");
MeleeHostBool melee_mars_attributes_decode(const MeleeArchive* a,u32 root,MarsAttributes* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    MarsAttributes value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        if(offset>=0x80&&offset<0x8c){
            memcpy((u8*)&value+offset,a->bytes+32+at+offset,4);continue;
        }
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0&&offset!=4&&offset!=8&&offset!=0x64&&offset!=0x8c){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftPikachuAttributes)==0xf8,"Pikachu/Pichu special attribute layout");
_Static_assert(offsetof(ftPikachuAttributes,height_attributes)==0xe0,"Pikachu collision box offset");
MeleeHostBool melee_pikachu_attributes_decode(const MeleeArchive* a,u32 root,ftPikachuAttributes* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftPikachuAttributes value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0x14&&offset!=0x18&&offset!=0x5c&&offset!=0x60&&
           offset!=0xa0&&offset!=0xa8&&offset!=0xd4&&offset!=0xd8&&offset!=0xdc){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftPurinAttributes)==0x100,"Jigglypuff special attribute layout");
_Static_assert(offsetof(ftPurinAttributes,xF0)==0xf0,"Jigglypuff late scalar offset");
MeleeHostBool melee_purin_attributes_decode(const MeleeArchive* a,u32 root,ftPurinAttributes* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftPurinAttributes value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        if(offset==0x48||offset==0x60||offset==0x64||offset==0xb0||offset>=0xf8){
            memcpy((u8*)&value+offset,a->bytes+32+at+offset,4);continue;
        }
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        /* x00..x30 also serve as Fighter_x2D0_t multi-jump settings;
         * its five y impulses are floats despite Purin's legacy labels. */
        if(offset!=0&&offset!=0x28&&offset!=0x2c&&offset!=0x30&&
           offset!=0x34&&offset!=0x38&&offset!=0x70&&offset!=0x9c&&
           offset!=0xe8&&offset!=0xec){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftYoshiAttributes)==0x138,"Yoshi full attribute layout");
_Static_assert(sizeof(struct ftYs_DatAttrs)==0x120,"Yoshi alternate attribute layout");
_Static_assert(offsetof(struct ftYs_DatAttrs,specialhi_base_angle)==0xf8,"Yoshi egg throw angle offset");
_Static_assert(offsetof(struct ftYs_DatAttrs,speciallw_star_offset)==0x118,"Yoshi star spawn offset");
MeleeHostBool melee_yoshi_attributes_decode(const MeleeArchive* a,u32 root,ftYoshiAttributes* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftYoshiAttributes value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        if(offset>=0x12c){memcpy((u8*)&value+offset,a->bytes+32+at+offset,4);continue;}
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        /* The alternate ftYs_DatAttrs view uses the float words at EC..110
         * that the full declaration labels as padding. */
        if(offset!=0&&offset!=0x38&&offset!=0x48&&offset!=0x4c&&
           offset!=0x50&&offset!=0xa4&&offset!=0xdc){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftZelda_DatAttrs)==0xa8,"Zelda attribute layout");
_Static_assert(offsetof(ftZelda_DatAttrs,x84)==0x84,"Zelda reflector offset");
MeleeHostBool melee_zelda_attributes_decode(const MeleeArchive* a,u32 root,ftZelda_DatAttrs* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftZelda_DatAttrs value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        /* ReflectDesc ends in one behavior byte and three padding bytes. */
        if(offset==0xa4){memcpy((u8*)&value+offset,a->bytes+32+at+offset,4);continue;}
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        /* The unused x28/x30 scalar slots contain float parameters in retail. */
        if(offset!=4&&offset!=0x10&&offset!=0x14&&offset!=0x18&&offset!=0x1c&&
           offset!=0x48&&offset!=0x60&&offset!=0x84&&offset!=0x88){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftSeakAttributes)==0x74,"Sheik attribute layout");
MeleeHostBool melee_sheik_attributes_decode(const MeleeArchive* a,u32 root,ftSeakAttributes* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftSeakAttributes value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0x38&&offset!=0x50){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftPe_DatAttrs)==0xc0,"Peach attribute layout");
_Static_assert(offsetof(ftPe_DatAttrs,speciallw_item_table)==0x18,"Peach item chance table offset");
_Static_assert(offsetof(ftPe_DatAttrs,xAC)==0xac,"Peach absorb descriptor offset");
MeleeHostBool melee_peach_attributes_decode(const MeleeArchive* a,u32 root,ftPe_DatAttrs* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftPe_DatAttrs value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(!(offset>=0x10&&offset<=0x30)&&offset!=0x90&&offset!=0xac){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftSs_DatAttrs)==0xd4,"Samus attribute layout");
_Static_assert(offsetof(ftSs_DatAttrs,height_attributes)==0x84,"Samus collision box offset");
_Static_assert(offsetof(ftSs_DatAttrs,xD0)==0xd0,"Samus final scalar offset");
MeleeHostBool melee_samus_attributes_decode(const MeleeArchive* a,u32 root,ftSs_DatAttrs* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftSs_DatAttrs value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0x20&&!(offset>=0x9c&&offset<=0xc8)&&offset!=0xd0){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

/* The retail x94/x9C/xA0 slots are non-relocated integer words, not pointers. */
_Static_assert(sizeof(struct ftLk_DatAttrs)==0xdc,"Link/Young Link attribute layout");
_Static_assert(offsetof(struct ftLk_DatAttrs,x64)==0x64,"Link sword attribute offset");
_Static_assert(offsetof(struct ftLk_DatAttrs,xC4)==0xc4,"Link absorb descriptor offset");
MeleeHostBool melee_link_attributes_decode(const MeleeArchive* a,u32 root,struct ftLk_DatAttrs* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    struct ftLk_DatAttrs value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        if(offset==0x6c||offset==0x70||offset==0x74||offset==0xc0){memcpy((u8*)&value+offset,a->bytes+32+at+offset,4);continue;}
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0xc&&offset!=0x10&&offset!=0x2c&&offset!=0x48&&offset!=0x58&&offset!=0x5c&&offset!=0x60&&offset!=0x78&&offset!=0x84&&offset!=0x88&&offset!=0x8c&&offset!=0x90&&offset!=0x94&&offset!=0x98&&offset!=0x9c&&offset!=0xa0&&offset!=0xa4&&offset!=0xa8&&offset!=0xac&&offset!=0xb0&&offset!=0xb8&&offset!=0xbc&&offset!=0xc4){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftMewtwoAttributes)==0x88,"Mewtwo attribute layout");
_Static_assert(offsetof(ftMewtwoAttributes,x1C_MEWTWO_CONFUSION_REFLECTION)==0x1c,"Mewtwo reflector offset");
MeleeHostBool melee_mewtwo_attributes_decode(const MeleeArchive* a,u32 root,ftMewtwoAttributes* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftMewtwoAttributes value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        if(offset==0x3c){memcpy((u8*)&value+offset,a->bytes+32+at+offset,4);continue;}
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0xc&&offset!=0x10&&offset!=0x1c&&offset!=0x20&&offset!=0x50&&offset!=0x68){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftIceClimberAttributes)==0x15c,"Ice Climbers attributes layout");
MeleeHostBool melee_iceclimbers_attributes_decode(const MeleeArchive* a,u32 root,ftIceClimberAttributes* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftIceClimberAttributes value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        if(offset==0xcc||(offset>=0xd4&&offset<0x12c)||offset>=0x150){memcpy((u8*)&value+offset,a->bytes+32+at+offset,4);continue;}
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset!=0x1c&&offset!=0x68){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(ftGameWatchAttributes)==0x94,"Game & Watch attribute layout");
MeleeHostBool melee_gamewatch_attributes_decode(const MeleeArchive* a,u32 root,ftGameWatchAttributes* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    ftGameWatchAttributes value;
    for(unsigned offset=0;offset<sizeof(value);offset+=4){
        if(offset>=4&&offset<=0x14){memcpy((u8*)&value+offset,a->bytes+32+at+offset,4);continue;}
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(!(offset>=0x34&&offset<=0x54)&&offset!=0x80){
            float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    *out=value;return true;
}

_Static_assert(sizeof(struct ftKb_DatAttrs)==0x424,"Kirby special attribute layout");
_Static_assert(offsetof(struct ftKb_DatAttrs,jumpaerial_unk)==0x34,"Kirby short jump parameter");
_Static_assert(offsetof(struct ftKb_DatAttrs,ms)==0x35c,"Kirby Marth copy parameters");
_Static_assert(offsetof(struct ftKb_DatAttrs,fe)==0x370,"Kirby Roy copy parameters");
_Static_assert(offsetof(struct ftKb_DatAttrs,specialn_zd_reflectdesc)==0x400,"Kirby copied reflector");
MeleeHostBool melee_kirby_attributes_decode(const MeleeArchive* a,u32 root,struct ftKb_DatAttrs* out)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||sizeof(*out)>a->data_size-at)return false;
    struct ftKb_DatAttrs value;memcpy(&value,a->bytes+32+at,sizeof(value));
    for(unsigned offset=0;offset<0x420;offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        if(offset==0x34){
            /* A signed halfword followed by two padding bytes. */
            u16 half=(u16)(bits>>16);memcpy((u8*)&value+offset,&half,2);continue;
        }
        switch(offset){
        case 0:case 0x28:case 0x2c:case 0x30:case 0xec:case 0xf0:
        case 0x108:case 0x10c:case 0x110:case 0x118:case 0x134:
        case 0x140:case 0x15c:case 0x170:case 0x190:case 0x194:
        case 0x1a0:case 0x1a4:case 0x1a8:case 0x1ac:
        case 0x1d4:case 0x1d8:case 0x1f0:case 0x1f4:
        case 0x23c:case 0x240:case 0x260:case 0x264:
        case 0x274:case 0x278:case 0x288:case 0x28c:
        case 0x2a8:case 0x2b4:case 0x2b8:case 0x2f0:case 0x31c:
        case 0x35c:case 0x360:case 0x364:case 0x370:case 0x374:case 0x378:
        case 0x390:case 0x394:case 0x3d4:case 0x3ec:case 0x400:case 0x404:
            break;
        default:{float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;break;}
        }
        memcpy((u8*)&value+offset,&bits,4);
    }
    /* ReflectDesc's last word is one behavior byte and three padding bytes. */
    *out=value;return true;
}

_Static_assert(sizeof(struct ftMasterHand_SpecialAttrs)==0x17c,"Master Hand attribute layout");
_Static_assert(sizeof(ftCrazyHand_DatAttrs)==0x144,"Crazy Hand attribute layout");
static MeleeHostBool hand_attributes(const MeleeArchive* a,u32 root,void* out,unsigned size,const unsigned* integers,unsigned count)
{
    u32 at;MeleeHostBool present;
    if(!a||!out||!melee_archive_pointer(a,root+4,&at,&present)||!present||
       at>a->data_size||size>a->data_size-at)return false;
    u32 words[0x17c/4];
    for(unsigned offset=0;offset<size;offset+=4){
        u32 bits;if(!melee_archive_u32(a,at+offset,&bits))return false;
        unsigned i=0;while(i<count&&integers[i]!=offset)i++;
        if(i==count){float value;memcpy(&value,&bits,4);if(!isfinite(value))return false;}
        words[offset/4]=bits;
    }
    memcpy(out,words,size);return true;
}
MeleeHostBool melee_masterhand_attributes_decode(const MeleeArchive* a,u32 root,struct ftMasterHand_SpecialAttrs* out)
{
    const unsigned ints[]={0,4,8,12,16,20,24,28,32,36,0x6c,0x70,0x74,0x7c,0x84,0x90,0x94,0xa0,0xb0,0xb4,0xec,0xf0,0x144,0x148,0x160,0x164,0x168,0x16c,0x170,0x174};
    return hand_attributes(a,root,out,sizeof(*out),ints,sizeof(ints)/sizeof(*ints));
}
MeleeHostBool melee_crazyhand_attributes_decode(const MeleeArchive* a,u32 root,ftCrazyHand_DatAttrs* out)
{
    const unsigned ints[]={0,4,8,12,0x48,0x54,0x58,0x64,0x74,0x78,0xf0,0xf4,0xf8,0x100,0x124,0x128};
    return hand_attributes(a,root,out,sizeof(*out),ints,sizeof(ints)/sizeof(*ints));
}
