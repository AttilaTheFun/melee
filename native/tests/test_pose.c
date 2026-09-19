#include "melee_pose.h"
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void word(uint8_t* p,uint32_t v) { p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v; }
static void number(uint8_t* p,float f) { uint32_t v;memcpy(&v,&f,4);word(p,v); }
static void near(float a,float b) { assert(fabsf(a-b)<0.0001f); }
int main(void)
{
    static _Alignas(32) uint8_t arena[1024*1024];
    assert(OSInitAlloc(arena,arena+sizeof(arena),1)); HSD_SetHeap(OSCreateHeap(arena,arena+sizeof(arena)));
    HSD_AObjInitAllocData(); HSD_FObjInitAllocData(); HSD_VecInitAllocData();
    uint8_t bytes[164]={0}; word(bytes,sizeof(bytes));word(bytes+4,128);word(bytes+8,1);
    uint8_t* d=bytes+32; word(d+8,64);word(d+128,8);
    for(unsigned j=0;j<2;j++) for(unsigned k=0;k<3;k++) number(d+j*64+32+k*4,1);
    number(d+28,1.57079632679f); number(d+44,3);number(d+48,4);number(d+64+44,1);
    MeleeArchive archive;assert(melee_archive_open(&archive,bytes,sizeof(bytes)));
    MeleeJointGraph* graph=melee_joint_decode(&archive,0);assert(graph);
    MeleePose* pose=melee_pose_create(graph);assert(pose && melee_pose_count(pose)==2);
    melee_joint_free(graph);
    const HSD_JObj* child=melee_pose_joint(pose,1);
    assert(child->parent==melee_pose_joint(pose,0) && melee_pose_source_offset(pose,1)==64);
    near(child->mtx[0][3],3);near(child->mtx[1][3],5);

    uint8_t anim[108]={0};word(anim,sizeof(anim));word(anim+4,64);word(anim+8,3);
    uint8_t* a=anim+32;word(a,1);number(a+8,10);word(a+12,20);word(a+16,24);
    a[20]=1;a[21]=0;a[22]=255;
    a[25]=5;a[28]=5;a[29]=HSD_A_FRAC_U8;word(a+32,36);
    memcpy(a+36,(uint8_t[]){0x12,0,10,10,10},5);
    word(a+64,12);word(a+68,16);word(a+72,32);
    assert(melee_archive_open(&archive,anim,sizeof(anim)));
    MeleeFighterAnimation* motion=melee_fighter_animation_decode(&archive,0);assert(motion);
    assert(melee_pose_bind(pose,motion,NULL));
    assert(melee_pose_request(pose,0));assert(melee_pose_step(pose));
    near(child->mtx[0][3],0);near(child->mtx[1][3],5);
    assert(melee_pose_set_rate(pose,0.5f));assert(melee_pose_step(pose));near(child->mtx[0][3],0.5f);
    assert(melee_pose_request(pose,7));assert(melee_pose_step(pose));near(child->mtx[0][3],7);
    size_t duplicate[]={0,0}; assert(!melee_pose_bind(pose,motion,duplicate));
    assert(melee_pose_joint(pose,0)->aobj); /* Failed binding preserves current state. */
    size_t swapped[]={1,0}; assert(melee_pose_bind(pose,motion,swapped));
    assert(melee_pose_request(pose,2) && melee_pose_step(pose));
    near(child->mtx[0][3],7);near(child->mtx[1][3],6);
    assert(!melee_pose_request(pose,NAN) && !melee_pose_set_rate(pose,-1));
    FigaTree* tree=melee_fighter_animation_tree(motion);
    tree->tracks[0].obj_type=HSD_A_J_BRANCH;
    assert(melee_pose_bind(pose,motion,NULL) && melee_pose_request(pose,0) && melee_pose_step(pose));
    assert((child->flags&JOBJ_HIDDEN) && (child->parent->flags&JOBJ_HIDDEN));
    assert(melee_pose_request(pose,2) && melee_pose_step(pose));
    assert(!(child->flags&JOBJ_HIDDEN) && !(child->parent->flags&JOBJ_HIDDEN));
    tree->tracks[0].obj_type=HSD_A_J_NODE;
    assert(melee_pose_bind(pose,motion,NULL) && melee_pose_request(pose,0) && melee_pose_step(pose));
    assert(!(child->flags&JOBJ_HIDDEN) && (child->parent->flags&JOBJ_HIDDEN));
    tree->tracks[0].obj_type=HSD_A_J_SCAX;
    assert(melee_pose_bind(pose,motion,NULL) && melee_pose_request(pose,0) && melee_pose_step(pose));
    near(child->parent->scale.x,0.001f);
    tree->tracks[0].obj_type=HSD_A_J_PATH; assert(!melee_pose_bind(pose,motion,NULL));
    melee_pose_free(pose);melee_fighter_animation_free(motion);
    assert(!HSD_AObjGetAllocData()->used && !HSD_FObjGetAllocData()->used && !HSD_VecGetAllocData()->used);
    word(d+4,JOBJ_INSTANCE); assert(melee_archive_open(&archive,bytes,sizeof(bytes)));
    graph=melee_joint_decode(&archive,0);assert(graph && !melee_pose_create(graph));melee_joint_free(graph);
    puts("Native pose binding uses original joint updates and parent matrices; stepping, mapping, rejection and cleanup passed");
}
