#include "melee_kirby_composite_copy.h"
#include "melee_character_parts.h"
#include "melee_scene.h"
#include "melee_item_article.h"
#include "melee_item_dynamics.h"
#include <melee/ft/dobjlist.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
struct MeleeKirbyCompositeCopy {
 FighterKind kind;MeleeKirbyCompositeCopyDesc header;MeleeCharacterParts* parts;MeleeScene* shared;
 ItemDynamics* dynamics;ftDynamics dynamic_header;struct ArticleDynamicBones dynamic_bones;
 MeleeItemArticle* items[2];u16* textures[6];MeleeScene* costumes[6];
};
MeleeKirbyCompositeCopyDesc* melee_kirby_composite_copy_descriptor(MeleeKirbyCompositeCopy*o){return o?&o->header:NULL;}
void melee_kirby_composite_copy_free(MeleeKirbyCompositeCopy*o){if(o){if(o->header.outline[0].x4)for(int i=0;i<o->header.outline[0].x0;i++)free(o->header.outline[0].x4[i].x4);free(o->header.outline[0].x4);melee_item_dynamics_free(o->dynamics);melee_character_parts_free(o->parts);melee_scene_free(o->shared);for(unsigned i=0;i<2;i++)melee_item_article_free(o->items[i]);for(unsigned i=0;i<6;i++){free(o->textures[i]);melee_scene_free(o->costumes[i]);}free(o);}}
MeleeKirbyCompositeCopy* melee_kirby_composite_copy_decode(const MeleeArchive*a,FighterKind kind)
{
 u32 root,table,joint;MeleeHostBool present;
 const char* symbol;
 if(kind==Ft_Kind_Falco)symbol="ftDataKirbyCopyFalco";
 else if(kind==Ft_Kind_Donkey)symbol="ftDataKirbyCopyDonkey";
 else if(kind==Ft_Kind_Mewtwo)symbol="ftDataKirbyCopyMewtwo";
 else if(kind==Ft_Kind_Purin)symbol="ftDataKirbyCopyPurin";
 else if(kind==Ft_Kind_GameWatch)symbol="ftDataKirbyCopyGamewatch";
 else return NULL;
 if(!a||!melee_archive_find(a,symbol,&root))return NULL;
 MeleeKirbyCompositeCopy*o=calloc(1,sizeof(*o));if(!o)return NULL;o->kind=kind;
 o->parts=melee_character_visibility_decode(a,root,6);if(!o->parts)goto fail;
 o->header.parts=*melee_character_visibility_descriptor(o->parts);
 if(!melee_archive_u32(a,root+8,&o->header.textures.x8)||o->header.textures.x8!=2||
    !melee_archive_pointer(a,root+12,&table,&present)||!present||
    !melee_archive_u32(a,root+16,&o->header.replacement_mask))goto fail;
 o->header.textures.xC=o->textures;
 for(unsigned c=0;c<6;c++){
  u32 at;if(!melee_archive_pointer(a,table+4*c,&at,&present))goto fail;if(!present)continue;
  if(at>a->data_size||4>a->data_size-at)goto fail;
  o->textures[c]=malloc(4);if(!o->textures[c])goto fail;
  for(unsigned j=0;j<2;j++)o->textures[c][j]=((u16)a->bytes[32+at+2*j]<<8)|a->bytes[33+at+2*j];
 }
 if(!melee_archive_pointer(a,root+20,&joint,&present))goto fail;
 if(present){o->shared=melee_scene_decode(a,joint);if(!o->shared)goto fail;
 o->header.shared_joint=melee_scene_joint_descriptor(o->shared);melee_scene_release_objects(o->shared);}
 else if(kind!=Ft_Kind_GameWatch)goto fail;
 const unsigned kinds[]={kind==Ft_Kind_GameWatch?It_Kind_Kirby_GameWatchChef:kind==Ft_Kind_Mewtwo?It_Kind_Kirby_MewtwoShadowBall:It_Kind_Kirby_FalcoLaser,kind==Ft_Kind_GameWatch?It_Kind_Kirby_GameWatchChefPan:It_Kind_Kirby_FalcoBlaster},counts[]={kind==Ft_Kind_Mewtwo?10u:2u,kind==Ft_Kind_GameWatch?0u:9u};
 for(unsigned i=0;i<((kind==Ft_Kind_Falco||kind==Ft_Kind_GameWatch)?2u:kind==Ft_Kind_Mewtwo?1u:0u);i++){
  u32 at;if(!melee_archive_pointer(a,root+(kind==Ft_Kind_GameWatch?32:24)+4*i,&at,&present)||!present)goto fail;
  o->items[i]=melee_item_article_decode(a,kinds[i],at,counts[i]);if(!o->items[i])goto fail;
 }
 if(kind==Ft_Kind_Mewtwo||kind==Ft_Kind_Purin){
  u32 at,selector;if(!melee_archive_pointer(a,root+(kind==Ft_Kind_Purin?24:28),&at,&present)||!present)goto fail;
  o->dynamics=melee_item_dynamics_decode(a,at,46);
  if(!o->dynamics||o->dynamics->count>=Ft_Dynamics_NumMax||o->dynamics->collision_count)goto fail;
  if(!melee_archive_pointer(a,at+16,&selector,&present)||present)goto fail;
  o->dynamic_header.dynamicsNum=o->dynamics->count;o->dynamic_header.ftDynamicBones=&o->dynamic_bones;
  memcpy(o->dynamic_bones.array,o->dynamics->dyn_descs,o->dynamics->count*sizeof(BoneDynamicsDesc));
  o->header.dynamics=&o->dynamic_header;
 }
 if(kind==Ft_Kind_GameWatch){
  u32 outline,choices,count,colors;
  if(!melee_archive_pointer(a,root+24,&outline,&present)||!present||!melee_archive_u32(a,outline,&count)||count>128||
     !melee_archive_pointer(a,outline+4,&choices,&present)||(count&&!present))goto fail;
  o->header.outline[0].x0=count;o->header.outline[0].x4=calloc(count?count:1,sizeof(TempS));if(!o->header.outline[0].x4)goto fail;
  for(unsigned i=0;i<count;i++){
   u32 n,indices;if(!melee_archive_u32(a,choices+8*i,&n)||n>124||!melee_archive_pointer(a,choices+8*i+4,&indices,&present)||(n&&!present)||indices>a->data_size||n>a->data_size-indices)goto fail;
   TempS*t=&o->header.outline[0].x4[i];t->x0=n;t->x4=malloc(n?n:1);if(!t->x4)goto fail;memcpy(t->x4,a->bytes+32+indices,n);
   for(unsigned j=0;j<n;j++)if(t->x4[j]>=124)goto fail;
  }
  if(!melee_archive_pointer(a,root+28,&colors,&present)||!present||colors>a->data_size||12>a->data_size-colors||
     !melee_archive_f32(a,colors,&o->header.model_depth)||!isfinite(o->header.model_depth))goto fail;
  memcpy(o->header.fill_rgba,a->bytes+32+colors+4,4);memcpy(o->header.outline_rgba,a->bytes+32+colors+8,4);
 }
 o->header.laser=melee_item_article_descriptor(o->items[0]);o->header.blaster=melee_item_article_descriptor(o->items[1]);return o;
fail:melee_kirby_composite_copy_free(o);return NULL;
}

