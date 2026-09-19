#include "melee_scene.h"
#include "melee_joint.h"
#include "melee_material.h"
#include "melee_vertex.h"
#include "melee_animation.h"
#include "melee_texture.h"
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/robj.h>
#include <sysdolphin/baselib/spline.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(HSD_Material)==20 && sizeof(HSD_PEDesc)==12, "Material scalar layout");
_Static_assert(sizeof(HSD_TexLODDesc)==16 && offsetof(HSD_TObjTevDesc,active)==28, "Texture scalar layout");
#define NONE UINT32_MAX
typedef struct SceneTracks {
    MeleeAnimationTracks* tracks;
    struct SceneTracks* next;
} SceneTracks;
typedef struct ScenePath {struct ScenePath* next;u32 offset;MeleeScene* scene;} ScenePath;
typedef struct SceneImage {struct SceneImage* next;u32 offset;HSD_ImageDesc* desc;} SceneImage;
struct MeleeScene {
    SceneImage* images;
    ScenePath* paths;
    MeleeArchive archive;
    uint8_t* bytes;
    MeleeJointGraph* graph;
    HSD_Joint* joints;
    HSD_JObj* root;
    HSD_JObj** order;
    MeleeFighterAnimation* animation;
    SceneTracks* material_tracks;
    HSD_MatAnimJoint* material_animation;
    HSD_AnimJoint* joint_animation;
    HSD_ShapeAnimJoint* shape_animation;
    void** allocations;
    size_t allocated;
};
static void* own(MeleeScene* s,size_t n,size_t size) {
    if(!n || size>SIZE_MAX/n || s->allocated>=65536)return NULL;
    void* p=calloc(n,size);if(!p)return NULL;
    void** list=realloc(s->allocations,(s->allocated+1)*sizeof(*list));
    if(!list){free(p);return NULL;}
    s->allocations=list;list[s->allocated++]=p;return p;
}
HSD_ImageDesc* melee_scene_find_image(MeleeScene* s,u32 offset){
    for(SceneImage* i=s?s->images:NULL;i;i=i->next)if(i->offset==offset)return i->desc;
    return NULL;
}
static int range(MeleeScene* s,uint32_t at,size_t n) { return at<=s->archive.data_size && n<=s->archive.data_size-at; }
static int word(MeleeScene* s,uint32_t at,uint32_t* v) { return melee_archive_u32(&s->archive,at,v); }
static int ref(MeleeScene* s,uint32_t at,uint32_t* v) {
    MeleeHostBool present;if(!melee_archive_pointer(&s->archive,at,v,&present))return 0;
    if(!present)*v=NONE;return 1;
}
static HSD_Joint* joint_at(MeleeScene* s,uint32_t offset) {
    for(size_t i=0;i<melee_joint_count(s->graph);i++)
        if(melee_joint_node(s->graph,i)->offset==offset)return &s->joints[i];
    return NULL;
}
/* Retail joint-copy expressions use PUSH_ARG (big-endian u16), RETURN.
 * Keep unsupported expression programs rejected until their control flow and
 * stack bounds can be validated. Native descriptors never point into the caller. */
