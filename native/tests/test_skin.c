#include "melee_skin.h"
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static uint8_t bytes[32+416+36];
static void word(uint8_t* p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void number(uint8_t* p,float v){uint32_t bits;memcpy(&bits,&v,4);word(p,bits);}
static void near(float a,float b){assert(fabsf(a-b)<0.0001f);}
static void fixture(void)
{
    memset(bytes,0,sizeof(bytes));word(bytes,sizeof(bytes));word(bytes+4,416);word(bytes+8,9);
    uint8_t* d=bytes+32;word(d+8,64);word(d+4,JOBJ_SKELETON_ROOT|JOBJ_CLASSICAL_SCALE);word(d+68,JOBJ_CLASSICAL_SCALE);
    for(unsigned i=0;i<2;i++){for(unsigned c=0;c<3;c++)number(d+i*64+32+c*4,1);word(d+i*64+56,128+i*48);for(unsigned c=0;c<3;c++)number(d+128+i*48+c*20,1);}
    number(d+32,2);number(d+44,10);number(d+108,3);
    word(d+236,POBJ_ENVELOPE<<16);word(d+244,248);word(d+248,264);word(d+252,296);
    word(d+264,0);number(d+268,0.25f);word(d+272,64);number(d+276,0.75f);
    word(d+296,64);number(d+300,1);
    const uint32_t slots[]={8,56,120,244,248,252,264,272,296};for(unsigned i=0;i<9;i++)word(d+416+i*4,slots[i]);
}
int main(void)
{
    static _Alignas(32) uint8_t heap[1024*1024];assert(OSInitAlloc(heap,heap+sizeof(heap),1));HSD_SetHeap(OSCreateHeap(heap,heap+sizeof(heap)));
    HSD_AObjInitAllocData();HSD_FObjInitAllocData();HSD_VecInitAllocData();
    fixture();MeleeArchive a;assert(melee_archive_open(&a,bytes,sizeof(bytes)));
    MeleeJointGraph* graph=melee_joint_decode(&a,0);assert(graph);MeleePose* pose=melee_pose_create(graph);assert(pose);melee_joint_free(graph);
    MeleeSkin* s=melee_skin_decode(&a,224,pose,0);assert(s && melee_skin_palette_count(s)==2);
    MeleeVertexAttribute attrs[]={{.attribute=0,.mode=1},{.attribute=9,.mode=1,.components=1,.format=0},{.attribute=10,.mode=1,.format=1}};
    uint8_t dl[]={0xb8,0,2,0,2,0,0,64,0,0,3,2,0,0,64,0,0};
    MeleeVertexBatch* batch=melee_vertex_decode(dl,sizeof(dl),attrs,3,0);assert(batch);MeleeVertex out[2];
    assert(melee_skin_apply(s,batch,out,2));near(out[0].position[0],18.5f);near(out[1].position[0],20);near(out[0].normal[0][0],0.5f);
    assert(!melee_skin_apply(s,batch,out,1));melee_skin_free(s);
    s=melee_skin_decode(&a,224,pose,1);assert(s && melee_skin_apply(s,batch,out,2));near(out[0].position[0],24.5f);near(out[1].position[0],26);melee_skin_free(s);
    /* Full-weight fast path does not multiply by inverse bind at a skeleton root. */
    uint8_t* d=bytes+32;number(d+268,1);s=melee_skin_decode(&a,224,pose,0);assert(s && melee_skin_apply(s,batch,out,2));near(out[0].position[0],14);melee_skin_free(s);
    /* Shared vertices use row 0 for the owner and row 3 for the referenced joint. */
    word(d+236,0);word(d+244,64);s=melee_skin_decode(&a,224,pose,0);assert(s && melee_skin_apply(s,batch,out,2));near(out[0].position[0],14);near(out[1].position[0],20);melee_skin_free(s);
    /* Genuine null: zero word without relocation. */
    word(d+244,0);word(d+416+3*4,8);s=melee_skin_decode(&a,224,pose,0);assert(s && melee_skin_palette_count(s)==1);assert(!melee_skin_apply(s,batch,out,2));melee_skin_free(s);
    word(d+236,POBJ_SHAPEANIM<<16);assert(!melee_skin_decode(&a,224,pose,0));
    fixture();number(d+268,NAN);assert(!melee_skin_decode(&a,224,pose,0));
    fixture();word(d+272,400);assert(!melee_skin_decode(&a,224,pose,0));
    fixture();word(d+248,412);assert(!melee_skin_decode(&a,224,pose,0));
    fixture();
    uint8_t external[sizeof(bytes)+16];memcpy(external,bytes,sizeof(bytes));memset(external+sizeof(bytes),0,16);
    word(external,sizeof(external));word(external+16,1);
    word(external+32+244,UINT32_MAX);word(external+32+416+3*4,8);
    word(external+sizeof(bytes),244);memcpy(external+sizeof(bytes)+8,"ext",4);
    MeleeArchive linked;assert(melee_archive_open(&linked,external,sizeof(external)));
    uint32_t target;MeleeHostBool present;
    assert(!melee_archive_pointer(&linked,244,&target,&present));
    assert(!melee_skin_decode(&linked,224,pose,0));
    assert(!melee_archive_pointer(NULL,0,&target,&present));
    melee_vertex_free(batch);melee_pose_free(pose);
    assert(!HSD_VecGetAllocData()->used && !HSD_AObjGetAllocData()->used && !HSD_FObjGetAllocData()->used);
    puts("Skin binding: weighted, full-weight, shared and rigid matrices, node correction, normal transforms and invalid references passed");
}