MeleeHostBool melee_kirby_composite_copy_bind_costume(MeleeKirbyCompositeCopy*o,const MeleeArchive*a,unsigned costume)
{
 if(!o||!a||costume>=6)return false;
 const char*suffix[]={"","Ye","Bu","Re","Gr","Wh"};char name[80];u32 joint,material;
 snprintf(name,sizeof(name),"PlyKirby%s%s_Share_joint",o->kind==Ft_Kind_Falco?"Fc":o->kind==Ft_Kind_Mewtwo?"Mt":o->kind==Ft_Kind_Purin?"Pr":o->kind==Ft_Kind_GameWatch?"Gw":"Dk",o->kind==Ft_Kind_GameWatch?"":suffix[costume]);if(!melee_archive_find(a,name,&joint))return false;
 snprintf(name,sizeof(name),"PlyKirby%s%s_Share_matanim_joint",o->kind==Ft_Kind_Falco?"Fc":o->kind==Ft_Kind_Mewtwo?"Mt":o->kind==Ft_Kind_Purin?"Pr":o->kind==Ft_Kind_GameWatch?"Gw":"Dk",o->kind==Ft_Kind_GameWatch?"":suffix[costume]);if(!melee_archive_find(a,name,&material))return false;
 MeleeScene*fresh=melee_scene_decode(a,joint);if(!fresh)return false;
 if(!melee_scene_bind_materials(fresh,material)){melee_scene_free(fresh);return false;}
 melee_scene_release_objects(fresh);melee_scene_free(o->costumes[costume]);o->costumes[costume]=fresh;
 o->header.costume_joints[costume]=melee_scene_joint_descriptor(fresh);o->header.costume_materials[costume]=melee_scene_material_descriptor(fresh);return true;
}