static int constraints(MeleeScene* s,uint32_t at,HSD_RObjDesc** out,unsigned depth) {
    *out=NULL;if(at==NONE)return 1;
    uint32_t next,flags,expr,code,values,target,mask,end;
    if(depth>=64||!range(s,at,12)||!ref(s,at,&next)||!word(s,at+4,&flags)||
       (flags&ROBJ_TYPE_MASK)!=REFTYPE_BYTECODE||
       (flags&0x0fffffff)<1||(flags&0x0fffffff)>3||
       !ref(s,at+8,&expr)||expr==NONE||!range(s,expr,8)||
       !ref(s,expr,&code)||!ref(s,expr+4,&values)||
       code==NONE||!range(s,code,4)||values==NONE||!range(s,values,16)||
       !word(s,values,&mask)||!ref(s,values+4,&target)||target==NONE||
       !ref(s,values+12,&end)||end!=NONE)return 0;
    /* One local rotation argument, and a terminating null rvalue record. */
    if(mask!=1&&mask!=2&&mask!=4)return 0;
    const uint8_t* program=s->archive.bytes+32+code;
    if(program[0]!=2||program[1]!=0||program[2]!=0||program[3]!=1)return 0;
    HSD_Joint* joint=joint_at(s,target);if(!joint)return 0;
    HSD_RObjDesc* d=own(s,1,sizeof(*d));if(!d)return 0;
    d->flags=flags;d->u.bcexp=own(s,1,sizeof(*d->u.bcexp));if(!d->u.bcexp)return 0;
    d->u.bcexp->rvalue=own(s,2,sizeof(HSD_RvalueList));if(!d->u.bcexp->rvalue)return 0;
    d->u.bcexp->rvalue[0].flags=mask;d->u.bcexp->rvalue[0].joint=joint;
    d->u.bcexp->bytecode=(u8*)program;
    if(!constraints(s,next,&d->next,depth+1))return 0;
    *out=d;return 1;
}
static void release_paths(MeleeScene* s,ScenePath* previous){
    while(s->paths!=previous){ScenePath* p=s->paths;s->paths=p->next;melee_scene_free(p->scene);free(p);}
}
void melee_scene_free(MeleeScene* s) {
    if(!s)return;
    HSD_JObjRemoveAll(s->root);
    melee_fighter_animation_free(s->animation);
    release_paths(s,NULL);
    while(s->material_tracks){SceneTracks* t=s->material_tracks;s->material_tracks=t->next;melee_animation_tracks_free(t->tracks);free(t);}
    for(size_t i=0;i<s->allocated;i++)free(s->allocations[i]);
    free(s->allocations);free(s->joints);melee_joint_free(s->graph);melee_archive_release(&s->archive);free(s);
}
void melee_scene_release_objects(MeleeScene* s) {
    if(!s)return;
    HSD_JObjRemoveAll(s->root);s->root=NULL;
}
HSD_JObj* melee_scene_root(MeleeScene* s){return s?s->root:NULL;}
HSD_Joint* melee_scene_joint_descriptor(MeleeScene* s){return s?joint_at(s,melee_joint_root(s->graph)->offset):NULL;}
HSD_MatAnimJoint* melee_scene_material_descriptor(MeleeScene* s){return s?s->material_animation:NULL;}
HSD_AnimJoint* melee_scene_animation_descriptor(MeleeScene* s){return s?s->joint_animation:NULL;}
size_t melee_scene_joint_count(const MeleeScene* s){return s?melee_joint_count(s->graph):0;}
static void collect(HSD_JObj* j,HSD_JObj** order,size_t* n){
    for(;j;j=j->next){order[(*n)++]=j;if(!(j->flags&JOBJ_INSTANCE))collect(j->child,order,n);}
}
MeleeHostBool melee_scene_bind(MeleeScene* s,const MeleeArchive* archive,uint32_t root){
    if(!s||!archive)return false;
    MeleeFighterAnimation* animation=melee_fighter_animation_decode(archive,root);
    if(!animation)return false;
    size_t count=melee_fighter_animation_joint_count(animation);
    FigaTree* tree=melee_fighter_animation_tree(animation);FigaTrack* tracks=tree->tracks;
    if(count!=melee_scene_joint_count(s))goto fail;
    for(size_t i=0;i<count;i++)for(int k=0;k<tree->nodes[i];k++,tracks++){
        unsigned type=tracks->obj_type;if(!((type>=1&&type<=3)||(type>=5&&type<=12)))goto fail;
    }
    for(size_t i=0;i<count;i++){HSD_AObjRemove(s->order[i]->aobj);s->order[i]->aobj=NULL;}
    melee_fighter_animation_free(s->animation);s->animation=animation;tracks=tree->tracks;
    for(size_t i=0;i<count;i++){
        int n=tree->nodes[i];HSD_JObj* j=s->order[i];
        if(n){j->aobj=lbAnim_LoadAObj(tree,tracks,n);
            if(tree->type&1)HSD_JObjSetFlags(j,JOBJ_CLASSICAL_SCALE);else HSD_JObjClearFlags(j,JOBJ_CLASSICAL_SCALE);
            tracks+=n;}
    }
    return true;
fail:melee_fighter_animation_free(animation);return false;
}
MeleeHostBool melee_scene_request(MeleeScene* s,float frame){
    if(!s||!isfinite(frame)||frame<0)return false;
    HSD_JObjReqAnimAll(s->root,frame);return true;
}
MeleeHostBool melee_scene_step(MeleeScene* s){
    if(!s)return false;
    HSD_AObjInitEndCallBack();HSD_JObjAnimAll(s->root);HSD_AObjInvokeCallBacks();
    for(size_t i=0;i<melee_scene_joint_count(s);i++){
        HSD_JObjSetupMatrix(s->order[i]);
        for(int r=0;r<3;r++)for(int c=0;c<4;c++)if(!isfinite(s->order[i]->mtx[r][c]))return false;
    }
    return true;
}
static HSD_MObjDesc* material(MeleeScene* s,uint32_t at) {
    MeleeMaterial* m=melee_material_decode(&s->archive,at);if(!m)return NULL;
    HSD_MObjDesc* d=own(s,1,sizeof(*d));if(!d)goto fail;
    d->mat=own(s,1,sizeof(*d->mat));if(!d->mat)goto fail;
    d->rendermode=m->render_mode;
    memcpy(&d->mat->ambient,m->ambient,4);memcpy(&d->mat->diffuse,m->diffuse,4);memcpy(&d->mat->specular,m->specular,4);
    d->mat->alpha=m->alpha;d->mat->shininess=m->shininess;
    if(m->has_pixel_engine){d->pedesc=own(s,1,sizeof(*d->pedesc));if(!d->pedesc)goto fail;memcpy(d->pedesc,m->pixel_engine,12);}
    HSD_TObjDesc** link=&d->texdesc;
    for(size_t i=0;i<m->texture_count;i++) {
        MeleeMaterialTexture* t=&m->textures[i];
        HSD_TObjDesc* td=own(s,1,sizeof(*td));if(!td)goto fail;*link=td;link=&td->next;
        td->id=t->id;td->src=t->source;memcpy(&td->rotate,t->rotation,12);memcpy(&td->scale,t->scale,12);memcpy(&td->translate,t->translation,12);
        td->wrap_s=t->wrap_s;td->wrap_t=t->wrap_t;td->repeat_s=t->repeat_s;td->repeat_t=t->repeat_t;
        td->blend_flags=t->flags;td->blending=t->blend;td->magFilt=t->mag_filter;
        uint32_t image,pixels,palette,paldata,lod;
        if(!ref(s,t->offset+76,&image)||!ref(s,image,&pixels)||!ref(s,t->offset+80,&palette)||!ref(s,t->offset+84,&lod))goto fail;
        td->imagedesc=melee_scene_find_image(s,image);
        if(!td->imagedesc){
            SceneImage* entry=own(s,1,sizeof(*entry));if(!entry)goto fail;
            td->imagedesc=own(s,1,sizeof(*td->imagedesc));if(!td->imagedesc)goto fail;
            *td->imagedesc=(HSD_ImageDesc){.image_ptr=s->bytes+32+pixels,.width=t->mips[0].width,.height=t->mips[0].height,
                .format=t->image_format,.mipmap=t->has_mipmap,.minLOD=t->min_lod,.maxLOD=t->max_lod};
            entry->offset=image;entry->desc=td->imagedesc;entry->next=s->images;s->images=entry;
        }
        if(palette!=NONE){
            uint32_t fmt,name,count;if(!ref(s,palette,&paldata)||!word(s,palette+4,&fmt)||!word(s,palette+8,&name)||!word(s,palette+12,&count))goto fail;
            td->tlutdesc=own(s,1,sizeof(*td->tlutdesc));if(!td->tlutdesc)goto fail;
            *td->tlutdesc=(HSD_TlutDesc){.lut=s->bytes+32+paldata,.fmt=fmt,.tlut_name=name,.n_entries=count>>16};
        }
        if(lod!=NONE){td->lod=own(s,1,sizeof(*td->lod));if(!td->lod)goto fail;
            *td->lod=(HSD_TexLODDesc){t->min_filter,t->lod_bias,t->bias_clamp,t->edge_lod,t->anisotropy};}
        if(t->has_tev){td->tev=own(s,1,sizeof(*td->tev));if(!td->tev)goto fail;memcpy(td->tev,t->tev,28);td->tev->active=t->tev_active;}
    }
    melee_material_free(m);return d;
fail:melee_material_free(m);return NULL;
}
static HSD_VtxDescList* vertices(MeleeScene* s,uint32_t at,const uint8_t* display,size_t length) {
    MeleeVertexAttribute attrs[26];
    HSD_VtxDescList* v=own(s,27,sizeof(*v));if(!v)return NULL;
    for(unsigned i=0;i<27;i++,at+=24){
        uint32_t attr,mode,count,type,layout,data;
        if(!word(s,at,&attr))return NULL;
        v[i].attr=attr;if(attr==GX_VA_NULL){
            MeleeVertexBatch* batch=melee_vertex_decode(display,length,attrs,i,0);
            if(!batch)return NULL;
#if defined(MELEE_AURORA)
            for(unsigned j=0;j<i;j++)v[j].native_size=(u32)melee_vertex_array_bytes(batch,v[j].attr);
#endif
            melee_vertex_free(batch);return v;
        }
        if(i==26||!range(s,at,24)||!word(s,at+4,&mode)||!word(s,at+8,&count)||!word(s,at+12,&type)||!word(s,at+16,&layout)||!ref(s,at+20,&data))return NULL;
        if((layout&65535)>255 || mode>GX_INDEX16 || (attr>GX_VA_TEX7&&attr!=GX_VA_NBT) || (data!=NONE&&!range(s,data,1)))return NULL;
        v[i].attr_type=mode;v[i].comp_cnt=count;v[i].comp_type=type;v[i].frac=layout>>24;v[i].stride=layout&65535;
        v[i].vertex=data==NONE?NULL:s->bytes+32+data;
#if defined(MELEE_AURORA)
        v[i].native_size=data==NONE?0:s->archive.data_size-data;
        v[i].native_little_endian=GX_FALSE;
#endif
        attrs[i]=(MeleeVertexAttribute){attr,mode,count,type,layout>>24,layout&65535,
            v[i].vertex,data==NONE?0:s->archive.data_size-data};
    }
    return NULL;
}
static HSD_EnvelopeDesc** envelopes(MeleeScene* s,uint32_t at) {
    HSD_EnvelopeDesc** list=own(s,11,sizeof(*list));if(!list)return NULL;
    for(unsigned i=0;i<11;i++){
        uint32_t env;if(!ref(s,at+i*4,&env))return NULL;if(env==NONE)return list;if(i==10)return NULL;
        size_t n=0;uint32_t j;
        for(;;n++){if(n>melee_joint_count(s->graph)||!range(s,env+n*8,8)||!ref(s,env+n*8,&j))return NULL;if(j==NONE)break;}
        list[i]=own(s,n+1,sizeof(**list));if(!list[i])return NULL;
        for(size_t k=0;k<n;k++){
            float weight;if(!ref(s,env+k*8,&j)||!melee_archive_f32(&s->archive,env+k*8+4,&weight)||!isfinite(weight)||weight<0)return NULL;
            list[i][k].joint=joint_at(s,j);list[i][k].weight=weight;if(!list[i][k].joint)return NULL;
        }
    }
    return NULL;
}
/* Morph coordinates are consumed by original CPU code, so convert them to
 * owned native float triples. Display-list and shape-index bytes stay BE. */
