#include <melee/lb/lbanim.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int calls;
static float value;
static void update(void* object, int type, HSD_ObjData* data)
{ assert(object==&value && type==5); value=data->fv; ++calls; }
static void lefloat(unsigned char* p, float f)
{ uint32_t u; memcpy(&u,&f,4); p[0]=u; p[1]=u>>8; p[2]=u>>16; p[3]=u>>24; }
static void tick(HSD_AObj* a, float frame, float expected)
{
    HSD_AObjInitEndCallBack(); HSD_AObjInterpretAnim(a,&value,update);
    assert(fabsf(a->curr_frame-frame)<0.0001f && fabsf(value-expected)<0.0001f);
    HSD_AObjInvokeCallBacks();
}
static void release(HSD_AObj* a)
{
    /* These track-only controllers never acquire joint references. */
    assert(!a->hsd_obj); HSD_AObjSetFObj(a,NULL); HSD_AObjFree(a);
}
int main(void)
{
    static _Alignas(32) unsigned char arena[65536];
    assert(OSInitAlloc(arena,arena+sizeof(arena),1));
    HSD_SetHeap(OSCreateHeap(arena,arena+sizeof(arena)));
    HSD_FObjInitAllocData(); HSD_AObjInitAllocData();
    unsigned char stream[11]={0x12}; lefloat(stream+1,0); stream[5]=10;
    lefloat(stream+6,10); stream[10]=10;
    FigaTrack track={.length=sizeof(stream),.obj_type=5,.ad_head=stream};
    FigaTree tree={.type=1,.frames=10};
    HSD_AObj* a=lbAnim_LoadAObj(&tree,&track,1); assert(a);
    assert(a->flags==AOBJ_NO_ANIM && a->framerate==1 && a->fobj->ad_head==stream);
    HSD_AObjInterpretAnim(a,&value,update); assert(!calls);
    HSD_AObjReqAnim(a,0); tick(a,0,0);
    for(int i=1;i<=10;i++) tick(a,(float)i,(float)i);
    assert(a->flags&AOBJ_NO_ANIM);
    int stopped_calls=calls; HSD_AObjInterpretAnim(a,&value,update); assert(calls==stopped_calls);
    HSD_AObjSetCurrentFrame(a,3); assert(a->curr_frame==10);
    HSD_AObjReqAnim(a,2); tick(a,2,2);
    HSD_AObjSetRate(a,0.5f); tick(a,2.5f,2.5f);
    HSD_AObjSetCurrentFrame(a,6); tick(a,6,6);
    HSD_AObjStopAnim(a,&value,update); assert(a->flags&AOBJ_NO_ANIM);
    release(a);

    tree.flags=AOBJ_LOOP;
    a=lbAnim_LoadAObj(&tree,&track,1); assert(a);
    HSD_AObjSetRewindFrame(a,2); HSD_AObjSetRate(a,3); HSD_AObjReqAnim(a,0);
    tick(a,0,0); tick(a,3,3); tick(a,6,6); tick(a,9,9); tick(a,4,4);
    assert((a->flags&AOBJ_REWINDED) && !(a->flags&AOBJ_NO_ANIM));
    tick(a,7,7); assert(!(a->flags&AOBJ_REWINDED));
    HSD_AObjSetRate(a,25); tick(a,8,8); assert(a->flags&AOBJ_REWINDED);
    HSD_AObjSetRate(a,0); tick(a,8,8);
    HSD_AObjSetCurrentFrame(a,2); tick(a,2,2);
    HSD_AObjSetRate(a,1); HSD_AObjSetFlags(a,AOBJ_NO_UPDATE);
    int before=calls; tick(a,3,2); assert(calls==before);
    HSD_AObjClearFlags(a,AOBJ_NO_UPDATE); tick(a,4,4);
    HSD_AObjClearFlags(a,AOBJ_LOOP); HSD_AObjSetCurrentFrame(a,10); tick(a,10,10);
    assert(a->flags&AOBJ_NO_ANIM); release(a);
    FigaTrack ordered[3]={track,track,track};
    ordered[0].obj_type=5; ordered[0].startframe=UINT16_MAX;
    ordered[1].obj_type=TYPE_JOBJ; ordered[2].obj_type=1;
    a=lbAnim_LoadAObj(&tree,ordered,3); assert(a);
    assert(a->fobj->obj_type==TYPE_JOBJ && a->fobj->next->obj_type==5);
    assert(a->fobj->next->startframe==-1 && a->fobj->next->next->obj_type==1);
    assert(!a->fobj->next->next->next); release(a);
    assert(!lbAnim_LoadAObj(&tree,&track,0));
    assert(!lbAnim_LoadAObj(NULL,&track,1));
    assert(!lbAnim_LoadAObj(&tree,NULL,1));
    assert(!HSD_AObjGetAllocData()->used && !HSD_FObjGetAllocData()->used);
    puts("Original fighter track loader and AObj timeline: stepping, seek, rates, loops, suppression, stop and cleanup passed");
}
