#include "melee_pose.h"
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct MeleePose {
    HSD_JObj* joints;
    uint32_t* offsets;
    Mtx* inverse_bind;
    size_t count;
};
size_t melee_pose_count(const MeleePose* p) { return p ? p->count : 0; }
const HSD_JObj* melee_pose_joint(const MeleePose* p, size_t i)
{ return p && i<p->count ? p->joints+i : NULL; }
uint32_t melee_pose_source_offset(const MeleePose* p, size_t i)
{ return p && i<p->count ? p->offsets[i] : UINT32_MAX; }
void melee_pose_unbind(MeleePose* p)
{
    if (!p) return;
    for (size_t i=0;i<p->count;i++) {
        HSD_AObj* a=p->joints[i].aobj;
        if (a) { HSD_AObjSetFObj(a,NULL); HSD_AObjFree(a); p->joints[i].aobj=NULL; }
    }
}
void melee_pose_free(MeleePose* p)
{
    if (!p) return;
    melee_pose_unbind(p);
    for (size_t i=0;i<p->count;i++) if(p->joints[i].scl) HSD_VecFree(p->joints[i].scl);
    free(p->inverse_bind); free(p->offsets); free(p->joints); free(p);
}
static MeleeHostBool matrices(MeleePose* p)
{
    for (size_t i=0;i<p->count;i++) {
        /* Parent matrices are already clean in preorder. No class dispatch or
         * constraint evaluator is needed for the supported transform records. */
        HSD_JObjMakeMatrix(p->joints+i);
        p->joints[i].flags &= ~JOBJ_MTX_DIRTY;
        for (unsigned row=0;row<3;row++) for(unsigned col=0;col<4;col++)
            if(!isfinite(p->joints[i].mtx[row][col])) return false;
    }
    return true;
}
MeleePose* melee_pose_create(const MeleeJointGraph* graph)
{
    typedef struct { const MeleeJointNode* node; size_t parent; } Pending;
    size_t count=melee_joint_count(graph);
    if (!count || count>SIZE_MAX/sizeof(HSD_JObj) || count>SIZE_MAX/sizeof(Pending)) return NULL;
    MeleePose* p=calloc(1,sizeof(*p));
    Pending* stack=malloc(count*sizeof(*stack));
    const MeleeJointNode** order=calloc(count,sizeof(*order));
    if (!p || !stack || !order) { free(p); free(stack); free(order); return NULL; }
    p->joints=calloc(count,sizeof(*p->joints)); p->offsets=calloc(count,sizeof(*p->offsets));
    p->inverse_bind=calloc(count,sizeof(*p->inverse_bind));
    if (!p->joints || !p->offsets || !p->inverse_bind) goto fail;
    size_t depth=1; stack[0]=(Pending){melee_joint_root(graph),SIZE_MAX};
    while(depth) {
        Pending item=stack[--depth]; const MeleeJointNode* n=item.node;
        if(p->count==count) goto fail;
        for(size_t i=0;i<p->count;i++) if(order[i]==n) goto fail;
        if (n->class_name || n->constraints_offset!=UINT32_MAX || n->flags &
            (JOBJ_INSTANCE|JOBJ_JOINT|JOBJ_USE_QUATERNION|JOBJ_USER_DEF_MTX|
             JOBJ_MTX_INDEP_PARENT|JOBJ_MTX_INDEP_SRT)) goto fail;
        size_t index=p->count++; order[index]=n; p->offsets[index]=n->offset;
        HSD_JObj* j=p->joints+index;
        j->flags=n->flags|JOBJ_MTX_DIRTY;
        j->rotate=(Quaternion){n->rotation[0],n->rotation[1],n->rotation[2],0};
        j->scale=(Vec3){n->scale[0],n->scale[1],n->scale[2]};
        j->translate=(Vec3){n->position[0],n->position[1],n->position[2]};
        if(item.parent!=SIZE_MAX) j->parent=p->joints+item.parent;
        if(n->has_inverse_bind) { memcpy(p->inverse_bind[index],n->inverse_bind,sizeof(Mtx)); j->envelopemtx=p->inverse_bind[index]; }
        if(n->next) { if(depth==count) goto fail; stack[depth++]=(Pending){n->next,item.parent}; }
        if(n->child) { if(depth==count) goto fail; stack[depth++]=(Pending){n->child,index}; }
    }
    if(p->count!=count) goto fail;
    for(size_t i=0;i<count;i++) for(size_t k=0;k<count;k++) {
        if(order[i]->child==order[k]) p->joints[i].child=p->joints+k;
        if(order[i]->next==order[k]) p->joints[i].next=p->joints+k;
    }
    if(!matrices(p)) goto fail;
    free(order); free(stack); return p;
fail:
    free(order); free(stack); melee_pose_free(p); return NULL;
}
static MeleeHostBool supported_track(unsigned type)
{ return (type>=1 && type<=3) || (type>=5 && type<=12); }
MeleeHostBool melee_pose_bind(MeleePose* p, MeleeFighterAnimation* animation, const size_t* mapping)
{
    size_t count=melee_fighter_animation_joint_count(animation);
    FigaTree* tree=melee_fighter_animation_tree(animation);
    if(!p || !tree || !count || (!mapping && count!=p->count)) return false;
    uint8_t* used=calloc(p->count,1); if(!used) return false;
    FigaTrack* tracks=tree->tracks;
    for(size_t i=0;i<count;i++) {
        size_t index=mapping ? mapping[i] : i;
        if(index>=p->count || used[index]) { free(used); return false; }
        used[index]=1;
        for(int k=0;k<tree->nodes[i];k++,tracks++) if(!supported_track(tracks->obj_type)) { free(used); return false; }
    }
    free(used);
    melee_pose_unbind(p);
    tracks=tree->tracks;
    for(size_t i=0;i<count;i++) {
        HSD_JObj* j=p->joints+(mapping ? mapping[i] : i);
        int n=tree->nodes[i];
        if(n) {
            j->aobj=lbAnim_LoadAObj(tree,tracks,(s8)n);
            if(tree->type&1) HSD_JObjSetFlags(j,JOBJ_CLASSICAL_SCALE);
            else HSD_JObjClearFlags(j,JOBJ_CLASSICAL_SCALE);
        }
        if(n) tracks+=n;
    }
    return true;
}
MeleeHostBool melee_pose_request(MeleePose* p, float frame)
{
    if(!p || !isfinite(frame) || frame<0) return false;
    for(size_t i=0;i<p->count;i++) HSD_AObjReqAnim(p->joints[i].aobj,frame);
    return true;
}
MeleeHostBool melee_pose_set_rate(MeleePose* p, float rate)
{
    if(!p || !isfinite(rate) || rate<0) return false;
    for(size_t i=0;i<p->count;i++) HSD_AObjSetRate(p->joints[i].aobj,rate);
    return true;
}
MeleeHostBool melee_pose_step(MeleePose* p)
{
    if(!p) return false;
    HSD_AObjInitEndCallBack();
    for(size_t i=0;i<p->count;i++) HSD_AObjInterpretAnim(p->joints[i].aobj,p->joints+i,JObjUpdateFunc);
    HSD_AObjInvokeCallBacks();
    return matrices(p);
}