static int shape_array(MeleeScene* s,uint32_t descriptor,uint32_t table,unsigned shapes,
                       unsigned count,unsigned attr,HSD_VtxDescList** out,u8*** indices){
    uint32_t a,mode,components,type,layout,data;
    if(!count||count>2000||!range(s,descriptor,24)||!word(s,descriptor,&a)||a!=attr||
       !word(s,descriptor+4,&mode)||(mode!=GX_INDEX8&&mode!=GX_INDEX16)||
       !word(s,descriptor+8,&components)||components!=(attr==GX_VA_POS?GX_POS_XYZ:GX_NRM_XYZ)||
       !word(s,descriptor+12,&type)||type>GX_F32||!word(s,descriptor+16,&layout)||
       !ref(s,descriptor+20,&data)||data==NONE)return 0;
    unsigned width=type==GX_F32?4:(type==GX_U16||type==GX_S16)?2:1;
    unsigned stride=layout&65535,frac=layout>>24,index_width=mode==GX_INDEX16?2:1,max=0;
    if(stride<3*width||frac>31||!range(s,table,shapes*4))return 0;
    u8** list=own(s,shapes,sizeof(*list));if(!list)return 0;
    for(unsigned i=0;i<shapes;i++){
        uint32_t at;if(!ref(s,table+4*i,&at)||at==NONE||!range(s,at,count*index_width))return 0;
        list[i]=s->bytes+32+at;
        for(unsigned k=0;k<count;k++){
            unsigned idx=index_width==2?((unsigned)list[i][2*k]<<8|list[i][2*k+1]):list[i][k];
            if(idx>max)max=idx;
        }
    }
    if(!range(s,data,(size_t)max*stride+3*width))return 0;
    float* values=own(s,(size_t)(max+1)*3,sizeof(float));if(!values)return 0;
    for(unsigned i=0;i<=max;i++)for(unsigned k=0;k<3;k++){
        uint32_t at=data+i*stride+k*width;const u8* p=s->bytes+32+at;float value;
        if(type==GX_F32){if(!melee_archive_f32(&s->archive,at,&value)||!isfinite(value))return 0;}
        else {int v=width==2?((unsigned)p[0]<<8|p[1]):p[0];
            if(type==GX_S16)v=(int16_t)v;else if(type==GX_S8)v=(int8_t)v;
            value=ldexpf((float)v,-(int)frac);}
        values[3*i+k]=value;
    }
    HSD_VtxDescList* d=own(s,1,sizeof(*d));if(!d)return 0;
    d->attr=a;d->attr_type=mode;d->comp_cnt=components;d->comp_type=GX_F32;d->stride=12;d->vertex=values;
#if defined(MELEE_AURORA)
    d->native_size=(max+1)*12;d->native_little_endian=GX_TRUE;
#endif
    *out=d;*indices=list;return 1;
}
static HSD_ShapeSetDesc* shape_set(MeleeScene* s,uint32_t at){
    uint32_t packed,nv,vd,vi,nn,nd,ni;
    if(!range(s,at,28)||!word(s,at,&packed)||!word(s,at+4,&nv)||!ref(s,at+8,&vd)||
       !ref(s,at+12,&vi)||!word(s,at+16,&nn)||!ref(s,at+20,&nd)||!ref(s,at+24,&ni))return NULL;
    unsigned flags=packed>>16,count=packed&65535;
    if(!count||count>256||(flags&~7u)||((flags&3)!=SHAPESET_AVERAGE&&(flags&3)!=SHAPESET_ADDITIVE))return NULL;
    unsigned arrays=count+((flags&SHAPESET_ADDITIVE)?1:0);
    HSD_ShapeSetDesc* d=own(s,1,sizeof(*d));if(!d)return NULL;
    d->flags=flags;d->nb_shape=count;d->nb_vertex_index=nv;d->nb_normal_index=nn;
    if(!shape_array(s,vd,vi,arrays,nv,GX_VA_POS,&d->vertex_desc,&d->vertex_idx_list))return NULL;
    if(nn){if(!shape_array(s,nd,ni,arrays,nn,GX_VA_NRM,&d->normal_desc,&d->normal_idx_list))return NULL;}
    else if(nd!=NONE||ni!=NONE)return NULL;
    return d;
}

