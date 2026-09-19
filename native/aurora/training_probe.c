#include <melee/gm/gm_1884.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <stdint.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>
void melee_training_probe(void)
{
    assert(sizeof(gm_80473814)>0x204);
    memset(&gm_80473814,0,sizeof(gm_80473814));
    HSD_Joint digits[3]={0};
    for(unsigned i=0;i<3;i++){
        digits[i].scale=(Vec3){1,1,1};
        if(i<2)digits[i].next=digits+i+1;
    }
    HSD_Joint desc={.child=digits,.scale={1,1,1}};
    HSD_JObj* root=HSD_JObjLoadJoint(&desc);assert(root);
    HSD_JObj* joints[3];size_t count=0;
    for(HSD_JObj* j=root->child;j;j=j->next){
        joints[count++]=j;j->aobj=HSD_AObjAlloc();assert(j->aobj);
        HSD_AObjSetEndFrame(j->aobj,10);HSD_AObjSetRate(j->aobj,0);
    }
    assert(count==3);
    for(unsigned i=0;i<39;i++){gm_80473814.jobjs[i]=joints[i%3];gm_80473814.anim_frames[i]=100+i;}
    const unsigned values[]={0,7,10,42,99,100,305,999};
    for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);i++){
        unsigned value=values[i];gm_80473814.menu_values[4]=value;
        fn_80188D3C(root);
        assert(joints[0]->aobj->curr_frame==value%10);
        assert(joints[1]->aobj->curr_frame==(value<10?10:(value/10)%10));
        assert(joints[2]->aobj->curr_frame==(value<100?10:value/100));
        for(unsigned k=0;k<39;k++){
            assert(gm_80473814.jobjs[k]==joints[k%3]);
            assert((uintptr_t)gm_80473814.jobjs[k]>UINT32_MAX);
            assert(gm_80473814.anim_frames[k]==100+(int)k);
        }
    }
    HSD_JObjRemoveAll(root);memset(&gm_80473814,0,sizeof(gm_80473814));
}
