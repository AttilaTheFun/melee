#include "melee_item_attributes.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(u8* p,u32 v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void check(MeleeArchive* a,u32 at){
    ItemAttr v;assert(melee_item_attributes_decode(a,at,&v));const u8* b=a->bytes+32+at;
    assert(b[0]==((v.x0_is_heavy<<7)|(v.x0_78<<3)|v.x0_hold_kind));
    assert(b[1]==((v.x1_1<<6)|(v.x1_3<<5)|(v.x1_4<<4)|(v.x1_5<<3)|(v.x1_67_cam_kind<<1)|v.x1_8));
    assert(v.x3==b[2]&&((u8*)&v)[3]==b[3]);
    for(unsigned i=4;i<sizeof(v);i+=4){u32 bits;memcpy(&bits,(u8*)&v+i,4);assert(bits==(((u32)b[i]<<24)|((u32)b[i+1]<<16)|((u32)b[i+2]<<8)|b[i+3]));}
}
int main(int argc,char** argv){
    size_t size=164;u8* b=calloc(1,size);assert(b);word(b,size);word(b+4,132);
    for(unsigned i=4;i<132;i+=4)word(b+32+i,0x3f800000+i*8192);
    b[34]=0x98;b[35]=0x76;MeleeArchive a;assert(melee_archive_open(&a,b,size));
    for(unsigned flags=0;flags<256;flags++){b[32]=flags;b[33]=255-flags;check(&a,0);}
    if(argc==2){free(b);FILE* f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);size=n;b=malloc(size);assert(b&&fread(b,1,size,f)==size);fclose(f);assert(melee_archive_open(&a,b,size));
        u32 root,table;MeleeHostBool present;assert(melee_archive_find(&a,"itPublicData",&root)&&melee_archive_pointer(&a,root+4,&table,&present)&&present);
        unsigned checked=0;
        for(unsigned i=0;i<43;i++){u32 article,attr;assert(melee_archive_pointer(&a,table+4*i,&article,&present));if(!present)continue;assert(melee_archive_pointer(&a,article,&attr,&present)&&present);check(&a,attr);checked++;}
        assert(checked);printf("Retail common-item attribute records: %u passed\n",checked);
    }else assert(argc==1);
    free(b);
    u8 scalar[164]={0};word(scalar,164);word(scalar+4,132);assert(melee_archive_open(&a,scalar,sizeof(scalar)));
    ItemAttr out;assert(melee_item_attributes_decode(&a,0,&out));ItemAttr saved=out;
    word(scalar+32+0x60,0x7fc00000);assert(!melee_item_attributes_decode(&a,0,&out));assert(!memcmp(&out,&saved,sizeof(out)));
    a.data_size=131;assert(!melee_item_attributes_decode(&a,0,&out));assert(!memcmp(&out,&saved,sizeof(out)));
    puts("Item attributes: exhaustive flag bytes, scalars, raw header bytes and NaN/truncation rejection passed");
}