static int polygons(MeleeScene* s,uint32_t at,HSD_PObjDesc** out,unsigned depth) {
    if(at==NONE){*out=NULL;return 1;}if(depth>=256)return 0;
    uint32_t name,next,v,display,flags,payload;
    if(!range(s,at,24)||!ref(s,at,&name)||name!=NONE||!ref(s,at+4,&next)||!ref(s,at+8,&v)||!word(s,at+12,&flags)||!ref(s,at+16,&display)||!ref(s,at+20,&payload))return 0;
    HSD_PObjDesc* p=own(s,1,sizeof(*p));if(!p)return 0;*out=p;
    p->flags=flags>>16;p->n_display=flags&65535;
    if(p->n_display && !range(s,display,p->n_display*32u))return 0;
    p->display=display==NONE?NULL:s->bytes+32+display;
    p->verts=vertices(s,v,p->display,p->n_display*32u);if(!p->verts)return 0;
    switch(p->flags&0x3000){
    case POBJ_SKIN: p->u.joint=payload==NONE?NULL:joint_at(s,payload);if(payload!=NONE&&!p->u.joint)return 0;break;
    case POBJ_SHAPEANIM:
        p->u.shape_set=shape_set(s,payload);if(!p->u.shape_set)return 0;
#if defined(MELEE_AURORA)
        for(HSD_VtxDescList* v=p->verts;v->attr!=GX_VA_NULL;v++){
            if(v->attr==GX_VA_POS&&v->native_size>(unsigned)p->u.shape_set->nb_vertex_index*v->stride)return 0;
            if(v->attr==GX_VA_NRM&&v->native_size>(unsigned)p->u.shape_set->nb_normal_index*v->stride)return 0;
        }
#endif
        break;
    case POBJ_ENVELOPE: p->u.envelope_p=envelopes(s,payload);if(!p->u.envelope_p)return 0;break;
    default:return 0;
    }
    return polygons(s,next,&p->next,depth+1);
}
static int drawables(MeleeScene* s,uint32_t at,HSD_DObjDesc** out,unsigned depth) {
    if(at==NONE){*out=NULL;return 1;}if(depth>=256)return 0;
    uint32_t name,next,mat,poly;
    if(!range(s,at,16)||!ref(s,at,&name)||name!=NONE||!ref(s,at+4,&next)||!ref(s,at+8,&mat)||!ref(s,at+12,&poly))return 0;
    HSD_DObjDesc* d=own(s,1,sizeof(*d));if(!d)return 0;*out=d;
    if(mat!=NONE){d->mobjdesc=material(s,mat);if(!d->mobjdesc)return 0;}
    return polygons(s,poly,&d->pobjdesc,0)&&drawables(s,next,&d->next,depth+1);
}
/* Instance children borrow joints loaded through the ordinary ownership tree.
 * Reject repeated ownership; all referenced nodes must have one ordinary owner. */
