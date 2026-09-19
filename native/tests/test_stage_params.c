#include "melee_stage_params.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(u8* p,u32 v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static u32 be(const u8* p){return ((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];}
static void eqword(const void* p,const u8* d){u32 v;memcpy(&v,p,4);assert(v==be(d));}
static void eqhalf(const void* p,const u8* d){u16 v;memcpy(&v,p,2);assert(v==((d[0]<<8)|d[1]));}
int main(int argc,char** argv){
    size_t size=378;u8* b=calloc(1,size);assert(b);word(b,size);word(b+4,320);word(b+8,1);word(b+12,1);
    u8* d=b+32;word(d,0x3f800000);word(d+176,220);word(d+180,1);word(d+320,176);memcpy(d+332,"grGroundParam",14);
    for(unsigned i=184;i<220;i++)d[i]=i;for(unsigned i=220;i<320;i++)d[i]=i;
    if(argc==2){free(b);FILE* f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);size=n;b=malloc(size);assert(b&&fread(b,1,size,f)==size);fclose(f);}else assert(argc==1);
    MeleeArchive a;assert(melee_archive_open(&a,b,size));u32 root;assert(melee_archive_find(&a,"grGroundParam",&root));d=b+32+root;
    GroundParam* p=melee_stage_params_decode(&a);assert(p);
    /* The scalar prefix keeps disk offsets until the widened row pointer. */
    _Static_assert(offsetof(GroundParam,stage_params)==176,"Scalar prefix");
    unsigned half_offsets[]={4,8,10,46};
    for(unsigned i=0;i<4;i++)eqhalf((u8*)p+half_offsets[i],d+half_offsets[i]);
    for(unsigned i=104;i<176;i+=2)eqhalf((u8*)p+i,d+i);
    unsigned words[]={0,12,16,20,24,28,32,36,40,48,52,56,60,64,68,72,80,84,88,92,96,100};
    for(unsigned i=0;i<sizeof(words)/sizeof(*words);i++)eqword((u8*)p+words[i],d+words[i]);
    assert(p->x4C_fixed_cam==(be(d+76)!=0));assert(!memcmp(p->x6_pad,d+6,2)&&!memcmp(p->x2C_pad,d+44,2));
    assert(!memcmp(&p->xB8,d+184,36));
    u32 rows=be(d+176);for(int i=0;i<p->stage_param_count;i++){
        const u8* disk=b+32+rows+100*i;StageParam* s=p->stage_params+i;
        _Static_assert(sizeof(StageParam)==100,"Stage row scalars");
        for(unsigned k=0;k<20;k+=4)eqword((u8*)s+k,disk+k);
        for(unsigned k=20;k<100;k+=2)eqhalf((u8*)s+k,disk+k);
    }
    u32 save=be(d);word(d,0x7fc00000);assert(!melee_stage_params_decode(&a));word(d,save);
    save=be(d+180);word(d+180,257);assert(!melee_stage_params_decode(&a));word(d+180,save);
    save=be(d+176);word(d+176,a.data_size);if(p->stage_param_count)assert(!melee_stage_params_decode(&a));word(d+176,save);
    save=be(d+76);word(d+76,2);assert(!melee_stage_params_decode(&a));word(d+76,save);
    float y=p->y;int n=p->stage_param_count;memset(b,0xa5,size);free(b);assert(p->y==y);melee_stage_params_free(p);
    printf("Ground parameters: %d rows, all scalars/colors, bounds/NaN and owned lifetime passed\n",n);
}
