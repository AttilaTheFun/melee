#include "../../src/melee/ft/ftaction.c"
#include "melee_fighter_command.h"
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static unsigned effect_calls;
static int effect_id,effect_bone,effect_common,effect_destroy;
static Vec3 effect_offset,effect_range;
static float effect_extra;
void ftCo_8009F834(Fighter_GObj* g,int id,Fighter_Part bone,int common,int destroy,Vec3* offset,Vec3* range,float extra)
{(void)g;effect_calls++;effect_id=id;effect_bone=bone;effect_common=common;effect_destroy=destroy;effect_offset=*offset;effect_range=*range;effect_extra=extra;}
static unsigned sound_calls, common_calls;
static int sound_ids[8], sound_behaviors[8];
static u8 sound_volumes[8], sound_pans[8];
static int context_sound, context_fallback, context_effect;
static bool context_found;
static void record_sound(int behavior,int id,u8 volume,u8 pan)
{ assert(sound_calls<8);sound_behaviors[sound_calls]=behavior;sound_ids[sound_calls]=id;sound_volumes[sound_calls]=volume;sound_pans[sound_calls++]=pan; }
void ft_PlaySFX(Fighter* fp,int id,u8 volume,u8 pan) { (void)fp;record_sound(0,id,volume,pan); }
void ft_80088478(Fighter* fp,int id,u8 volume,u8 pan) { (void)fp;record_sound(1,id,volume,pan); }
void ft_800881D8(Fighter* fp,int id,u8 volume,u8 pan) { (void)fp;record_sound(2,id,volume,pan); }
void ft_80088510(Fighter* fp,int id,u8 volume,u8 pan) { (void)fp;record_sound(3,id,volume,pan); }
void ft_800885A8(Fighter* fp,int id,u8 volume,u8 pan) { (void)fp;record_sound(4,id,volume,pan); }
void ft_80088640(Fighter* fp,int id,u8 volume,u8 pan) { (void)fp;record_sound(5,id,volume,pan); }
void ft_80088328(Fighter* fp,int id,u8 volume,u8 pan) { (void)fp;record_sound(6,id,volume,pan); }
void ft_80088828(Fighter* fp) { (void)fp;record_sound(10,0,0,0); }
void ft_80088770(Fighter* fp) { (void)fp;record_sound(11,0,0,0); }
void ft_80088884(Fighter* fp) { (void)fp;record_sound(12,0,0,0); }
void ft_800888E0(Fighter* fp) { (void)fp;record_sound(13,0,0,0); }
void ft_8008893C(Fighter* fp) { (void)fp;record_sound(14,0,0,0); }
void ft_800887CC(Fighter* fp) { (void)fp;record_sound(15,0,0,0); }
bool ft_80084BFC(Fighter_GObj* g,int* sound,int* fallback,int* effect) { (void)g;*sound=context_sound;*fallback=context_fallback;*effect=context_effect;return context_found; }
bool ft_80084C38(Fighter_GObj* g,int* sound,int* fallback,int* effect) { (void)g;*sound=context_sound;*fallback=context_fallback;*effect=context_effect;return context_found; }
void ftCommon_8007EBAC(Fighter* fp,u32 a,u32 b) { (void)fp;assert(a==0x16&&b==0);common_calls++; }
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
static unsigned hit_resets,hit_common;
static u32 hit_damage;
static int hit_angle;
void ftColl_800768A0(Fighter* fp,HitCapsule* hit) {(void)fp;(void)hit;hit_resets++;}
void ftColl_8007ABD0(HitCapsule* hit,u32 damage,Fighter_GObj* g) {(void)hit;(void)g;hit_damage=damage;}
void ftColl_8007AC9C(HitCapsule* hit,int angle,Fighter_GObj* g) {(void)hit;(void)g;hit_angle=angle;}
void ftColl_8007AD18(Fighter* fp,HitCapsule* hit) {(void)fp;(void)hit;assert(0);}
void ftCommon_80080484(Fighter* fp) {(void)fp;hit_common++;}
Fighter_Part ftParts_GetBoneIndex(Fighter* fp,Fighter_Part part) {(void)fp;return part+1;}
int main(void){
 Fighter fighter={0};HSD_GObj gobj={0};gobj.user_data=&fighter;union CmdUnion commands[2]={{0}};CommandInfo c={0};
 const u32 values[]={0,1,0x7fffff,0x800000,0xffffff};
 for(unsigned i=0;i<4;i++)for(unsigned j=0;j<5;j++){
  assert(melee_fighter_command_single((19u<<26)|(i<<24)|values[j],commands));c.u=commands;ftAction_80071820(&gobj,&c);assert(fighter.cmd_vars[i]==values[j]&&c.u==commands+1);
 }
 assert(melee_fighter_command_single(20u<<26,commands));c.u=commands;c.timer=3.25f;ftAction_800718A4(&gobj,&c);assert(fighter.throw_flags_b3&&fighter.cmd_timer==3.25f&&c.u==commands+1);
 assert(melee_fighter_command_single((20u<<26)|1,commands));c.u=commands;ftAction_800718A4(&gobj,&c);assert(fighter.throw_flags_b4);
 assert(melee_fighter_command_single(21u<<26,commands));c.u=commands;ftAction_80071908(&gobj,&c);assert(fighter.throw_flags_b1);
 assert(melee_fighter_command_single(22u<<26,commands));c.u=commands;ftAction_8007192C(&gobj,&c);assert(fighter.throw_flags_b2);
 assert(melee_fighter_command_single((31u<<26)|0x3ffffff,commands));assert(commands[0].set_dobj_flags.idx==-1&&commands[0].set_dobj_flags.value==-1);
 assert(melee_fighter_command_single((40u<<26)|(1u<<25)|(64u<<18)|(127u<<11)|1024,commands));assert(commands[0].set_tex_anim.b==1&&commands[0].set_tex_anim.idx==-64&&commands[0].set_tex_anim.idx2==-1&&commands[0].set_tex_anim.frame==-1024);
 assert(melee_fighter_command_single((49u<<26)|(1u<<25)|0x1000000,commands));assert(commands[0].unk16.unk3==-1&&commands[0].unk16.unk4==-16777216);
 assert(melee_fighter_command_single((51u<<26)|0x2000000,commands));assert(commands[0].unk18.damage_amount==-33554432);
 unsigned supported=0;for(unsigned op=0;op<64;op++){
  union CmdUnion saved=commands[0];if(melee_fighter_command_single((op<<26)|0x1555555,commands)){assert(commands[0].Command_00.code==op);supported++;}else assert(!memcmp(commands,&saved,sizeof(saved)));
 }
 union CmdUnion multi[8]={{0}};
 u32 effect[]={ (10u<<26)|(7u<<18)|(1u<<17)|(1u<<16),0x12340123,0xff008000,0x7fff0100,0x02000300 };
 assert(melee_fighter_command_decode(effect,5,multi));c.u=multi;ftAction_80071028(&gobj,&c);
 assert(effect_calls==1&&effect_id==0x1234&&effect_bone==7&&effect_common&&effect_destroy&&effect_extra==0x123);
 assert(effect_offset.x==0.003906f*-256&&effect_offset.y==0.003906f*-32768&&effect_offset.z==0.003906f*32767);
 assert(effect_range.x==0.003906f*256&&effect_range.y==0.003906f*512&&effect_range.z==0.003906f*768&&c.u==multi+5);
 fighter.invisible=true;c.u=multi;ftAction_80071028(&gobj,&c);assert(effect_calls==1&&c.u==multi+5);fighter.invisible=false;
 struct ftData_x8 bones={0};ftData data={0};data.x8=&bones;bones.x12=9;fighter.ft_data=&data;effect[0]|=1u<<15;
 assert(melee_fighter_command_decode(effect,5,multi));c.u=multi;ftAction_80071028(&gobj,&c);assert(effect_calls==2&&effect_bone==9);
 u32 sound[]={(17u<<26)|(2u<<18)|123,0x12345678,0x00006440};assert(melee_fighter_command_decode(sound,3,multi));assert(multi[0].sound_effect_0.behavior==2&&multi[1].sound_effect_1.sfx_id==0x12345678&&multi[2].sound_effect_2.volume==100&&multi[2].sound_effect_2.panning==64);
 u32 throwing[]={(34u<<26)|(2u<<23)|123, (361u<<23)|(100u<<14)|(50u<<5), (80u<<23)|(3u<<19)|(2u<<16)|(5u<<12)};
 assert(melee_fighter_command_decode(throwing,3,multi));assert(multi[0].set_throw_hitbox_0.idx==2&&multi[0].set_throw_hitbox_0.damage==123&&multi[1].set_throw_hitbox_1.unk0==361&&multi[2].set_throw_hitbox_2.sfx_kind==5);
 u32 random[]={(38u<<26)|(100u<<18)|(64u<<10)|(2u<<6)|6,1,2,3,4,5,6};assert(melee_fighter_command_decode(random,7,multi));assert(multi[0].pseudo_random_sfx_0.random_range==6);for(unsigned i=1;i<7;i++)assert(multi[i].pseudo_random_sfx_1.sfx_id==i);
 u32 stage[]={(39u<<26)|(123u<<16)|0xabcd,0x12345678,0x11223344,0x55667788};assert(melee_fighter_command_decode(stage,4,multi));assert(multi[0].stage_sfx_0.sfx_base==123&&multi[2].stage_sfx_2.x2_b0_15==0x3344&&multi[3].stage_sfx_3.x3_b0_7==0x88);
 u32 charge[]={(56u<<26)|(63u<<16)|256,0x05000000};assert(melee_fighter_command_decode(charge,2,multi));assert(multi[0].smash_charge_0.charge_frames==63&&multi[0].smash_charge_0.charge_rate==256&&multi[1].smash_charge_1.color_anim==5);
 u32 wind[]={(58u<<26)|7,0xffff8000,0x7fff0001,0x8000ffff};assert(melee_fighter_command_decode(wind,4,multi));assert(multi[1].wind_fx_1.timer==-1&&multi[1].wind_fx_1.x==-32768&&multi[3].wind_fx_3.angle==-32768&&multi[3].wind_fx_3.decay==-1);

 /* Exercise both original overlay handlers, including their temporary sound
  * commands: native slots are wider than the three serialized words. */
 u32 footstep[]={(54u<<26)|(2u<<18),0x12345678,0x00006440};
 bones.x13=11;bones.x14=12;
 context_found=true;context_sound=99;context_fallback=1;context_effect=77;
 assert(melee_fighter_command_decode(footstep,3,multi));c.u=multi;sound_calls=0;
 ftAction_80072CD8(&gobj,&c);
 assert(c.u==multi+3&&sound_calls==2&&sound_ids[0]==99&&sound_ids[1]==0x12345678);
 assert(sound_behaviors[0]==2&&sound_behaviors[1]==2&&sound_volumes[0]==100&&sound_pans[0]==64&&effect_bone==11&&effect_id==77);
 footstep[0]|=1u<<17;context_fallback=0;
 assert(melee_fighter_command_decode(footstep,3,multi));c.u=multi;sound_calls=0;ftAction_80072CD8(&gobj,&c);
 assert(c.u==multi+3&&sound_calls==1&&effect_bone==12);
 context_found=false;context_fallback=1;c.u=multi;sound_calls=0;unsigned before_effects=effect_calls;ftAction_80072CD8(&gobj,&c);
 assert(c.u==multi+3&&sound_calls==1&&sound_ids[0]==0x12345678&&effect_calls==before_effects);
 u32 contextual[]={(55u<<26)|(2u<<18)|0x1234,0x76543210,0x00006030};
 context_found=true;context_sound=101;context_fallback=1;context_effect=-1;
 assert(melee_fighter_command_decode(contextual,3,multi));c.u=multi;sound_calls=0;ftAction_80072E4C(&gobj,&c);
 assert(c.u==multi+3&&sound_calls==3&&sound_ids[0]==101&&sound_ids[1]==0x46&&sound_ids[2]==0x76543210);
 assert(sound_behaviors[0]==2&&sound_behaviors[2]==2&&sound_volumes[0]==96&&sound_pans[0]==48&&effect_id==0x1234&&effect_bone==FtPart_TopN&&common_calls==1);
 contextual[0]|=1u<<16;context_effect=88;
 assert(melee_fighter_command_decode(contextual,3,multi));c.u=multi;sound_calls=0;ftAction_80072E4C(&gobj,&c);
 assert(c.u==multi+3&&sound_calls==1&&sound_ids[0]==0x46&&effect_id==88&&common_calls==2);
 contextual[0]&=~(1u<<16);context_fallback=0;
 assert(melee_fighter_command_decode(contextual,3,multi));c.u=multi;sound_calls=0;ftAction_80072E4C(&gobj,&c);
 assert(c.u==multi+3&&sound_calls==1&&sound_ids[0]==101&&common_calls==3);

 assert(sizeof(union CmdUnion)==sizeof(void*));
 u32 hit_words[]={(11u<<26)|(1u<<23)|(3u<<20)|(1u<<19)|(2u<<11)|37,0x01008000,0xffff7fff,(361u<<23)|(100u<<14)|(50u<<5)|23,(80u<<23)|(3u<<18)|(0x80u<<10)|(2u<<7)|(5u<<2)|3};
 /* Deliberately disagree with the first word's similarly named field. */
 assert(melee_fighter_hitbox_decode(hit_words,5,0,multi));
 fighter.parts=calloc(4,sizeof(*fighter.parts));assert(fighter.parts);
 HSD_JObj joints[2]={{0}};fighter.parts[2].joint=&joints[0];fighter.parts[3].joint=&joints[1];
 c.u=multi;ftAction_8007121C(&gobj,&c);HitCapsule* hit=&fighter.x914[1];
 assert(c.u==multi+5&&hit_resets==1&&hit_common==1&&hit_damage==37&&hit_angle==361);
 assert(hit->jobj==&joints[0]&&hit->scale==0.003906f*256&&hit->b_offset.x==0.003906f*-32768&&hit->b_offset.y==0.003906f*-1&&hit->b_offset.z==0.003906f*32767);
 assert(hit->x24==100&&hit->x28==50&&hit->x2C==80&&hit->element==3&&hit->x34==-128&&hit->sfx_severity==2&&hit->sfx_kind==5&&!hit->hit_grabbed_victim_only);
 hit_words[0]^=1u<<19;hit_words[0]|=1u<<10;
 assert(melee_fighter_hitbox_decode(hit_words,5,1u<<19,multi));c.u=multi;ftAction_8007121C(&gobj,&c);
 assert(hit_resets==1&&hit->jobj==&joints[1]&&hit->hit_grabbed_victim_only);
 hit_words[3]|=8;assert(melee_fighter_hitbox_decode(hit_words,5,0,multi));c.u=multi;unsigned before_hit=hit_common;ftAction_8007121C(&gobj,&c);
 assert(c.u==multi+5&&hit_common==before_hit+1&&hit->hit_grabbed_victim_only);
 fighter.x1064_thrownHitbox.owner=&gobj;c.u=multi;ftAction_8007121C(&gobj,&c);assert(!hit->hit_grabbed_victim_only);
 free(fighter.parts);
 union CmdUnion saved_multi[8];memcpy(saved_multi,multi,sizeof(multi));assert(!melee_fighter_command_decode(effect,4,multi)&&!memcmp(saved_multi,multi,sizeof(multi)));
 u32 unsupported[]={11u<<26,0,0,0,0};assert(!melee_fighter_command_decode(unsupported,5,multi)&&!memcmp(saved_multi,multi,sizeof(multi)));
 assert(supported==39);puts("Fighter commands: 39 single-word and nine multiword formats plus hitbox lookahead, effect/footstep/context/hitbox handlers, original command-variable/throw handlers, signed boundaries and transactional rejection passed");
}
