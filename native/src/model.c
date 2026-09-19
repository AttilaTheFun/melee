#include "melee_model.h"
#include "melee_skin.h"
#include <stdlib.h>
#include <string.h>
typedef struct { MeleeModelPart view; MeleeVertexBatch* batch; MeleeSkin* skin; MeleeVertex* vertices; size_t owner; } Part;
struct MeleeModel { MeleeJointGraph* graph; MeleePose* pose; Part* parts; size_t count; MeleeMaterial** materials; size_t material_count; MeleeHostBool* hidden;size_t drawable_count; };
typedef struct { uint32_t* offsets; size_t count; } Seen;
static MeleeHostBool visit(Seen* s,uint32_t offset)
{
    for(size_t i=0;i<s->count;i++)if(s->offsets[i]==offset)return false;
    if(s->count>=SIZE_MAX/sizeof(uint32_t))return false;
    uint32_t* p=realloc(s->offsets,(s->count+1)*sizeof(*p));if(!p)return false;
    s->offsets=p;p[s->count++]=offset;return true;
}
static MeleeHostBool range(const MeleeArchive* a,uint32_t at,size_t size)
{ return at<=a->data_size && size<=a->data_size-at; }
static MeleeHostBool ref(const MeleeArchive* a,uint32_t at,uint32_t* value)
{ MeleeHostBool present;if(!melee_archive_pointer(a,at,value,&present))return false;if(!present)*value=UINT32_MAX;return true; }
void melee_model_free(MeleeModel* m)
{
    if(!m)return;
    for(size_t i=0;i<m->count;i++){free(m->parts[i].vertices);melee_skin_free(m->parts[i].skin);melee_vertex_free(m->parts[i].batch);}
    for(size_t i=0;i<m->material_count;i++)melee_material_free(m->materials[i]);
    free(m->hidden);free(m->materials);free(m->parts);melee_pose_free(m->pose);melee_joint_free(m->graph);free(m);
}
size_t melee_model_part_count(const MeleeModel* m){return m?m->count:0;}
const MeleeModelPart* melee_model_part(const MeleeModel* m,size_t i){return m && i<m->count?&m->parts[i].view:NULL;}
size_t melee_model_drawable_count(const MeleeModel* m){return m?m->drawable_count:0;}
MeleeHostBool melee_model_set_drawable_hidden(MeleeModel* m,size_t index,MeleeHostBool hidden)
{
    if(!m || index>=m->drawable_count)return false;m->hidden[index]=hidden;
    for(size_t i=0;i<m->count;i++)if(m->parts[i].view.drawable_index==index)
        m->parts[i].view.hidden=hidden || !!(melee_pose_joint(m->pose,m->parts[i].owner)->flags&JOBJ_HIDDEN);
    return true;
}
static MeleeMaterial* material_for(MeleeModel* m,const MeleeArchive* a,uint32_t at)
{
    for(size_t i=0;i<m->material_count;i++)if(m->materials[i]->offset==at)return m->materials[i];
    if(m->material_count>=SIZE_MAX/sizeof(MeleeMaterial*))return NULL;
    MeleeMaterial** list=realloc(m->materials,(m->material_count+1)*sizeof(*list));if(!list)return NULL;m->materials=list;
    MeleeMaterial* material=melee_material_decode(a,at);if(!material)return NULL;
    list[m->material_count++]=material;return material;
}
static MeleeHostBool add_part(MeleeModel* m,const MeleeArchive* a,size_t owner,uint32_t drawable,uint32_t polygon,uint32_t material)
{
    uint32_t descriptor,display,flags;
    if(!range(a,polygon,24) || !ref(a,polygon+8,&descriptor) || !ref(a,polygon+16,&display) || !melee_archive_u32(a,polygon+12,&flags))return false;
    size_t length=(flags&65535)*32;
    if(!length)return true;
    if(!range(a,display,length))return false;
    MeleeVertexAttribute attrs[26];size_t count=0;
    for(;;){
        uint32_t attr,mode,components,format,layout,array;
        if(!melee_archive_u32(a,descriptor,&attr))return false;if(attr==255)break;
        if(count==26 || !range(a,descriptor,24) || !melee_archive_u32(a,descriptor+4,&mode) || !melee_archive_u32(a,descriptor+8,&components) || !melee_archive_u32(a,descriptor+12,&format) || !melee_archive_u32(a,descriptor+16,&layout) || !ref(a,descriptor+20,&array))return false;
        attrs[count++]=(MeleeVertexAttribute){.attribute=attr,.mode=mode,.components=components,.format=format,.fraction=layout>>24,.stride=layout&255,.array=array==UINT32_MAX?NULL:a->bytes+32+array,.array_size=array==UINT32_MAX?0:a->data_size-array};
        descriptor+=24;
    }
    if(m->count>=SIZE_MAX/sizeof(Part))return false;
    Part* parts=realloc(m->parts,(m->count+1)*sizeof(*parts));if(!parts)return false;m->parts=parts;
    Part* p=m->parts+m->count++;memset(p,0,sizeof(*p));p->owner=owner;
    p->batch=melee_vertex_decode(a->bytes+32+display,length,attrs,count,0);
    p->skin=melee_skin_decode(a,polygon,m->pose,owner);
    if(!p->batch || !p->skin)return false;
    size_t vertices=melee_vertex_count(p->batch);
    if(vertices>SIZE_MAX/sizeof(MeleeVertex))return false;
    p->vertices=vertices?malloc(vertices*sizeof(*p->vertices)):NULL;if(vertices && !p->vertices)return false;
    p->view=(MeleeModelPart){.joint_offset=melee_pose_source_offset(m->pose,owner),.drawable_offset=drawable,.polygon_offset=polygon,.material_offset=material,.polygon_flags=flags>>16,.vertex_count=vertices,.draw_count=melee_draw_count(p->batch),.vertices=p->vertices,.draws=melee_draw_data(p->batch)};
    p->view.drawable_index=m->drawable_count-1;
    if(material!=UINT32_MAX){p->view.material=material_for(m,a,material);if(!p->view.material)return false;}
    return true;
}
static MeleeHostBool polygons(MeleeModel* m,const MeleeArchive* a,size_t owner,uint32_t drawable,uint32_t at,uint32_t material)
{
    Seen seen={0};MeleeHostBool okay=true;
    while(at!=UINT32_MAX){
        if((at&3) || !visit(&seen,at) || !add_part(m,a,owner,drawable,at,material) || !ref(a,at+4,&at)){okay=false;break;}
    }
    free(seen.offsets);return okay;
}
static MeleeHostBool drawables(MeleeModel* m,const MeleeArchive* a,size_t owner,uint32_t at)
{
    Seen seen={0};MeleeHostBool okay=true;
    while(at!=UINT32_MAX){
        uint32_t polygon,material;
        if((at&3) || !range(a,at,16) || !visit(&seen,at) || !ref(a,at+8,&material) || !ref(a,at+12,&polygon)){okay=false;break;}
        if(m->drawable_count==SIZE_MAX/sizeof(*m->hidden)){okay=false;break;}
        MeleeHostBool* flags=realloc(m->hidden,(m->drawable_count+1)*sizeof(*flags));if(!flags){okay=false;break;}
        m->hidden=flags;flags[m->drawable_count++]=false;
        if(!polygons(m,a,owner,at,polygon,material) || !ref(a,at+4,&at)){okay=false;break;}
    }
    free(seen.offsets);return okay;
}
static MeleeHostBool update(MeleeModel* m)
{
    for(size_t i=0;i<m->count;i++){
        Part* p=m->parts+i;p->view.hidden=m->hidden[p->view.drawable_index] || !!(melee_pose_joint(m->pose,p->owner)->flags&JOBJ_HIDDEN);
        if(!melee_skin_apply(p->skin,p->batch,p->vertices,p->view.vertex_count))return false;
    }
    return true;
}
MeleeModel* melee_model_decode(const MeleeArchive* a,uint32_t root)
{
    MeleeModel* m=calloc(1,sizeof(*m));if(!m)return NULL;
    m->graph=melee_joint_decode(a,root);m->pose=melee_pose_create(m->graph);if(!m->pose)goto fail;
    for(size_t owner=0;owner<melee_pose_count(m->pose);owner++){
        const MeleeJointNode* n=NULL;
        for(size_t i=0;i<melee_joint_count(m->graph);i++)if(melee_joint_node(m->graph,i)->offset==melee_pose_source_offset(m->pose,owner)){n=melee_joint_node(m->graph,i);break;}
        if(!n)goto fail;
        if(n->flags&(JOBJ_SPLINE|JOBJ_PTCL)){if(n->payload_offset!=UINT32_MAX)goto fail;continue;}
        if(!drawables(m,a,owner,n->payload_offset))goto fail;
    }
    if(!update(m))goto fail;return m;
fail:melee_model_free(m);return NULL;
}
MeleeHostBool melee_model_bind(MeleeModel* m,MeleeFighterAnimation* a){return m && melee_pose_bind(m->pose,a,NULL);}
MeleeHostBool melee_model_request(MeleeModel* m,float frame){return m && melee_pose_request(m->pose,frame);}
MeleeHostBool melee_model_step(MeleeModel* m){return m && melee_pose_step(m->pose) && update(m);}