static int tree(MeleeScene* s,const MeleeJointNode* j,uint8_t* seen,unsigned depth) {
    if(!j)return 1;if(depth>=256)return 0;
    size_t i=(size_t)(j-melee_joint_node(s->graph,0));
    if(seen[i])return 0;seen[i]=1;
    return ((j->flags&JOBJ_INSTANCE)||tree(s,j->child,seen,depth+1))&&tree(s,j->next,seen,depth+1);
}
static float* spline_values(MeleeScene* s,u32 at,unsigned count){
    if(at==NONE||!range(s,at,(size_t)count*4))return NULL;
    float* values=own(s,count,sizeof(float));if(!values)return NULL;
    for(unsigned i=0;i<count;i++)if(!melee_archive_f32(&s->archive,at+4*i,values+i)||!isfinite(values[i]))return NULL;
    return values;
}
static HSD_Spline* spline_descriptor(MeleeScene* s,u32 at){
    u32 header,cv,lengths,poly;
    if(at==NONE||!range(s,at,24)||!word(s,at,&header)||!ref(s,at+8,&cv)||
       !ref(s,at+16,&lengths)||!ref(s,at+20,&poly))return NULL;
    unsigned type=header>>24,n=header&65535;
    if(type>3||n<2||n>4096)return NULL;
    HSD_Spline* d=own(s,1,sizeof(*d));if(!d)return NULL;d->type=type;d->numcv=n;
    if(!melee_archive_f32(&s->archive,at+4,&d->tension)||!isfinite(d->tension)||
       !melee_archive_f32(&s->archive,at+12,&d->totalLength)||!isfinite(d->totalLength)||d->totalLength<=0)return NULL;
    unsigned points=type==0?n:type==1?3*(n-1)+1:n+2;
    d->cv=(Vec3*)spline_values(s,cv,points*3);d->segLength=spline_values(s,lengths,n);
    if(!d->cv||!d->segLength||d->segLength[0]!=0||d->segLength[n-1]!=1)return NULL;
    for(unsigned i=1;i<n;i++){
        if(d->segLength[i]<d->segLength[i-1])return NULL;
        /* Mario's target course contains repeated linear control points.
         * Their zero-length segments share a cumulative distance; the
         * original spline lookup skips them for positive travel distances. */
        if(d->segLength[i]==d->segLength[i-1]&&
           (type!=0||d->cv[i].x!=d->cv[i-1].x||
            d->cv[i].y!=d->cv[i-1].y||d->cv[i].z!=d->cv[i-1].z))return NULL;
    }
    if(type||poly!=NONE){d->segPoly=(float(*)[5])spline_values(s,poly,(n-1)*5);if(!d->segPoly)return NULL;}
    return d;
}
MeleeScene* melee_scene_decode_spline(const MeleeArchive* a,uint32_t at,HSD_Spline** out){
    if(!a||!out)return NULL;MeleeScene* s=calloc(1,sizeof(*s));if(!s)return NULL;
    if(!melee_archive_acquire(&s->archive,a))goto fail;
    s->bytes=(uint8_t*)s->archive.bytes;
    HSD_Spline* spline=spline_descriptor(s,at);if(!spline)goto fail;
    *out=spline;return s;
fail:melee_scene_free(s);return NULL;
}
MeleeScene* melee_scene_decode(const MeleeArchive* a,uint32_t root) {
    if(!a)return NULL;MeleeScene* s=calloc(1,sizeof(*s));if(!s)return NULL;
    if(!melee_archive_acquire(&s->archive,a))goto fail;
    s->bytes=(uint8_t*)s->archive.bytes;
    s->graph=melee_joint_decode(&s->archive,root);if(!s->graph)goto fail;
    size_t n=melee_joint_count(s->graph);if(!n||n>4096)goto fail;
    uint8_t* seen=own(s,n,1);if(!seen||!tree(s,melee_joint_root(s->graph),seen,0))goto fail;
    for(size_t i=0;i<n;i++){
        const MeleeJointNode* node=melee_joint_node(s->graph,i);
        if(!seen[i]||((node->flags&JOBJ_INSTANCE)&&!node->child))goto fail;
    }
    s->joints=calloc(n,sizeof(*s->joints));if(!s->joints)goto fail;
    for(size_t i=0;i<n;i++){
        const MeleeJointNode* j=melee_joint_node(s->graph,i);HSD_Joint* d=&s->joints[i];
        if(j->class_name||(j->flags&(JOBJ_PTCL|JOBJ_USE_QUATERNION|JOBJ_EFFECTOR|JOBJ_USER_DEF_MTX|JOBJ_MTX_INDEP_SRT)))goto fail;
        d->flags=j->flags;d->child=j->child?joint_at(s,j->child->offset):NULL;d->next=j->next?joint_at(s,j->next->offset):NULL;
        memcpy(&d->rotation,j->rotation,12);memcpy(&d->scale,j->scale,12);memcpy(&d->position,j->position,12);
        if(j->has_inverse_bind){d->mtx=own(s,1,sizeof(Mtx));if(!d->mtx)goto fail;memcpy(d->mtx,j->inverse_bind,sizeof(Mtx));}
        if(j->flags&JOBJ_SPLINE){d->u.spline=spline_descriptor(s,j->payload_offset);if(!d->u.spline)goto fail;}
        else if(!drawables(s,j->payload_offset,&d->u.dobjdesc,0))goto fail;
        if(!constraints(s,j->constraints_offset,&d->robjdesc,0))goto fail;
    }
    s->order=own(s,n,sizeof(*s->order));if(!s->order)goto fail;
    s->root=HSD_JObjLoadJoint(joint_at(s,root));if(!s->root)goto fail;
    size_t count=0;collect(s->root,s->order,&count);if(count!=n)goto fail;
    return s;
fail:melee_scene_free(s);return NULL;
}

/* Costume expression tracks are discrete image/palette selectors. Keep every
 * descriptor and stream alive until the HSD objects that borrow them die. */
