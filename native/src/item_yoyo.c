#include "melee_item_yoyo.h"
#include "melee_scene.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
struct MeleeItemYoyo {itYoyoAttributes attrs;MeleeScene* scenes[2];};
_Static_assert(offsetof(itYoyoAttributes,x50_string_joint)==0x50,"Yoyo scalar prefix");
void melee_item_yoyo_free(MeleeItemYoyo* o){if(o){for(unsigned i=0;i<2;i++)melee_scene_free(o->scenes[i]);free(o);}}
itYoyoAttributes* melee_item_yoyo_attributes(MeleeItemYoyo* o){return o?&o->attrs:NULL;}
MeleeItemYoyo* melee_item_yoyo_decode(const MeleeArchive* a,u32 at){
 if(!a||(uint64_t)at+0x60>a->data_size)return NULL;
 MeleeItemYoyo* o=calloc(1,sizeof(*o));if(!o)return NULL;
 for(unsigned i=0;i<20;i++){u32 bits;if(!melee_archive_u32(a,at+4*i,&bits))goto fail;
  if(i>=3&&i<16){float f;memcpy(&f,&bits,4);if(!isfinite(f))goto fail;}
  memcpy((u8*)&o->attrs+4*i,&bits,4);
 }
 for(unsigned i=0;i<2;i++){u32 joint;MeleeHostBool p;
  if(!melee_archive_pointer(a,at+0x50+4*i,&joint,&p)||!p)goto fail;
  o->scenes[i]=melee_scene_decode(a,joint);if(!o->scenes[i])goto fail;
 }
 u32 mat,last;MeleeHostBool p;
 if(!melee_archive_pointer(a,at+0x58,&mat,&p)||!melee_archive_u32(a,at+0x5c,&last))goto fail;
 if(p&&!melee_scene_bind_materials(o->scenes[1],mat))goto fail;
 o->attrs.x50_string_joint=melee_scene_joint_descriptor(o->scenes[0]);
 o->attrs.x54_yoyo_joint=melee_scene_joint_descriptor(o->scenes[1]);
 o->attrs.x58_yoyo_matanim=melee_scene_material_descriptor(o->scenes[1]);
 memcpy(&o->attrs.x5C_UNK7,&last,4);
 for(unsigned i=0;i<2;i++)melee_scene_release_objects(o->scenes[i]);return o;
fail:melee_item_yoyo_free(o);return NULL;
}
