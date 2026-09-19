#include "melee_fighter_command.h"

static int signed_field(u32 value,unsigned width)
{ u32 sign=1u<<(width-1);return (int)((value&((1u<<width)-1))^sign)-(int)sign; }

MeleeHostBool melee_fighter_command_single(u32 word,union CmdUnion* out)
{
    if(!out)return false;
    union CmdUnion value={0};
    unsigned op=word>>26;
    value.Command_00.code=op;
    switch(op){
    case 12:
        value.set_hitbox_damage=(struct set_hitbox_damage){((word >> 26) & 0x3fu), ((word >> 23) & 0x7u), (word & 0x7fffffu)};
        break;
    case 13:
        value.set_hitbox_scale=(struct set_hitbox_scale){((word >> 26) & 0x3fu), ((word >> 23) & 0x7u), (word & 0x7fffffu)};
        break;
    case 14:
        value.set_hitbox_x42_b57=(struct set_hitbox_x42_b57){((word >> 26) & 0x3fu), ((word >> 2) & 0xffffffu), ((word >> 1) & 0x1u), (word & 0x1u)};
        break;
    case 15:
        value.set_throw_flags=(struct set_throw_flags){((word >> 26) & 0x3fu), (word & 0x3ffffffu)};
        break;
    case 19:
        value.set_cmd_var=(struct set_cmd_var){((word >> 26) & 0x3fu), ((word >> 24) & 0x3u), (word & 0xffffffu)};
        break;
    case 20:
        value.set_throw_flags=(struct set_throw_flags){((word >> 26) & 0x3fu), (word & 0x3ffffffu)};
        break;
    case 25:
        value.set_airborne_state=(struct set_airborne_state){((word >> 26) & 0x3fu), (word & 0x3ffffffu)};
        break;
    case 26:
        value.set_airborne_state=(struct set_airborne_state){((word >> 26) & 0x3fu), (word & 0x3ffffffu)};
        break;
    case 27:
        value.set_airborne_state=(struct set_airborne_state){((word >> 26) & 0x3fu), (word & 0x3ffffffu)};
        break;
    case 28:
        value.set_hurt_state=(struct set_hurt_state){((word >> 26) & 0x3fu), ((word >> 18) & 0xffu), (word & 0x3ffffu)};
        break;
    case 29:
        value.set_jab_combo=(struct set_jab_combo){((word >> 26) & 0x3fu), (word & 0x3ffffffu)};
        break;
    case 30:
        value.set_jab_rapid=(struct set_jab_rapid){((word >> 26) & 0x3fu), (word & 0x3ffffffu)};
        break;
    case 31:
        value.set_dobj_flags=(struct set_dobj_flags){((word >> 26) & 0x3fu), signed_field(((word >> 19) & 0x7fu),7), signed_field((word & 0x7ffffu),19)};
        break;
    case 35:
        value.unk27=(struct unk27){((word >> 26) & 0x3fu), (word & 0x3ffffffu)};
        break;
    case 36:
        value.set_article_vis=(struct set_article_vis){((word >> 26) & 0x3fu), (word & 0x3ffffffu)};
        break;
    case 37:
        value.set_fighter_vis=(struct set_fighter_vis){((word >> 26) & 0x3fu), (word & 0x3ffffffu)};
        break;
    case 40:
        value.set_tex_anim=(struct set_tex_anim){((word >> 26) & 0x3fu), ((word >> 25) & 0x1u), signed_field(((word >> 18) & 0x7fu),7), signed_field(((word >> 11) & 0x7fu),7), signed_field((word & 0x7ffu),11)};
        break;
    case 41:
        value.part_anim=(struct part_anim){signed_field(((word >> 26) & 0x3fu),6), signed_field(((word >> 19) & 0x7fu),7), signed_field(((word >> 12) & 0x7fu),7), (word & 0xfffu)};
        break;
    case 42:
        value.unk9=(struct unk9){signed_field(((word >> 26) & 0x3fu),6), ((word >> 13) & 0x1fffu), (word & 0x1fffu)};
        break;
    case 43:
        value.unk10=(struct unk10){signed_field(((word >> 26) & 0x3fu),6), ((word >> 25) & 0x1u), ((word >> 13) & 0xfffu), (word & 0x1fffu)};
        break;
    case 44:
        value.unk11=(struct unk11){signed_field(((word >> 26) & 0x3fu),6), (word & 0x3ffffffu)};
        break;
    case 45:
        value.unk12=(struct unk12){((word >> 26) & 0x3fu), ((word >> 24) & 0x3u), ((word >> 14) & 0x3ffu), (word & 0x3fffu)};
        break;
    case 46:
        value.unk13=(struct unk13){((word >> 26) & 0x3fu), ((word >> 18) & 0xffu), (word & 0x3ffffu)};
        break;
    case 47:
        value.unk14=(struct unk14){((word >> 26) & 0x3fu), ((word >> 18) & 0xffu)};
        break;
    case 48:
        value.unk15=(struct unk15){((word >> 26) & 0x3fu), (word & 0x3ffffffu)};
        break;
    case 49:
        value.unk16=(struct unk16){((word >> 26) & 0x3fu), signed_field(((word >> 25) & 0x1u),1), signed_field((word & 0x1ffffffu),25)};
        break;
    case 50:
        value.unk17=(struct unk17){((word >> 26) & 0x3fu), signed_field((word & 0x3ffffffu),26)};
        break;
    case 51:
        value.unk18=(struct unk18){((word >> 26) & 0x3fu), signed_field((word & 0x3ffffffu),26)};
        break;
    case 52:
        value.unk19=(struct unk19){((word >> 26) & 0x3fu), (word & 0x3ffffffu)};
        break;
    case 53:
        value.unk20=(struct unk20){((word >> 26) & 0x3fu), (word & 0x3ffffffu)};
        break;
    case 57:
        value.unk21=(struct unk21){((word >> 26) & 0x3fu), ((word >> 25) & 0x1u), ((word >> 17) & 0xffu)};
        break;
    case 16: case 18: case 21: case 22: case 23: case 24: case 32: case 33:
        break;
    default:return false;
    }
    *out=value;return true;
}