static HSD_AObjDesc* descriptor_aobj(MeleeScene* s,uint32_t at,int material) {
    uint32_t flags,tracks,id;float end;
    if(!range(s,at,16)||!word(s,at,&flags)||!ref(s,at+12,&id)||
       !ref(s,at+8,&tracks)||tracks==NONE||!melee_archive_f32(&s->archive,at+4,&end)||!isfinite(end)||end<0)return NULL;
    HSD_Joint* path=NULL;
    if(id!=NONE){
        if(material!=2)return NULL;
        path=joint_at(s,id);
        if(!path){
            for(ScenePath* p=s->paths;p;p=p->next)if(p->offset==id){path=melee_scene_joint_descriptor(p->scene);break;}
            if(!path){
                ScenePath* p=calloc(1,sizeof(*p));if(!p)return NULL;
                p->scene=melee_scene_decode(&s->archive,id);
                if(!p->scene){free(p);return NULL;}
                path=melee_scene_joint_descriptor(p->scene);
                /* AObj owns only the referenced root, not its siblings.
                 * Keep descriptors here; live HSD objects belong to the game
                 * arena and must not survive its teardown in a scene owner. */
                melee_scene_release_objects(p->scene);
                path->next=NULL;
                p->offset=id;p->next=s->paths;s->paths=p;
            }
        }
        if(!(path->flags&JOBJ_SPLINE))return NULL;
    }
    SceneTracks* owner=calloc(1,sizeof(*owner));if(!owner)return NULL;
    owner->tracks=melee_animation_tracks_decode(&s->archive,tracks);
    if(!owner->tracks){free(owner);return NULL;}
    owner->next=s->material_tracks;s->material_tracks=owner;
    HSD_FObjDesc* first=melee_animation_tracks_descriptors(owner->tracks);
    for(HSD_FObjDesc* f=first;f;f=f->next){
        if(material==3){if(f->type<1)return NULL;}
        else if(material==2){if(!((f->type>=1&&f->type<=12&&(f->type!=4||path))||(f->type>=40&&f->type<=42)))return NULL;}
        else if(material){if(f->type<HSD_A_M_AMBIENT_R||f->type>HSD_A_M_PE_DSTALPHA)return NULL;}
        else if(f->type<HSD_A_T_TIMG||f->type>HSD_A_T_TS_BLEND)return NULL;
    }
    HSD_AObjDesc* a=own(s,1,sizeof(*a));if(!a)return NULL;
    *a=(HSD_AObjDesc){.flags=flags,.end_frame=end,.fobjdesc=first,.obj_id=(HSD_IDKey)path};return a;
}
static HSD_ImageDesc* animation_image(MeleeScene* s,uint32_t at) {
    uint32_t pixels,dimensions,format,mipmap;float min,max;
    if(!range(s,at,24)||!ref(s,at,&pixels)||pixels==NONE||!word(s,at+4,&dimensions)||
       !word(s,at+8,&format)||!word(s,at+12,&mipmap)||!melee_archive_f32(&s->archive,at+16,&min)||
       !melee_archive_f32(&s->archive,at+20,&max)||!isfinite(min)||!isfinite(max)||min<0||max<min||max>16)return NULL;
    unsigned width=dimensions>>16,height=dimensions&65535,levels=mipmap?(unsigned)max+1:1;
    size_t bytes=0;
    for(unsigned i=0,w=width,h=height;i<levels;i++,w=w>1?w/2:1,h=h>1?h/2:1){
        size_t n=melee_texture_level_size(w,h,format);if(!n||n>SIZE_MAX-bytes)return NULL;bytes+=n;
    }
    if(!range(s,pixels,bytes))return NULL;
    HSD_ImageDesc* d=own(s,1,sizeof(*d));if(!d)return NULL;
    *d=(HSD_ImageDesc){.image_ptr=s->bytes+32+pixels,.width=width,.height=height,.format=format,.mipmap=mipmap,.minLOD=min,.maxLOD=max};return d;
}
static HSD_TlutDesc* animation_palette(MeleeScene* s,uint32_t at) {
    uint32_t data,format,name,count;
    if(!range(s,at,16)||!ref(s,at,&data)||data==NONE||!word(s,at+4,&format)||format>2||
       !word(s,at+8,&name)||!word(s,at+12,&count)||!(count>>16)||!range(s,data,(count>>16)*2u))return NULL;
    HSD_TlutDesc* d=own(s,1,sizeof(*d));if(!d)return NULL;
    *d=(HSD_TlutDesc){.lut=s->bytes+32+data,.fmt=format,.tlut_name=name,.n_entries=count>>16};return d;
}
static int texture_anims(MeleeScene* s,uint32_t at,HSD_TexAnim** out,unsigned depth) {
    if(at==NONE){*out=NULL;return 1;}if(depth>=8||!range(s,at,24))return 0;
    uint32_t next,id,a,images,palettes,counts;
    if(!ref(s,at,&next)||!word(s,at+4,&id)||id>7||!ref(s,at+8,&a)||a==NONE||
       !ref(s,at+12,&images)||!ref(s,at+16,&palettes)||!word(s,at+20,&counts))return 0;
    unsigned ni=counts>>16,np=counts&65535;
    if(ni>4096||np>256||(ni&&!range(s,images,ni*4u))||(np&&!range(s,palettes,np*4u)))return 0;
    HSD_TexAnim* t=own(s,1,sizeof(*t));if(!t)return 0;*out=t;t->id=id;t->n_imagetbl=ni;t->n_tluttbl=np;
    t->aobjdesc=descriptor_aobj(s,a,0);if(!t->aobjdesc)return 0;
    t->imagetbl=ni?own(s,ni,sizeof(*t->imagetbl)):NULL;t->tluttbl=np?own(s,np,sizeof(*t->tluttbl)):NULL;
    if((ni&&!t->imagetbl)||(np&&!t->tluttbl))return 0;
    for(unsigned i=0;i<ni;i++){uint32_t entry;if(!ref(s,images+i*4,&entry)||entry==NONE||!(t->imagetbl[i]=animation_image(s,entry)))return 0;}
    for(unsigned i=0;i<np;i++){uint32_t entry;if(!ref(s,palettes+i*4,&entry)||entry==NONE||!(t->tluttbl[i]=animation_palette(s,entry)))return 0;}
    for(HSD_FObjDesc* f=t->aobjdesc->fobjdesc;f;f=f->next)if((f->type==HSD_A_T_TIMG&&!ni)||(f->type==HSD_A_T_TCLT&&!np))return 0;
    if(!texture_anims(s,next,&t->next,depth+1))return 0;
    for(HSD_TexAnim* n=t->next;n;n=n->next)if(n->id==t->id)return 0;
    return 1;
}
static int material_anims(MeleeScene* s,uint32_t at,HSD_MatAnim** out,HSD_DObj* d,unsigned depth) {
    if(at==NONE){*out=NULL;return 1;}if(depth>=256||!range(s,at,16))return 0;
    uint32_t next,a,t,r;
    if(!ref(s,at,&next)||!ref(s,at+4,&a)||!ref(s,at+8,&t)||!ref(s,at+12,&r)||r!=NONE)return 0;
    if(!d&&(a!=NONE||t!=NONE))return 0;
    HSD_MatAnim* m=own(s,1,sizeof(*m));if(!m)return 0;*out=m;
    if(a!=NONE){m->aobjdesc=descriptor_aobj(s,a,1);if(!m->aobjdesc)return 0;}
    if(!texture_anims(s,t,&m->texanim,0))return 0;
    for(HSD_TexAnim* ta=m->texanim;ta;ta=ta->next){
        HSD_TObj* to=d->mobj?d->mobj->tobj:NULL;while(to&&to->id!=ta->id)to=to->next;if(!to)return 0;
        for(HSD_FObjDesc* f=ta->aobjdesc->fobjdesc;f;f=f->next){
            if(f->type==HSD_A_T_LOD_BIAS&&!to->lod)return 0;
            if(f->type>=HSD_A_T_KONST_R&&f->type<=HSD_A_T_TEV1_A&&!to->tev)return 0;
        }
    }
    return material_anims(s,next,&m->next,d?d->next:NULL,depth+1);
}
static int material_joints(MeleeScene* s,uint32_t at,HSD_MatAnimJoint** out,HSD_JObj* joint,unsigned depth) {
    if(at==NONE){*out=NULL;return 1;}if(!joint||depth>=256||!range(s,at,12))return 0;
    uint32_t child,next,m;if(!ref(s,at,&child)||!ref(s,at+4,&next)||!ref(s,at+8,&m))return 0;
    HSD_MatAnimJoint* j=own(s,1,sizeof(*j));if(!j)return 0;*out=j;
    return material_anims(s,m,&j->matanim,(joint->flags&JOBJ_SPLINE)?NULL:joint->u.dobj,0)&&
        material_joints(s,child,&j->child,joint->child,depth+1)&&material_joints(s,next,&j->next,joint->next,depth+1);
}
MeleeHostBool melee_scene_bind_materials(MeleeScene* s,uint32_t root) {
    if(!s||s->material_animation)return false;
    size_t checkpoint=s->allocated;SceneTracks* previous=s->material_tracks;HSD_MatAnimJoint* graph=NULL;
    if(material_joints(s,root,&graph,s->root,0)&&graph){
        HSD_JObjAddAnimAll(s->root,NULL,graph,NULL);s->material_animation=graph;return true;
    }
    while(s->material_tracks!=previous){SceneTracks* t=s->material_tracks;s->material_tracks=t->next;melee_animation_tracks_free(t->tracks);free(t);}
    while(s->allocated>checkpoint)free(s->allocations[--s->allocated]);return false;
}

