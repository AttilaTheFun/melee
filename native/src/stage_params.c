#include "melee_stage_params.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(((StageParam*)0)->stkind)==4,"Stage kind width");
_Static_assert(sizeof(((GroundParam*)0)->xC)==4,"Ground scalar width");
static int span(const MeleeArchive* a,uint32_t at,size_t n){return at<=a->data_size&&n<=a->data_size-at;}
static void half(const MeleeArchive* a,uint32_t at,s16* out){const u8* p=a->bytes+32+at;u16 bits=(p[0]<<8)|p[1];memcpy(out,&bits,2);}
void melee_stage_params_free(GroundParam* p){if(p){free(p->stage_params);free(p);}}
GroundParam* melee_stage_params_decode(const MeleeArchive* a){
    u32 root,rows,n;MeleeHostBool present;
    if(!a||!melee_archive_find(a,"grGroundParam",&root)||!span(a,root,0xdc)||
       !melee_archive_u32(a,root+0xb4,&n)||n>256||
       !melee_archive_pointer(a,root+0xb0,&rows,&present)||(n&&(!present||!span(a,rows,100u*n))))return NULL;
    GroundParam* p=calloc(1,sizeof(*p));if(!p)return NULL;
#define F(field,off) if(!melee_archive_f32(a,root+(off),&p->field)||!isfinite(p->field))goto fail
#define W(field,off) do {u32 bits;melee_archive_u32(a,root+(off),&bits);memcpy(&p->field,&bits,4);}while(0)
#define H(field,off) half(a,root+(off),&p->field)
    F(y,0);H(x4,4);memcpy(p->x6_pad,a->bytes+32+root+6,2);H(x8,8);H(xA,10);
    W(xC,12);W(x10,16);W(x14,20);F(x18,24);F(x1C,28);F(x20,32);F(x24,36);F(x28,40);
    memcpy(p->x2C_pad,a->bytes+32+root+44,2);H(x2E,46);W(x30,48);W(x34,52);W(x38,56);
    F(x3C,60);F(x40,64);F(x44,68);F(x48,72);
    u32 fixed;melee_archive_u32(a,root+76,&fixed);if(fixed>1)goto fail;p->x4C_fixed_cam=fixed!=0;
    F(x50,80);F(x54,84);F(x58,88);F(x5C,92);F(x60,96);F(x64,100);H(x68,104);
    for(unsigned i=0;i<35;i++)half(a,root+106+2*i,&p->x6A[i]);
    p->stage_param_count=n;p->stage_params=calloc(n?n:1,sizeof(*p->stage_params));if(!p->stage_params)goto fail;
    for(unsigned i=0;i<n;i++){
        StageParam* s=p->stage_params+i;u32 at=rows+100*i,bits;
#define ROW(field,off) do{melee_archive_u32(a,at+(off),&bits);memcpy(&s->field,&bits,4);}while(0)
        ROW(stkind,0);ROW(x4,4);ROW(x8,8);ROW(xC,12);ROW(x10,16);
        half(a,at+20,&s->x14);half(a,at+22,&s->x16);half(a,at+24,&s->x18);
        for(unsigned k=0;k<37;k++)half(a,at+26+2*k,&s->x1A[k]);
    }
#define COLOR(field,off) memcpy(&p->field,a->bytes+32+root+(off),4)
    COLOR(xB8,184);COLOR(xBC,188);COLOR(xC0,192);COLOR(xC4,196);COLOR(xC8,200);
    COLOR(xCC,204);COLOR(xD0,208);COLOR(xD4,212);COLOR(xD8,216);
    return p;
fail:melee_stage_params_free(p);return NULL;
}