MeleeHostBool melee_fighter_command_decode(const u32* words,size_t count,union CmdUnion* out)
{
    if(!words||!out||!count)return false;
    if(count==1)return melee_fighter_command_single(words[0],out);
    union CmdUnion value[7]={0};
    switch(words[0]>>26){
    case 10:
        if(count!=5)return false;
        value[0].spawn_gfx_0=(struct spawn_gfx_0){((words[0] >> 26) & 0x3fu), ((words[0] >> 18) & 0xffu), ((words[0] >> 17) & 0x1u), ((words[0] >> 16) & 0x1u), ((words[0] >> 15) & 0x1u), ((words[0] >> 0) & 0x7fffu)};
        value[1].spawn_gfx_1=(struct spawn_gfx_1){((words[1] >> 16) & 0xffffu), ((words[1] >> 0) & 0xffffu)};
        value[2].spawn_gfx_2=(struct spawn_gfx_2){signed_field(((words[2] >> 16) & 0xffffu),16), signed_field(((words[2] >> 0) & 0xffffu),16)};
        value[3].spawn_gfx_3=(struct spawn_gfx_3){signed_field(((words[3] >> 16) & 0xffffu),16), ((words[3] >> 0) & 0xffffu)};
        value[4].spawn_gfx_4=(struct spawn_gfx_4){((words[4] >> 16) & 0xffffu), ((words[4] >> 0) & 0xffffu)};
        break;
    case 17:
        if(count!=3)return false;
        value[0].sound_effect_0=(struct sound_effect_0){((words[0] >> 26) & 0x3fu), ((words[0] >> 18) & 0xffu), ((words[0] >> 0) & 0x3ffffu)};
        value[1].sound_effect_1=(struct sound_effect_1){words[1]};
        value[2].sound_effect_2=(struct sound_effect_2){((words[2] >> 16) & 0xffffu), ((words[2] >> 8) & 0xffu), ((words[2] >> 0) & 0xffu)};
        break;
    case 34:
        if(count!=3)return false;
        value[0].set_throw_hitbox_0=(struct set_throw_hitbox_0){((words[0] >> 26) & 0x3fu), ((words[0] >> 23) & 0x7u), ((words[0] >> 0) & 0x7fffffu)};
        value[1].set_throw_hitbox_1=(struct set_throw_hitbox_1){((words[1] >> 23) & 0x1ffu), ((words[1] >> 14) & 0x1ffu), ((words[1] >> 5) & 0x1ffu)};
        value[2].set_throw_hitbox_2=(struct set_throw_hitbox_2){((words[2] >> 23) & 0x1ffu), ((words[2] >> 19) & 0xfu), ((words[2] >> 16) & 0x7u), ((words[2] >> 12) & 0xfu)};
        break;
    case 38:
        if(count!=7)return false;
        value[0].pseudo_random_sfx_0=(struct pseudo_random_sfx_0){((words[0] >> 26) & 0x3fu), ((words[0] >> 18) & 0xffu), ((words[0] >> 10) & 0xffu), ((words[0] >> 6) & 0xfu), ((words[0] >> 0) & 0x3fu)};
        value[1].pseudo_random_sfx_1=(struct pseudo_random_sfx_1){words[1]};
        value[2].pseudo_random_sfx_1=(struct pseudo_random_sfx_1){words[2]};
        value[3].pseudo_random_sfx_1=(struct pseudo_random_sfx_1){words[3]};
        value[4].pseudo_random_sfx_1=(struct pseudo_random_sfx_1){words[4]};
        value[5].pseudo_random_sfx_1=(struct pseudo_random_sfx_1){words[5]};
        value[6].pseudo_random_sfx_1=(struct pseudo_random_sfx_1){words[6]};
        break;
    case 39:
        if(count!=4)return false;
        value[0].stage_sfx_0=(struct stage_sfx_0){((words[0] >> 26) & 0x3fu), ((words[0] >> 16) & 0x3ffu), ((words[0] >> 8) & 0xffu), ((words[0] >> 0) & 0xffu)};
        value[1].stage_sfx_1=(struct stage_sfx_1){words[1]};
        value[2].stage_sfx_2=(struct stage_sfx_2){((words[2] >> 16) & 0xffffu), ((words[2] >> 0) & 0xffffu)};
        value[3].stage_sfx_3=(struct stage_sfx_3){((words[3] >> 16) & 0xffffu), ((words[3] >> 8) & 0xffu), ((words[3] >> 0) & 0xffu)};
        break;
    case 54:
    case 55:
        if(count!=3)return false;
        if((words[0]>>26)==54)
            value[0].footstep_fx_0=(struct footstep_fx_0){54, (words[0]>>18)&255, (words[0]>>17)&1, (words[0]>>16)&1, (words[0]>>8)&255, words[0]&255};
        else
            value[0].unk_fx_0=(struct unk_fx_0){55, (words[0]>>24)&3, (words[0]>>16)&255, (words[0]>>8)&255, words[0]&255};
        value[1].sound_effect_1.sfx_id=words[1];
        value[2].sound_effect_2=(struct sound_effect_2){words[2]>>16, (words[2]>>8)&255, words[2]&255};
        break;
    case 56:
        if(count!=2)return false;
        value[0].smash_charge_0=(struct smash_charge_0){((words[0] >> 26) & 0x3fu), ((words[0] >> 16) & 0x3ffu), ((words[0] >> 0) & 0xffffu)};
        value[1].smash_charge_1=(struct smash_charge_1){((words[1] >> 24) & 0xffu), ((words[1] >> 0) & 0xffffffu)};
        break;
    case 58:
        if(count!=4)return false;
        value[0].wind_fx_0=(struct wind_fx_0){((words[0] >> 26) & 0x3fu), ((words[0] >> 8) & 0x3ffffu), ((words[0] >> 0) & 0xffu)};
        value[1].wind_fx_1=(struct wind_fx_1){signed_field(((words[1] >> 16) & 0xffffu),16), signed_field(((words[1] >> 0) & 0xffffu),16)};
        value[2].wind_fx_2=(struct wind_fx_2){signed_field(((words[2] >> 16) & 0xffffu),16), signed_field(((words[2] >> 0) & 0xffffu),16)};
        value[3].wind_fx_3=(struct wind_fx_3){signed_field(((words[3] >> 16) & 0xffffu),16), signed_field(((words[3] >> 0) & 0xffffu),16)};
        break;
    default:return false;
    }
    for(size_t i=0;i<count;i++)out[i]=value[i];return true;
}

