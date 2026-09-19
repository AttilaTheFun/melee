#include "melee_item_hurtbones.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(u8* p,u32 v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void check(MeleeArchive* a,u32 at,unsigned bones){
    ItHurtBoneList* h=melee_item_hurtbones_decode(a,at,bones);assert(h);u32 descs;MeleeHostBool present;
    assert(melee_archive_pointer(a,at+4,&descs,&present));
    for(int i=0;i<h->count;i++)for(unsigned k=0;k<8;k++){
        u32 expected,actual;assert(melee_archive_u32(a,descs+32*i+4*k,&expected));memcpy(&actual,(u8*)&h->descs[i]+4*k,4);assert(actual==expected);
    }melee_item_hurtbones_free(h);
}
int main(int argc,char** argv){
    u8 b[108]={0};word(b,sizeof(b));word(b+4,72);word(b+8,1);word(b+32,2);word(b+36,8);word(b+104,4);
    word(b+40,1);word(b+68,0x3f800000);word(b+100,0x40000000);
    MeleeArchive a;assert(melee_archive_open(&a,b,sizeof(b)));check(&a,0,2);
    assert(!melee_item_hurtbones_decode(&a,0,1));word(b+40,0);check(&a,0,0);
    ItHurtBoneList* owned=melee_item_hurtbones_decode(&a,0,0);assert(owned);
    word(b+32,3);assert(!melee_item_hurtbones_decode(&a,0,0));word(b+32,2);
    word(b+68,0xbf800000);assert(!melee_item_hurtbones_decode(&a,0,0));word(b+68,0x3f800000);
    word(b+44,0x7fc00000);assert(!melee_item_hurtbones_decode(&a,0,0));word(b+44,0);
    a.data_size=71;assert(!melee_item_hurtbones_decode(&a,0,0));
    memset(b,0xa5,sizeof(b));assert(owned->count==2&&owned->descs[1].scale==2);melee_item_hurtbones_free(owned);
    if(argc==2){FILE* f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>0);rewind(f);u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);assert(melee_archive_open(&a,bytes,size));
        u32 root,table;MeleeHostBool present;assert(melee_archive_find(&a,"itPublicData",&root)&&melee_archive_pointer(&a,root+4,&table,&present)&&present);unsigned n=0;
        for(unsigned i=0;i<43;i++){u32 article,hurt,model,bones;assert(melee_archive_pointer(&a,table+4*i,&article,&present));if(!present)continue;
            assert(melee_archive_pointer(&a,article+8,&hurt,&present));if(!present)continue;
            assert(melee_archive_pointer(&a,article+16,&model,&present)&&present&&melee_archive_u32(&a,model+4,&bones));check(&a,hurt,bones);n++;
        }assert(n);free(bytes);printf("Retail common-item hurtbone lists: %u passed\n",n);
    }else assert(argc==1);
    puts("Item hurtbones: descriptor bits, root/bone indices, capacity/radius/NaN/bounds and owned lifetime passed");
}