/* Empty constraint animation records still clear any previous RObj AObj. */
static int empty_constraint_anims(MeleeScene* s,uint32_t at,HSD_RObjAnimJoint** out,HSD_RObj* robj,unsigned depth){
    *out=NULL;if(at==NONE)return 1;
    uint32_t next,a;if(!robj||depth>=64||!range(s,at,8)||!ref(s,at,&next)||!ref(s,at+4,&a)||a!=NONE)return 0;
    HSD_RObjAnimJoint* d=own(s,1,sizeof(*d));if(!d)return 0;
    if(!empty_constraint_anims(s,next,&d->next,robj->next,depth+1))return 0;
    *out=d;return 1;
}
static int animation_joints(MeleeScene* s,uint32_t at,HSD_AnimJoint** out,
                            HSD_JObj* joint,unsigned depth) {
    if(at==NONE){*out=NULL;return 1;}
    if(!joint||depth>=256||!range(s,at,20))return 0;
    uint32_t child,next,a,r,flags;
    if(!ref(s,at,&child)||!ref(s,at+4,&next)||!ref(s,at+8,&a)||
       !ref(s,at+12,&r)||!word(s,at+16,&flags)||(flags&~1u))return 0;
    HSD_AnimJoint* j=own(s,1,sizeof(*j));if(!j)return 0;*out=j;j->flags=flags;
    if(!empty_constraint_anims(s,r,&j->robj_anim,joint->robj,0))return 0;
    if(a!=NONE){j->aobjdesc=descriptor_aobj(s,a,2);if(!j->aobjdesc||((joint->flags&JOBJ_SPLINE)&&j->aobjdesc->obj_id))return 0;}
    return animation_joints(s,child,&j->child,joint->child,depth+1)&&
        animation_joints(s,next,&j->next,joint->next,depth+1);
}
MeleeHostBool melee_scene_bind_joints(MeleeScene* s,uint32_t root) {
    if(!s||!s->root||s->animation||s->joint_animation)return false;
    ScenePath* previous_paths=s->paths;
    size_t checkpoint=s->allocated;SceneTracks* previous=s->material_tracks;
    HSD_AnimJoint* graph=NULL;
    if(animation_joints(s,root,&graph,s->root,0)&&graph){
        HSD_JObjAddAnimAll(s->root,graph,NULL,NULL);s->joint_animation=graph;return true;
    }
    release_paths(s,previous_paths);
    while(s->material_tracks!=previous){SceneTracks* t=s->material_tracks;s->material_tracks=t->next;melee_animation_tracks_free(t->tracks);free(t);}
    while(s->allocated>checkpoint)free(s->allocations[--s->allocated]);return false;
}

