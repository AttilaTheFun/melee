#include "melee_item_dynamics.h"
#include <melee/lb/types.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void lb_80011710(DynamicsDesc*,DynamicsDesc*);
static void word(u8* p,u32 v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void bits(MeleeArchive* a,u32 at,const void* out,unsigned n){for(unsigned i=0;i<n;i++){u32 expected,v;assert(melee_archive_u32(a,at+4*i,&expected));memcpy(&v,(u8*)out+4*i,4);assert(v==expected);}}
static void check(MeleeArchive* a,u32 at,unsigned bones){
    ItemDynamics* d=melee_item_dynamics_decode(a,at,bones);assert(d);u32 ds,cs;MeleeHostBool p;
    assert(melee_archive_pointer(a,at+4,&ds,&p)&&melee_archive_pointer(a,at+12,&cs,&p));
    for(int i=0;i<d->count;i++){
        BoneDynamicsDesc* b=d->dyn_descs+i;bits(a,ds+24*i,&b->bone_id,1);bits(a,ds+24*i+8,&b->dyn_desc.count,1);bits(a,ds+24*i+12,&b->dyn_desc.pos,3);
        u32 params;assert(melee_archive_pointer(a,ds+24*i+4,&params,&p));
        unsigned count=b->dyn_desc.count;
        struct DynamicsData* nodes=calloc(count?count:1,sizeof(*nodes));assert(nodes);
        for(unsigned j=0;j<count;j++){nodes[j].next=j+1<count?&nodes[j+1]:NULL;nodes[j].desc.lb_unk0.unk_48=2;}
        DynamicsDesc runtime={.data=nodes,.count=count};lb_80011710(&b->dyn_desc,&runtime);
        assert(!memcmp(&runtime.pos,&b->dyn_desc.pos,sizeof(runtime.pos)));
        for(unsigned j=0;j<count;j++){
            bits(a,params+60*j,b->dyn_desc.params+j,15);
            assert(!memcmp(&nodes[j].desc.lb_unk0.unk_4C,b->dyn_desc.params+j,8));
            assert(!memcmp(&nodes[j].desc.lb_unk0.unk_58,(u8*)(b->dyn_desc.params+j)+8,52));
        }free(nodes);
    }
    for(int i=0;i<d->collision_count;i++)bits(a,cs+20*i,d->collision_descs+i,5);
    melee_item_dynamics_free(d);
}
int main(int argc,char** argv){
    u8 b[164]={0};word(b,164);word(b+4,120);word(b+8,3);u8* d=b+32;
    word(d,1);word(d+4,16);word(d+8,1);word(d+12,100);word(d+16,2);word(d+20,40);word(d+24,1);
    for(unsigned i=40;i<100;i+=4)word(d+i,0x3f800000);word(d+100,3);word(d+116,0x3f800000);
    word(b+152,4);word(b+156,12);word(b+160,20);MeleeArchive a;assert(melee_archive_open(&a,b,sizeof(b)));check(&a,0,4);
    ItemDynamics* owned=melee_item_dynamics_decode(&a,0,4);assert(owned);
    assert(!melee_item_dynamics_decode(&a,0,3));word(d,25);assert(!melee_item_dynamics_decode(&a,0,4));word(d,1);
    word(d+8,3);assert(!melee_item_dynamics_decode(&a,0,4));word(d+8,1);
    word(d+40,0x7fc00000);assert(!melee_item_dynamics_decode(&a,0,4));word(d+40,0x3f800000);
    a.data_size=119;assert(!melee_item_dynamics_decode(&a,0,4));memset(b,0xa5,sizeof(b));assert(owned->dyn_descs[0].dyn_desc.params[0].unk_0==1);melee_item_dynamics_free(owned);
    if(argc==2){FILE* f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>0);rewind(f);u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);assert(melee_archive_open(&a,bytes,size));
        u32 root,table;MeleeHostBool present;assert(melee_archive_find(&a,"itPublicData",&root)&&melee_archive_pointer(&a,root+4,&table,&present)&&present);unsigned n=0;
        for(unsigned i=0;i<43;i++){u32 article,dynamics,model,bones;assert(melee_archive_pointer(&a,table+4*i,&article,&present));if(!present)continue;
            assert(melee_archive_pointer(&a,article+20,&dynamics,&present));if(!present)continue;
            assert(melee_archive_pointer(&a,article+16,&model,&present)&&present&&melee_archive_u32(&a,model+4,&bones));check(&a,dynamics,bones);n++;
        }assert(n);free(bytes);printf("Retail common-item dynamics records: %u passed\n",n);
    }else assert(argc==1);
    puts("Item dynamics: native sections, every parameter scalar, indices/capacities/NaN/bounds and owned lifetime passed");
}
