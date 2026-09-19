#include "melee_skin.h"
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/mtx.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
typedef struct { size_t joint; float weight; } Weight;
typedef struct { Weight* weights; size_t count; } Envelope;
struct MeleeSkin { const MeleePose* pose; size_t owner,shared,count; unsigned type; Envelope envelopes[10]; };
void melee_skin_free(MeleeSkin* s){if(s){for(unsigned i=0;i<10;i++)free(s->envelopes[i].weights);free(s);}}
size_t melee_skin_palette_count(const MeleeSkin* s){return s?s->count:0;}
static MeleeHostBool reference(const MeleeArchive* a,uint32_t slot,uint32_t* offset)
{ MeleeHostBool present;if(!melee_archive_pointer(a,slot,offset,&present))return false;if(!present)*offset=UINT32_MAX;return true; }
static size_t joint_index(const MeleePose* pose,uint32_t offset)
{ for(size_t i=0;i<melee_pose_count(pose);i++)if(melee_pose_source_offset(pose,i)==offset)return i;return SIZE_MAX; }
MeleeSkin* melee_skin_decode(const MeleeArchive* a,uint32_t polygon,const MeleePose* pose,size_t owner)
{
    if(!a || !a->bytes || polygon>a->data_size || a->data_size-polygon<24 || !melee_pose_joint(pose,owner))return NULL;
    uint32_t flags,reference_offset;
    if(!melee_archive_u32(a,polygon+12,&flags) || !reference(a,polygon+20,&reference_offset))return NULL;
    unsigned type=(flags>>16)&0x3000;
    if(type!=POBJ_SKIN && type!=POBJ_ENVELOPE)return NULL;
    if(melee_pose_joint(pose,owner)->flags & (JOBJ_BILLBOARD_FIELD|JOBJ_PBILLBOARD))return NULL;
    MeleeSkin* s=calloc(1,sizeof(*s));if(!s)return NULL;
    s->pose=pose;s->owner=owner;s->type=type;s->shared=SIZE_MAX;
    if(type==POBJ_SKIN){
        s->count=1;
        if(reference_offset!=UINT32_MAX){s->shared=joint_index(pose,reference_offset);if(s->shared==SIZE_MAX)goto fail;s->count=2;}
        return s;
    }
    if(reference_offset==UINT32_MAX)goto fail;
    for(unsigned group=0;group<=10;group++){
        uint32_t env;
        if(reference_offset>a->data_size || group>(a->data_size-reference_offset)/4 || !reference(a,reference_offset+group*4,&env))goto fail;
        if(env==UINT32_MAX){if(!s->count)goto fail;return s;}
        if(group==10)goto fail;
        Envelope* e=s->envelopes+group;
        for(;;){
            uint32_t offset;float weight;
            if(env>a->data_size || a->data_size-env<8 || !reference(a,env,&offset))goto fail;
            if(offset==UINT32_MAX)break;
            size_t index=joint_index(pose,offset);
            if(index==SIZE_MAX || !melee_archive_f32(a,env+4,&weight) || !isfinite(weight) || e->count>=SIZE_MAX/sizeof(Weight))goto fail;
            Weight* weights=realloc(e->weights,(e->count+1)*sizeof(*weights));if(!weights)goto fail;
            e->weights=weights;e->weights[e->count++]=(Weight){index,weight};env+=8;
        }
        if(!e->count)goto fail;s->count++;
    }
fail:melee_skin_free(s);return NULL;
}
static MeleeHostBool finite_matrix(Mtx m)
{ for(unsigned i=0;i<3;i++)for(unsigned j=0;j<4;j++)if(!isfinite(m[i][j]))return false;return true; }
MeleeHostBool melee_skin_matrices(const MeleeSkin* s,Mtx positions[10],Mtx normals[10])
{
    if(!s || !positions || !normals)return false;
    const HSD_JObj* owner=melee_pose_joint(s->pose,s->owner);
    if(s->type==POBJ_SKIN){
        PSMTXCopy((MtxPtr)owner->mtx,positions[0]);
        if(s->count==2)PSMTXCopy((MtxPtr)melee_pose_joint(s->pose,s->shared)->mtx,positions[1]);
    }else{
        Mtx node;MtxPtr right=NULL;
        if(!(owner->flags&JOBJ_SKELETON_ROOT)){
            const HSD_JObj* skeleton=owner;
            while(skeleton && !(skeleton->flags&(JOBJ_SKELETON|JOBJ_SKELETON_ROOT)))skeleton=skeleton->parent;
            if(!skeleton || (!(skeleton->flags&JOBJ_SKELETON_ROOT) && !skeleton->envelopemtx))return false;
            right=_HSD_mkEnvelopeModelNodeMtx((HSD_JObj*)owner,node);
        }
        for(size_t i=0;i<s->count;i++){
            const Envelope* e=s->envelopes+i;
            const HSD_JObj* first=melee_pose_joint(s->pose,e->weights[0].joint);
            /* Original PObj's 1 - 1e-10f threshold rounds to 1 in binary32. */
            if(e->weights[0].weight>=1.0f){
                if(right){if(!first->envelopemtx)return false;PSMTXConcat((MtxPtr)first->mtx,first->envelopemtx,positions[i]);}
                else PSMTXCopy((MtxPtr)first->mtx,positions[i]);
            }else{
                memset(positions[i],0,sizeof(Mtx));
                for(size_t k=0;k<e->count;k++){
                    const HSD_JObj* joint=melee_pose_joint(s->pose,e->weights[k].joint);Mtx temp;
                    if(!joint->envelopemtx)return false;
                    PSMTXConcat((MtxPtr)joint->mtx,joint->envelopemtx,temp);
                    HSD_MtxScaledAdd(temp,positions[i],positions[i],e->weights[k].weight);
                }
            }
            if(right)PSMTXConcat(positions[i],right,positions[i]);
        }
    }
    for(size_t i=0;i<s->count;i++){
        HSD_MtxInverseTranspose(positions[i],normals[i]);
        if(!finite_matrix(positions[i]))return false;
        for(unsigned row=0;row<3;row++)for(unsigned col=0;col<3;col++)if(!isfinite(normals[i][row][col]))return false;
    }
    return true;
}
MeleeHostBool melee_skin_apply(const MeleeSkin* s,const MeleeVertexBatch* b,MeleeVertex* output,size_t capacity)
{
    if(!s || !b || (!output && melee_vertex_count(b)) || capacity<melee_vertex_count(b))return false;
    Mtx positions[10],normals[10];if(!melee_skin_matrices(s,positions,normals))return false;
    const MeleeVertex* source=melee_vertex_data(b);uint32_t mask=melee_vertex_attribute_mask(b);
    for(size_t i=0;i<melee_vertex_count(b);i++){
        MeleeVertex v=source[i];unsigned row=mask&1 ? v.matrix_index[0] : 0;
        if(row%3 || row/3>=s->count)return false;unsigned index=row/3;
        Vec3 in={v.position[0],v.position[1],v.position[2]},out;
        PSMTXMultVec(positions[index],&in,&out);v.position[0]=out.x;v.position[1]=out.y;v.position[2]=out.z;
        for(unsigned n=0;n<3;n++){
            in=(Vec3){v.normal[n][0],v.normal[n][1],v.normal[n][2]};
            PSMTXMultVecSR(normals[index],&in,&out);v.normal[n][0]=out.x;v.normal[n][1]=out.y;v.normal[n][2]=out.z;
        }
        for(unsigned c=0;c<3;c++){
            if(!isfinite(v.position[c]))return false;
            for(unsigned n=0;n<3;n++)if(!isfinite(v.normal[n][c]))return false;
        }
        output[i]=v;
    }
    return true;
}
