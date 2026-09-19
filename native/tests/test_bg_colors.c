#include "../../src/melee/lb/lb_0219.c"
#include "melee_item_colors.h"
#include <melee/lb/lb_013B.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static GXColor displayed;
void lbBgFlash_InitState(GXColor* c){displayed=*c;}
void fn_800208B0(u8 alpha){displayed=(GXColor){0,0,0,alpha};}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);u8*bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
 MeleeArchive a;u32 root;assert(melee_archive_open(&a,bytes,size)&&melee_archive_find(&a,"lbBgFlashColAnimData",&root));
 MeleeItemColors*o=melee_item_colors_decode(&a,root,16);assert(o);struct Fighter_804D653C_t*e=melee_item_colors_entries(o);assert(!e[0].unk);
 for(unsigned i=1;i<16;i++)assert(e[i].unk&&e[i].unk4==bytes[32+root+8*i+4]);
 memset(bytes,0xa5,size);free(bytes);unsigned finished=0,changes=0;
 for(unsigned i=1;i<16;i++){ColorOverlay co={0};assert(lb_800144C8(&co,e,i,0));GXColor last={0};unsigned changed=0;
  for(unsigned frame=0;frame<600;frame++){int done=lb_80014258(NULL,&co,NULL);if(memcmp(&last,&co.x2C_hex,4)){last=co.x2C_hex;changed++;}if(done){finished++;break;}}
  assert(changed);changes+=changed;lb_80014498(&co);
 }
 /* Run the original background-flash entry points against native GObj user
  * data, including the former hard-coded +4 and +0x2c offsets. */
 BgFlashUserData* data=calloc(1,sizeof(*data));assert(data);HSD_GObj gobj={0};gobj.user_data=data;
 lbl_804D63E0=(BgFlashGlobal*)&gobj;lbl_804D63DC=e;lbl_804D63D8=1;data->x0=255;
 for(unsigned i=1;i<16;i++){
  fn_80021C1C();lbBgFlash_80021C48(i,0);assert(data->x4.x8_ptr1);
  for(unsigned frame=0;frame<600;frame++)fn_80021B04(&gobj);
 }
 fn_80021C1C();free(data);lbl_804D63E0=NULL;lbl_804D63DC=NULL;
 melee_item_colors_free(o);printf("Background flash: 15 scripts, %u completed, %u color changes, original interpreter and source disposal passed\n",finished,changes);
}