/* HSD_JObjAddAnim consumers read only this root; child/next trees describe
 * another skeleton and must not be bound to the item's hierarchy. */
MeleeHostBool melee_scene_bind_joint_root(MeleeScene* s,uint32_t at){
    if(!s||!s->root||s->animation||s->joint_animation||!range(s,at,20))return false;
    uint32_t a,r,flags;if(!ref(s,at+8,&a)||!ref(s,at+12,&r)||r!=NONE||!word(s,at+16,&flags)||(flags&~1u))return false;
    ScenePath* previous_paths=s->paths;
    size_t checkpoint=s->allocated;SceneTracks* previous=s->material_tracks;
    HSD_AnimJoint* j=own(s,1,sizeof(*j));if(!j)return false;j->flags=flags;
    if(a!=NONE&&(!(j->aobjdesc=descriptor_aobj(s,a,2))||((s->root->flags&JOBJ_SPLINE)&&j->aobjdesc->obj_id)))goto fail;
    HSD_JObjAddAnim(s->root,j,NULL,NULL);s->joint_animation=j;return true;
fail:
    release_paths(s,previous_paths);
    while(s->material_tracks!=previous){SceneTracks* t=s->material_tracks;s->material_tracks=t->next;melee_animation_tracks_free(t->tracks);free(t);}
    while(s->allocated>checkpoint)free(s->allocations[--s->allocated]);return false;
}

static int shape_tracks(MeleeScene* s,uint32_t at,HSD_ShapeAnim** out,HSD_PObj* p,unsigned depth){
    if(at==NONE){*out=NULL;return 1;}
    uint32_t next,animation;
    if(!p||depth>=256||(p->flags&0x3000)!=POBJ_SHAPEANIM||!range(s,at,8)||
       !ref(s,at,&next)||!ref(s,at+4,&animation))return 0;
    HSD_ShapeAnim* d=own(s,1,sizeof(*d));if(!d)return 0;*out=d;
    if(animation!=NONE){
        d->aobjdesc=descriptor_aobj(s,animation,3);if(!d->aobjdesc)return 0;
        for(HSD_FObjDesc* f=d->aobjdesc->fobjdesc;f;f=f->next){
            if(p->u.shape_set->flags&SHAPESET_ADDITIVE){
                if(f->type<HSD_A_S_W0||f->type-HSD_A_S_W0>=p->u.shape_set->nb_shape)return 0;
            }else if(f->type!=1)return 0;
        }
    }
    return shape_tracks(s,next,&d->next,p->next,depth+1);
}
static int shape_drawables(MeleeScene* s,uint32_t at,
                          HSD_ShapeAnimDObj** out,HSD_DObj* d,unsigned depth,int allow_tracks){
    if(at==NONE){*out=NULL;return 1;}
    uint32_t next,anim;
    if(depth>=256||!range(s,at,8)||!ref(s,at,&next)||!ref(s,at+4,&anim))return 0;
    HSD_ShapeAnimDObj* shape=own(s,1,sizeof(*shape));if(!shape)return 0;*out=shape;
    if(anim!=NONE&&(!allow_tracks||!d||!shape_tracks(s,anim,&shape->shapeanim,d->pobj,0)))return 0;
    return shape_drawables(s,next,&shape->next,d?d->next:NULL,depth+1,allow_tracks);
}
static int shape_joints(MeleeScene* s,uint32_t at,
                        HSD_ShapeAnimJoint** out,HSD_JObj* joint,unsigned depth,int allow_tracks){
    if(at==NONE){*out=NULL;return 1;}
    uint32_t child,next,drawables;
    if(!joint||depth>=256||!range(s,at,12)||!ref(s,at,&child)||
       !ref(s,at+4,&next)||!ref(s,at+8,&drawables))return 0;
    HSD_ShapeAnimJoint* shape=own(s,1,sizeof(*shape));if(!shape)return 0;*out=shape;
    return shape_drawables(s,drawables,&shape->shapeanimdobj,(joint->flags&JOBJ_SPLINE)?NULL:joint->u.dobj,0,allow_tracks)&&
        shape_joints(s,child,&shape->child,joint->child,depth+1,allow_tracks)&&
        shape_joints(s,next,&shape->next,joint->next,depth+1,allow_tracks);
}
static MeleeHostBool bind_shapes(MeleeScene* s,uint32_t root,int allow_tracks){
    if(!s||!s->root||s->shape_animation)return false;
    size_t checkpoint=s->allocated;SceneTracks* previous=s->material_tracks;HSD_ShapeAnimJoint* graph=NULL;
    if(shape_joints(s,root,&graph,s->root,0,allow_tracks)&&graph){
        HSD_JObjAddAnimAll(s->root,NULL,NULL,graph);s->shape_animation=graph;return true;
    }
    while(s->material_tracks!=previous){SceneTracks* t=s->material_tracks;s->material_tracks=t->next;melee_animation_tracks_free(t->tracks);free(t);}
    while(s->allocated>checkpoint)free(s->allocations[--s->allocated]);return false;
}
MeleeHostBool melee_scene_bind_empty_shapes(MeleeScene* s,uint32_t root){return bind_shapes(s,root,0);}
MeleeHostBool melee_scene_bind_shapes(MeleeScene* s,uint32_t root){return bind_shapes(s,root,1);}
HSD_ShapeAnimJoint* melee_scene_shape_descriptor(MeleeScene* s){return s?s->shape_animation:NULL;}
