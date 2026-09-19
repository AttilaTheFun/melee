#include "melee_character_part_anims.h"
#include "melee_animation.h"
#include <sysdolphin/baselib/aobj.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
typedef struct Node {struct Node* allocation_next;HSD_AnimJoint joint;HSD_AObjDesc aobj;MeleeAnimationTracks* tracks;u32 offset;} Node;
struct MeleeCharacterPartAnims {struct ftData_x1C* groups[5];Node* nodes;unsigned node_count;};
static int ref(const MeleeArchive*a,u32 slot,u32* at,MeleeHostBool* present,size_t size)
{return melee_archive_pointer(a,slot,at,present)&&(!*present||(*at<=a->data_size&&size<=a->data_size-*at));}
void melee_character_part_anims_free(MeleeCharacterPartAnims* o){if(!o)return;for(unsigned i=0;i<5;i++)if(o->groups[i]){free(o->groups[i]->x4);free(o->groups[i]->x8);free(o->groups[i]);}while(o->nodes){Node*n=o->nodes;o->nodes=n->allocation_next;melee_animation_tracks_free(n->tracks);free(n);}free(o);}
void melee_character_part_anims_bind(MeleeCharacterPartAnims*o,ftData*d){if(o&&d)d->x1C=o->groups;}
static int tree(MeleeCharacterPartAnims*o,const MeleeArchive*a,u32 at,HSD_AnimJoint**out,u32* ancestors,unsigned depth)
{
 if(depth>=256||o->node_count>=4096)return 0;
 for(unsigned i=0;i<depth;i++)if(ancestors[i]==at)return 0;ancestors[depth]=at;
 u32 child,next,anim,robj,flags;MeleeHostBool cp,np,ap,rp;
 if(!ref(a,at,&child,&cp,20)||!ref(a,at+4,&next,&np,20)||!ref(a,at+8,&anim,&ap,16)||!ref(a,at+12,&robj,&rp,8)||rp||!melee_archive_u32(a,at+16,&flags)||(flags&~1u))return 0;
 Node*n=calloc(1,sizeof(*n));if(!n)return 0;n->allocation_next=o->nodes;o->nodes=n;o->node_count++;n->offset=at;n->joint.flags=flags;*out=&n->joint;
 if(ap){u32 track,id;MeleeHostBool tp;
  if(!melee_archive_u32(a,anim,&n->aobj.flags)||!melee_archive_f32(a,anim+4,&n->aobj.end_frame)||!isfinite(n->aobj.end_frame)||n->aobj.end_frame<0||!ref(a,anim+8,&track,&tp,12)||!tp||!melee_archive_u32(a,anim+12,&id)||id)return 0;
  n->tracks=melee_animation_tracks_decode(a,track);if(!n->tracks)return 0;n->aobj.fobjdesc=melee_animation_tracks_descriptors(n->tracks);n->joint.aobjdesc=&n->aobj;
  for(HSD_FObjDesc*f=n->aobj.fobjdesc;f;f=f->next)if(!((f->type>=1&&f->type<=12&&f->type!=4)||(f->type>=40&&f->type<=42)))return 0;
 }
 return (!cp||tree(o,a,child,&n->joint.child,ancestors,depth+1))&&(!np||tree(o,a,next,&n->joint.next,ancestors,depth+1));
}
static MeleeCharacterPartAnims* decode(const MeleeArchive*a,u32 root,unsigned joints,const unsigned*counts,unsigned groups)
{
 if(!a||!joints||joints>140||!counts||!groups||groups>5)return NULL;
 u32 table;MeleeHostBool p;if(!ref(a,root+28,&table,&p,groups*4)||!p)return NULL;
 MeleeCharacterPartAnims*o=calloc(1,sizeof(*o));if(!o)return NULL;
 for(unsigned i=0;i<groups;i++){
  if(!counts[i]||counts[i]>128)goto fail;u32 at,packed,parts,anims;
  if(!ref(a,table+i*4,&at,&p,12)||!p||!melee_archive_u32(a,at,&packed)||(packed>>16)>=joints||(packed&65535)>joints||!ref(a,at+4,&parts,&p,packed&65535)||!p||!ref(a,at+8,&anims,&p,counts[i]*4)||!p)goto fail;
  struct ftData_x1C*d=calloc(1,sizeof(*d));if(!d)goto fail;o->groups[i]=d;d->x0=packed>>16;d->x2=packed&65535;d->x4=malloc(d->x2?d->x2:1);d->x8=calloc(counts[i],sizeof(*d->x8));if(!d->x4||!d->x8)goto fail;
  memcpy(d->x4,a->bytes+32+parts,d->x2);for(unsigned j=0;j<d->x2;j++)if(d->x4[j]>=joints)goto fail;
  for(unsigned j=0;j<counts[i];j++){
   u32 anim;
   if(!ref(a,anims+j*4,&anim,&p,20))goto fail;
   if(!p)continue;u32 ancestors[256];if(!tree(o,a,anim,&d->x8[j],ancestors,0))goto fail;
  }
 }
 return o;
fail:melee_character_part_anims_free(o);return NULL;
}

MeleeCharacterPartAnims* melee_character_part_anims_decode(const MeleeArchive*a,u32 root,unsigned joints,const unsigned*counts,unsigned groups)
{
 /* Match lbArchive_InitializeDAT: external animation references are null.
  * In particular Kirby's middle hand poses are external chain terminators,
  * not runtime pointers. The decoded trees own all remaining track data. */
 if(!a)return NULL;
 if(!a->extern_count)return decode(a,root,joints,counts,groups);
 size_t size;uint8_t* bytes=melee_archive_copy_null_externals(a,&size);
 if(!bytes)return NULL;
 MeleeArchive resolved;MeleeCharacterPartAnims* result=NULL;
 if(melee_archive_open(&resolved,bytes,size))result=decode(&resolved,root,joints,counts,groups);
 free(bytes);return result;
}
