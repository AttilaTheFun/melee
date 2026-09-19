#include "melee_kirby_copy.h"
#include "melee_scene.h"
#include "melee_character_parts.h"
#include "melee_item_article.h"
#include "melee_item_dynamics.h"
#include <string.h>
#include <stdlib.h>
struct MeleeKirbyCopy {KirbyHatStruct header;MeleeScene* model;MeleeCharacterParts* parts;MeleeItemArticle* items[2];MeleeScene* accessory;MeleeScene* yoshi_anims[4];ItemDynamics* dynamics;ftDynamics dynamic_header;struct ArticleDynamicBones dynamic_bones;};
KirbyHatStruct* melee_kirby_copy_descriptor(MeleeKirbyCopy*o){return o?&o->header:NULL;}
void melee_kirby_copy_free(MeleeKirbyCopy*o){if(o){for(unsigned i=0;i<4;i++)melee_scene_free(o->yoshi_anims[i]);melee_scene_free(o->accessory);melee_item_dynamics_free(o->dynamics);melee_scene_free(o->model);melee_character_parts_free(o->parts);for(unsigned i=0;i<2;i++)melee_item_article_free(o->items[i]);free(o);}}
MeleeKirbyCopy* melee_kirby_copy_decode(const MeleeArchive*a,FighterKind kind)
{
 const char*symbol;unsigned kinds[2],counts[2],count=1;
 switch(kind){
 case Ft_Kind_Yoshi:symbol="ftDataKirbyCopyYoshi";count=0;break;
 case Ft_Kind_Link:symbol="ftDataKirbyCopyLink";kinds[0]=It_Kind_Kirby_LinkArrow;kinds[1]=It_Kind_Kirby_LinkBow;counts[0]=1;counts[1]=6;count=2;break;
 case Ft_Kind_CLink:symbol="ftDataKirbyCopyClink";kinds[0]=It_Kind_Kirby_CLinkArrow;kinds[1]=It_Kind_Kirby_CLinkBow;counts[0]=1;counts[1]=6;count=2;break;
 case Ft_Kind_Koopa:symbol="ftDataKirbyCopyKoopa";kinds[0]=It_Kind_Kirby_KoopaFlame;counts[0]=1;break;
 case Ft_Kind_Pikachu:symbol="ftDataKirbyCopyPikachu";kinds[0]=It_Kind_Kirby_PikachuTJolt_Ground;kinds[1]=It_Kind_Kirby_PikachuTJolt_Air;counts[0]=2;counts[1]=1;count=2;break;
 case Ft_Kind_Pichu:symbol="ftDataKirbyCopyPichu";kinds[0]=It_Kind_Kirby_PichuTJolt_Ground;kinds[1]=It_Kind_Kirby_PichuTJolt_Air;counts[0]=2;counts[1]=1;count=2;break;
 case Ft_Kind_Samus:symbol="ftDataKirbyCopySamus";kinds[0]=It_Kind_Kirby_SamusCharge;counts[0]=9;break;
 case Ft_Kind_Popo:symbol="ftDataKirbyCopyPopo";kinds[0]=It_Kind_Kirby_IceClimberIce;counts[0]=1;break;
 case Ft_Kind_Peach:symbol="ftDataKirbyCopyPeach";kinds[0]=It_Kind_Kirby_PeachToad;kinds[1]=It_Kind_Kirby_PeachToadSpore;counts[0]=2;counts[1]=1;count=2;break;
 case Ft_Kind_Seak:symbol="ftDataKirbyCopySeak";kinds[0]=It_Kind_Kirby_SeakNeedleThrow;kinds[1]=It_Kind_Kirby_SeakNeedleHeld;counts[0]=5;counts[1]=1;count=2;break;
 case Ft_Kind_Zelda:symbol="ftDataKirbyCopyZelda";count=0;break;
 case Ft_Kind_Mars:symbol="ftDataKirbyCopyMars";count=0;break;
 case Ft_Kind_Emblem:symbol="ftDataKirbyCopyEmblem";count=0;break;
 case Ft_Kind_Captain:symbol="ftDataKirbyCopyCaptain";count=0;break;
 case Ft_Kind_Ganon:symbol="ftDataKirbyCopyGanon";count=0;break;
 case Ft_Kind_Fox:symbol="ftDataKirbyCopyFox";kinds[0]=It_Kind_Kirby_FoxLaser;kinds[1]=It_Kind_Kirby_FoxBlaster;counts[0]=2;counts[1]=9;count=2;break;
 case Ft_Kind_Ness:symbol="ftDataKirbyCopyNess";kinds[0]=It_Kind_Kirby_NessPKFlush;kinds[1]=It_Kind_Kirby_NessPKFlush_Explode;counts[0]=3;counts[1]=1;count=2;break;
 case Ft_Kind_Mario:symbol="ftDataKirbyCopyMario";kinds[0]=It_Kind_Kirby_MarioFire;counts[0]=1;break;
 case Ft_Kind_DrMario:symbol="ftDataKirbyCopyDrmario";kinds[0]=It_Kind_Kirby_DrMarioVitamin;counts[0]=6;break;
 case Ft_Kind_Luigi:symbol="ftDataKirbyCopyLuigi";kinds[0]=It_Kind_Kirby_LuigiFire;counts[0]=1;break;
 default:return NULL;
 }
 u32 root,joint;MeleeHostBool present;
 if(!a||!melee_archive_find(a,symbol,&root)||!melee_archive_pointer(a,root,&joint,&present)||!present)return NULL;
 MeleeKirbyCopy*o=calloc(1,sizeof(*o));if(!o)return NULL;
 o->model=melee_scene_decode(a,joint);if(!o->model)goto fail;
 o->header.hat_joint=melee_scene_joint_descriptor(o->model);melee_scene_release_objects(o->model);
 o->parts=melee_character_visibility_decode(a,root+4,1);if(!o->parts)goto fail;
 o->header.desc=*melee_character_visibility_descriptor(o->parts);
 if(kind==Ft_Kind_Mars||kind==Ft_Kind_Emblem||kind==Ft_Kind_Zelda||kind==Ft_Kind_Seak||kind==Ft_Kind_Pikachu||kind==Ft_Kind_Pichu||kind==Ft_Kind_Koopa||kind==Ft_Kind_Link||kind==Ft_Kind_CLink){
  u32 at,selector;
  if(kind==Ft_Kind_Mars||kind==Ft_Kind_Emblem){
  if(!melee_archive_pointer(a,root+12,&at,&present)||!present)goto fail;
  o->accessory=melee_scene_decode(a,at);if(!o->accessory)goto fail;
  o->header.hat_dynamics[0]=(ftDynamics*)melee_scene_joint_descriptor(o->accessory);melee_scene_release_objects(o->accessory);
  }
  unsigned slot=kind==Ft_Kind_Zelda?0:(kind==Ft_Kind_Seak||kind==Ft_Kind_Pikachu||kind==Ft_Kind_Pichu||kind==Ft_Kind_Link||kind==Ft_Kind_CLink)?2:1;
  if(!melee_archive_pointer(a,root+12+4*slot,&at,&present)||!present)goto fail;
  o->dynamics=melee_item_dynamics_decode(a,at,melee_scene_joint_count(o->model));
  if(!o->dynamics||o->dynamics->count>Ft_Dynamics_NumMax||o->dynamics->collision_count)goto fail;
  /* These hats use chains only; reject an unhandled selector table. */
  if(!melee_archive_pointer(a,at+16,&selector,&present)||present)goto fail;
  o->dynamic_header.dynamicsNum=o->dynamics->count;o->dynamic_header.ftDynamicBones=&o->dynamic_bones;
  memcpy(o->dynamic_bones.array,o->dynamics->dyn_descs,o->dynamics->count*sizeof(BoneDynamicsDesc));
  o->header.hat_dynamics[slot]=&o->dynamic_header;
 }
 if(kind==Ft_Kind_Popo){
  u32 at;if(!melee_archive_pointer(a,root+16,&at,&present)||!present)goto fail;
  o->accessory=melee_scene_decode(a,at);if(!o->accessory)goto fail;
  o->header.hat_dynamics[1]=(ftDynamics*)melee_scene_joint_descriptor(o->accessory);melee_scene_release_objects(o->accessory);
 }
 if(kind==Ft_Kind_Yoshi){
  u32 capture,article;
  if(!melee_archive_pointer(a,root+12,&capture,&present)||!present)goto fail;
  o->accessory=melee_scene_decode(a,capture);if(!o->accessory)goto fail;
  o->header.hat_dynamics[0]=(ftDynamics*)melee_scene_joint_descriptor(o->accessory);melee_scene_release_objects(o->accessory);
  for(unsigned i=1;i<=4;i++){
   u32 anim;if(!melee_archive_pointer(a,root+12+4*i,&anim,&present)||!present)goto fail;
   o->yoshi_anims[i-1]=melee_scene_decode(a,i%2?capture:joint);if(!o->yoshi_anims[i-1])goto fail;
   if(!melee_scene_bind_joints(o->yoshi_anims[i-1],anim))goto fail;
   o->header.hat_dynamics[i]=(ftDynamics*)melee_scene_animation_descriptor(o->yoshi_anims[i-1]);melee_scene_release_objects(o->yoshi_anims[i-1]);
  }
  if(!melee_archive_pointer(a,root+32,&article,&present)||!present)goto fail;
  o->items[0]=melee_item_article_decode(a,It_Kind_Kirby_YoshiEggLay,article,0);if(!o->items[0])goto fail;
  o->header.hat_dynamics[5]=(ftDynamics*)melee_item_article_descriptor(o->items[0]);
 }
 for(unsigned i=0;i<count;i++){
  u32 article;if(!melee_archive_pointer(a,root+12+4*i,&article,&present)||!present)goto fail;
  o->items[i]=melee_item_article_decode(a,kinds[i],article,counts[i]);if(!o->items[i])goto fail;
  /* Legacy hat fields contain articles in these slots, not dynamics. */
  o->header.hat_dynamics[i]=(ftDynamics*)melee_item_article_descriptor(o->items[i]);
 }
 return o;
fail:melee_kirby_copy_free(o);return NULL;
}

MeleeKirbyCopy* melee_kirby_copy_ness_decode(const MeleeArchive*a){return melee_kirby_copy_decode(a,Ft_Kind_Ness);}
