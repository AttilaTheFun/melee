#include "melee_model.h"
#include "melee_animation.h"
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t bytes[32+512+64];
static void word(uint8_t* p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void number(uint8_t* p,float f){uint32_t u;memcpy(&u,&f,4);word(p,u);}
static void fixture(void)
{
    memset(bytes,0,sizeof(bytes));word(bytes,sizeof(bytes));word(bytes+4,512);word(bytes+8,16);
    uint8_t* d=bytes+32;word(d+4,2);word(d+16,64);number(d+32,1);number(d+36,1);number(d+40,1);number(d+44,3);
    word(d+76,80);word(d+88,104);word(d+92,1);word(d+96,184);
    word(d+104,9);word(d+108,1);word(d+112,1);word(d+128,255);
    d[184]=0xb8;d[186]=1;d[187]=2;
    const uint32_t slots[]={16,76,88,96,16,16};for(unsigned i=0;i<16;i++)word(d+512+i*4,slots[i%6]);
}
static MeleeModel* decode(void){MeleeArchive a;assert(melee_archive_open(&a,bytes,sizeof(bytes)));return melee_model_decode(&a,0);}
static void pools(void){assert(!HSD_AObjGetAllocData()->used && !HSD_FObjGetAllocData()->used && !HSD_VecGetAllocData()->used);}
int main(void)
{
    static _Alignas(32) uint8_t heap[1024*1024];assert(OSInitAlloc(heap,heap+sizeof(heap),1));HSD_SetHeap(OSCreateHeap(heap,heap+sizeof(heap)));
    HSD_AObjInitAllocData();HSD_FObjInitAllocData();HSD_VecInitAllocData();
    fixture();MeleeModel* model=decode();assert(model && melee_model_part_count(model)==1);
    const MeleeModelPart* p=melee_model_part(model,0);assert(p && p->vertex_count==1 && p->draw_count==1);
    assert(p->vertices[0].position[0]==5 && p->polygon_offset==80 && p->material_offset==UINT32_MAX);
    memset(bytes,0,sizeof(bytes));assert(melee_model_step(model) && p->vertices[0].position[0]==5);
    uint8_t anim[108]={0};word(anim,sizeof(anim));word(anim+4,64);word(anim+8,3);
    uint8_t* a=anim+32;word(a,1);number(a+8,10);word(a+12,20);word(a+16,24);a[20]=1;a[21]=255;
    a[25]=5;a[28]=5;a[29]=HSD_A_FRAC_U8;word(a+32,36);memcpy(a+36,(uint8_t[]){0x12,0,10,10,10},5);
    word(a+64,12);word(a+68,16);word(a+72,32);MeleeArchive archive;assert(melee_archive_open(&archive,anim,sizeof(anim)));
    MeleeFighterAnimation* motion=melee_fighter_animation_decode(&archive,0);assert(motion);
    assert(melee_model_bind(model,motion) && melee_model_request(model,5) && melee_model_step(model));assert(p->vertices[0].position[0]==7);
    assert(melee_model_step(model) && p->vertices[0].position[0]==8);
    melee_model_free(model);melee_fighter_animation_free(motion);pools();
    /* Separate drawables may legitimately share a polygon chain. */
    fixture();uint8_t* d=bytes+32;word(d+68,216);word(d+228,80);word(d+512+16,68);word(d+512+20,228);
    model=decode();assert(model && melee_model_part_count(model)==2);
    assert(melee_model_part(model,0)->drawable_offset==64 && melee_model_part(model,1)->drawable_offset==216);
    assert(melee_model_drawable_count(model)==2 && melee_model_part(model,1)->drawable_index==1);
    assert(melee_model_set_drawable_hidden(model,0,true) && melee_model_step(model));
    assert(melee_model_part(model,0)->hidden && !melee_model_part(model,1)->hidden);
    assert(melee_model_set_drawable_hidden(model,0,false) && !melee_model_part(model,0)->hidden);
    assert(!melee_model_set_drawable_hidden(model,2,true));
    melee_model_free(model);pools();
    /* Root, child, grandchild, sibling: visibility indices are DFS, not the
     * archive decoder's discovery order. Shared PObjs remain separate DObjs. */
    fixture();word(d+8,256);word(d+264,384);word(d+268,320);word(d+272,216);
    word(d+336,464);word(d+400,480);word(d+228,80);word(d+476,80);word(d+492,80);
    for(unsigned at=256;at<=384;at+=64)for(unsigned axis=0;axis<3;axis++)number(d+at+32+axis*4,1);
    const uint32_t extra[]={8,264,268,272,336,400,228,476,492};
    for(unsigned i=0;i<9;i++)word(d+512+(6+i)*4,extra[i]);
    model=decode();assert(model && melee_model_drawable_count(model)==4);
    const uint32_t order[]={64,216,480,464};
    for(unsigned i=0;i<4;i++)assert(melee_model_part(model,i)->drawable_offset==order[i] && melee_model_part(model,i)->drawable_index==i);
    melee_model_free(model);pools();
    fixture();word(d+68,216);word(d+512+16,68); /* Empty DObj still has an index. */
    model=decode();assert(model && melee_model_drawable_count(model)==2 && melee_model_part_count(model)==1);
    melee_model_free(model);pools();
    fixture();word(d+4,2|16);model=decode();assert(model);
    assert(melee_model_set_drawable_hidden(model,0,false) && melee_model_part(model,0)->hidden);
    melee_model_free(model);pools();
    fixture();word(d+68,64);word(d+512+16,68);assert(!decode());pools();
    fixture();word(d+84,80);word(d+512+16,84);assert(!decode());pools();
    fixture();d[185]=255;d[186]=255;assert(!decode());pools();
    fixture();word(d+92,0x10000001);assert(!decode());pools();
    assert(!melee_model_decode(NULL,0));melee_model_free(NULL);
    puts("Owned model loader: archive independence, animated skin updates, shared geometry, cycle rejection and cleanup passed");
}