MeleeHostBool melee_fighter_hitbox_decode(const u32* w,size_t count,u32 following_word,union CmdUnion* out)
{
    if(!w||!out||count!=5||(w[0]>>26)!=11)return false;
    union CmdUnion value[5]={0};
    value[0].create_hitbox_0=(struct spawn_hitbox_0){11,(w[0]>>23)&7,(w[0]>>20)&7,(w[0]>>19)&1,(w[0]>>11)&255,(w[0]>>10)&1,w[0]&1023,(following_word>>19)&1};
    value[1].create_hitbox_1=(struct spawn_hitbox_1){w[1]>>16,signed_field(w[1],16)};
    value[2].create_hitbox_2=(struct spawn_hitbox_2){signed_field(w[2]>>16,16),signed_field(w[2],16)};
    value[3].create_hitbox_3=(struct spawn_hitbox_3){w[3]>>23,(w[3]>>14)&511,(w[3]>>5)&511,(w[3]>>4)&1,(w[3]>>3)&1,(w[3]>>2)&1,(w[3]>>1)&1,w[3]&1};
    value[4].create_hitbox_4=(struct spawn_hitbox_4){w[4]>>23,(w[4]>>18)&31,signed_field(w[4]>>10,8),(w[4]>>7)&7,(w[4]>>2)&31,(w[4]>>1)&1,w[4]&1};
    for(size_t i=0;i<5;i++)out[i]=value[i];
    return true;
}
