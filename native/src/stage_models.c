#include "melee_stage_models.h"
#include "melee_scene.h"
#include "melee_environment.h"
#include "melee_lights.h"
#include "melee_camera.h"
#include <sysdolphin/baselib/cobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(GrJoint)==6,"Joint map scalar layout");
#define NONE UINT32_MAX
#define VARIANTS 64
typedef struct {
    struct UnkStageDat_x8_t data;
    MeleeScene* scenes[VARIANTS+1];
    MeleeEnvironment* environment;
    MeleeCameraAnimation* camera_animation;
    HSD_AnimJoint* joints[VARIANTS+1];
    HSD_MatAnimJoint* materials[VARIANTS+1];
    HSD_ShapeAnimJoint* shapes[VARIANTS+1];
    int loaded,unused_joint_animation;
} Model;
struct MeleeStageModels {MeleeArchive archive;u8* bytes;u32 table,root;unsigned count;Model* models;MeleeScene** spline_owners;HSD_Spline** splines;unsigned spline_count;struct GroundShadowEntry* shadows;GroundJointMapEntry* maps;unsigned map_count;int maps_loaded;LightOverrideEntry* light_overrides;unsigned light_count;int lights_loaded;MeleeLights** extra_lights;unsigned extra_count;UnkStageDat header;struct UnkStageDat_x8_t* header_models;int header_loaded;};
static int span(const MeleeArchive* a,u32 at,size_t n){return at<=a->data_size&&n<=a->data_size-at;}
static int ref(const MeleeArchive* a,u32 at,u32* value){MeleeHostBool p;if(!melee_archive_pointer(a,at,value,&p))return 0;if(!p)*value=NONE;return 1;}
static void cleanup(Model* m){
    for(unsigned i=0;i<=VARIANTS;i++)melee_scene_free(m->scenes[i]);
    melee_camera_animation_free(m->camera_animation);melee_environment_free(m->environment);free(m->data.unk20);free(m->data.x2C);memset(m,0,sizeof(*m));
}
void melee_stage_models_free(MeleeStageModels* s){if(s){for(unsigned i=0;i<s->spline_count;i++)melee_scene_free(s->spline_owners[i]);free(s->spline_owners);free(s->splines);free(s->shadows);free(s->header_models);for(unsigned i=0;i<s->extra_count;i++)melee_lights_free(s->extra_lights[i]);free(s->extra_lights);free(s->light_overrides);for(unsigned i=0;i<s->map_count;i++)free(s->maps[i].pairs);free(s->maps);for(unsigned i=0;i<s->count;i++)cleanup(s->models+i);free(s->models);free(s->bytes);free(s);}}
unsigned melee_stage_models_count(const MeleeStageModels* s){return s?s->count:0;}
MeleeHostBool melee_stage_models_omit_unused_joint_animation(MeleeStageModels* s,unsigned index){
    if(!s||index>=s->count||s->header_loaded||s->models[index].loaded)return false;
    s->models[index].unused_joint_animation=1;return true;
}
MeleeStageModels* melee_stage_models_decode(const MeleeArchive* a){
    /* Each Stadium archive supplies only some model slots. Like the original
     * archive loader, leave unresolved external chains NULL until their owning
     * archive is loaded; their on-disc chain values are not descriptors. */
    if(a&&a->extern_count){
        size_t size;u8* bytes=melee_archive_copy_null_externals(a,&size);if(!bytes)return NULL;
        MeleeArchive local;MeleeStageModels* result=NULL;
        if(melee_archive_open(&local,bytes,size))result=melee_stage_models_decode(&local);
        free(bytes);return result;
    }
    u32 root,table,n;if(!a||!melee_archive_find(a,"map_head",&root)||!span(a,root,48)||
       !ref(a,root+8,&table)||!melee_archive_u32(a,root+12,&n)||n>256||(n&&(table==NONE||!span(a,table,52u*n))))return NULL;
    MeleeStageModels* s=calloc(1,sizeof(*s));if(!s)return NULL;s->table=table;s->root=root;
    s->bytes=malloc(a->size);s->models=calloc(n?n:1,sizeof(*s->models));if(!s->bytes||!s->models)goto fail;
    memcpy(s->bytes,a->bytes,a->size);if(!melee_archive_open(&s->archive,s->bytes,a->size))goto fail;
    /* Original grDatFiles_801C6228 sets this render-mode bit on each listed
     * MObj descriptor. Apply it before conversion so every owned scene copy
     * receives it; GameCube +4 is not a native descriptor field offset. */
    u32 overrides,no;
    if(!ref(&s->archive,root+40,&overrides)||!melee_archive_u32(&s->archive,root+44,&no)||no>65536||
       (no&&(overrides==NONE||!span(&s->archive,overrides,4u*no))))goto fail;
    for(unsigned i=0;i<no;i++){
        u32 material,flags;if(!ref(&s->archive,overrides+4*i,&material))goto fail;
        if(material==NONE)continue;
        if(!span(&s->archive,material,20)||!melee_archive_u32(&s->archive,material+4,&flags))goto fail;
        flags|=0x04000000;u8* p=s->bytes+32+material+4;p[0]=flags>>24;p[1]=flags>>16;p[2]=flags>>8;p[3]=flags;
    }
    s->count=n;return s;
fail:melee_stage_models_free(s);return NULL;
}
static int anims(const MeleeArchive* a,u32 at,u32* roots,unsigned* n){
    *n=0;if(at==NONE)return 1;
    for(unsigned i=0;i<=VARIANTS;i++){
        u32 r;if((uint64_t)at+4*i+4>a->data_size||!ref(a,at+4*i,&r))return 0;
        if(r==NONE)return 1;if(i==VARIANTS)return 0;roots[(*n)++]=r;
    }return 0;
}
static int halves(const MeleeArchive* a,u32 at,unsigned n,s16** out){
    if(!n)return at==NONE||span(a,at,0);
    if(at==NONE||!span(a,at,2u*n))return 0;
    *out=malloc(2u*n);if(!*out)return 0;
    for(unsigned i=0;i<n;i++){const u8* p=a->bytes+32+at+2*i;u16 v=(p[0]<<8)|p[1];memcpy(*out+i,&v,2);}return 1;
}
struct UnkStageDat_x8_t* melee_stage_models_get(MeleeStageModels* s,unsigned index){
    if(!s||index>=s->count)return NULL;Model* m=s->models+index;if(m->loaded)return &m->data;
    const MeleeArchive* a=&s->archive;u32 r[13],at=s->table+52*index;
    for(unsigned i=0;i<13;i++){
        if(i==9||i==12){if(!melee_archive_u32(a,at+4*i,r+i))goto fail;}
        else if(!ref(a,at+4*i,r+i))goto fail;
    }
    if(m->unused_joint_animation)r[1]=NONE;
    if(r[5]!=NONE){
        m->camera_animation=melee_camera_animation_decode(a,r[5]);
        if(!m->camera_animation){
            fprintf(stderr,"Stage model %u camera animation decode failed\n",index);goto fail;
        }
        m->data.x14=melee_camera_animation_descriptor(m->camera_animation);
    }
    if(r[9]>4096||r[12]>4096)goto fail;
    u32 roots[3][VARIANTS];unsigned counts[3],count=0;
    for(unsigned k=0;k<3;k++){if(!anims(a,r[k+1],roots[k],counts+k))goto fail;if(counts[k]>count)count=counts[k];}
    if(r[0]==NONE){if(count)goto fail;}else{
        for(unsigned i=0;i<(count?count:1);i++){
            MeleeScene* scene=m->scenes[i]=melee_scene_decode(a,r[0]);if(!scene){fprintf(stderr,"Stage model %u geometry failed\n",index);goto fail;}
            for(unsigned k=0;k<3;k++)if(i<counts[k]){
                int okay=k==0?melee_scene_bind_joints(scene,roots[k][i]):k==1?melee_scene_bind_materials(scene,roots[k][i]):melee_scene_bind_shapes(scene,roots[k][i]);
                if(!okay){fprintf(stderr,"Stage model %u animation %u kind %u failed\n",index,i,k);goto fail;}
            }
            if(!i)m->data.unk0=melee_scene_joint_descriptor(scene);
            m->joints[i]=melee_scene_animation_descriptor(scene);m->materials[i]=melee_scene_material_descriptor(scene);m->shapes[i]=melee_scene_shape_descriptor(scene);
            melee_scene_release_objects(scene);
        }
    }
    m->data.unk4=r[1]==NONE?NULL:m->joints;m->data.unk8=r[2]==NONE?NULL:m->materials;m->data.unkC=r[3]==NONE?NULL:m->shapes;
    m->environment=melee_environment_decode(a,r[4],r[6],r[7]);if(!m->environment){fprintf(stderr,"Stage model %u environment failed\n",index);goto fail;}
    m->data.x10=(HSD_CameraDescPerspective*)melee_environment_camera(m->environment);
    m->data.x18=melee_environment_lights(m->environment);m->data.x1C=melee_environment_fog(m->environment);
    s16* mapping=NULL;if(!halves(a,r[8],3*r[9],&mapping))goto fail;
    m->data.unk20=(GrJoint*)mapping;m->data.unk24=r[9];
    if(!halves(a,r[11],r[12],&m->data.x2C))goto fail;m->data.x30=r[12];
    if(r[10]!=NONE){if(!span(a,r[10],count?count:1))goto fail;m->data.x28=s->bytes+32+r[10];}
    m->loaded=1;return &m->data;
fail:cleanup(m);return NULL;
}

MeleeHostBool melee_stage_models_point_maps(MeleeStageModels* s,GroundJointMapEntry** out,unsigned* count){
    if(!s||!out||!count)return false;
    if(s->maps_loaded){*out=s->maps;*count=s->map_count;return true;}
    const MeleeArchive* a=&s->archive;u32 table,n;
    if(!ref(a,s->root,&table)||!melee_archive_u32(a,s->root+4,&n)||n>256||
       (n&&(table==NONE||!span(a,table,12u*n))))return false;
    GroundJointMapEntry* maps=NULL;unsigned used=0;
    for(unsigned i=0;i<n;i++){
        u32 joint,pairs,np,at=table+12*i;
        if(!ref(a,at,&joint)||joint==NONE||!ref(a,at+4,&pairs)||
           !melee_archive_u32(a,at+8,&np)||np>4096||
           (np&&(pairs==NONE||!span(a,pairs,4u*np))))goto fail;
        unsigned matches=0;
        for(unsigned k=0;k<s->count;k++){
            u32 model_root;if(!ref(a,s->table+52*k,&model_root))goto fail;
            if(model_root!=joint)continue;
            struct UnkStageDat_x8_t* model=melee_stage_models_get(s,k);if(!model||!model->unk0)goto fail;
            size_t joints=melee_scene_joint_count(s->models[k].scenes[0]);
            s16* values=NULL;if(!halves(a,pairs,2*np,&values))goto fail;
            for(unsigned j=0;j<np;j++){
                if(values[2*j]<0||(unsigned)values[2*j]>=joints||values[2*j+1]<0||values[2*j+1]>=261){free(values);goto fail;}
            }
            GroundJointMapEntry* grown=realloc(maps,(used+1)*sizeof(*maps));
            if(!grown){free(values);goto fail;}maps=grown;
            maps[used++]=(GroundJointMapEntry){model->unk0,values,np};matches++;
        }
        if(!matches)goto fail;
    }
    s->maps=maps;s->map_count=used;s->maps_loaded=1;*out=maps;*count=used;return true;
fail:for(unsigned i=0;i<used;i++)free(maps[i].pairs);free(maps);return false;
}

MeleeHostBool melee_stage_models_light_overrides(MeleeStageModels* s,LightOverrideEntry** out,unsigned* count){
    if(!s||!out||!count)return false;
    if(s->lights_loaded){*out=s->light_overrides;*count=s->light_count;return true;}
    const MeleeArchive* a=&s->archive;u32 table,words;
    /* Retail map_head stores this list's length in 32-bit words. Each entry
     * comprises a relocated descriptor word and a packed flag word. */
    if(!ref(a,s->root+24,&table)||!melee_archive_u32(a,s->root+28,&words)||words>1024||(words&1)||
       (words&&(table==NONE||!span(a,table,4u*words))))return false;
    unsigned n=words/2,used=0,extras=0;LightOverrideEntry* entries=NULL;
    MeleeLights** lights=calloc(n?n:1,sizeof(*lights));if(!lights)return false;
    for(unsigned i=0;i<n;i++){
        u32 offset,flags;if(!ref(a,table+8*i,&offset)||!melee_archive_u32(a,table+8*i+4,&flags))goto fail;
        if(offset==NONE)continue;
        if(flags&0x1fffffff)goto fail;
        unsigned found=0;
        for(unsigned k=0;k<=s->count;k++){
            HSD_LightDesc* d;
            if(k<s->count){
                if(!melee_stage_models_get(s,k))goto fail;
                d=melee_environment_find_light(s->models[k].environment,offset);if(!d)continue;
            }else{
                if(found)break;lights[extras]=melee_lights_decode(a,offset);if(!lights[extras])goto fail;
                d=melee_lights_descriptor(lights[extras++]);
            }
            LightOverrideEntry* grown=realloc(entries,(used+1)*sizeof(*entries));if(!grown)goto fail;entries=grown;
            LightOverrideEntry* e=&entries[used++];memset(e,0,sizeof(*e));e->desc=d;
            e->a=(flags>>31)&1;e->b=(flags>>30)&1;e->c=(flags>>29)&1;found++;
        }
    }
    s->light_overrides=entries;s->light_count=used;s->extra_lights=lights;s->extra_count=extras;s->lights_loaded=1;
    *out=entries;*count=used;return true;
fail:for(unsigned i=0;i<extras;i++)melee_lights_free(lights[i]);free(lights);free(entries);return false;
}

UnkStageDat* melee_stage_models_header(MeleeStageModels* s){
    if(!s)return NULL;if(s->header_loaded)return &s->header;
    const MeleeArchive* a=&s->archive;u32 spline,ns,shadow,nh;
    if(!ref(a,s->root+16,&spline)||!melee_archive_u32(a,s->root+20,&ns)||
       !ref(a,s->root+32,&shadow)||!melee_archive_u32(a,s->root+36,&nh))return NULL;
    if(ns>256||nh>4096||(ns&&(spline==NONE||!span(a,spline,4u*ns)))||(nh&&(shadow==NONE||!span(a,shadow,8u*nh))))return NULL;
    GroundJointMapEntry* maps;LightOverrideEntry* lights;unsigned nm,nl;
    if(!melee_stage_models_point_maps(s,&maps,&nm)||!melee_stage_models_light_overrides(s,&lights,&nl))return NULL;
    struct UnkStageDat_x8_t* models=calloc(s->count?s->count:1,sizeof(*models));if(!models)return NULL;
    for(unsigned i=0;i<s->count;i++){
        struct UnkStageDat_x8_t* m=melee_stage_models_get(s,i);if(!m){free(models);return NULL;}models[i]=*m;
    }
    /* Shared archive animation offsets must retain pointer identity across
     * model environments: Ground uses them as shadow-table keys. */
    for(unsigned i=0;i<s->count;i++){
        LightList** list=models[i].x18;if(!list)continue;
        u32 table;if(!ref(a,s->table+52*i+24,&table)||table==NONE){free(models);return NULL;}
        for(unsigned j=0;list[j];j++){
            if(!list[j]->anims)continue;
            u32 entry,animations;
            if(!ref(a,table+4*j,&entry)||entry==NONE||!ref(a,entry+4,&animations)||animations==NONE){free(models);return NULL;}
            for(unsigned k=0;list[j]->anims[k];k++){
                u32 offset;if(!ref(a,animations+4*k,&offset)||offset==NONE){free(models);return NULL;}
                HSD_LightAnim* canonical=melee_stage_models_find_light_animation(s,offset);
                if(!canonical){free(models);return NULL;}list[j]->anims[k]=canonical;
            }
        }
    }
    MeleeScene** owners=ns?calloc(ns,sizeof(*owners)):NULL;HSD_Spline** splines=ns?calloc(ns,sizeof(*splines)):NULL;
    struct GroundShadowEntry* shadows=nh?calloc(nh,sizeof(*shadows)):NULL;
    if((ns&&(!owners||!splines))||(nh&&!shadows))goto fail_tables;
    for(unsigned i=0;i<ns;i++){
        u32 at;if(!ref(a,spline+4*i,&at)||at==NONE||!(owners[i]=melee_scene_decode_spline(a,at,&splines[i])))goto fail_tables;
    }
    for(unsigned i=0;i<nh;i++){
        u32 at,flags;if(!ref(a,shadow+8*i,&at)||at==NONE||!melee_archive_u32(a,shadow+8*i+4,&flags)||(flags&0x7fffffff))goto fail_tables;
        for(unsigned j=0;j<s->count&&!shadows[i].unk0;j++)shadows[i].unk0=melee_environment_find_light_animation(s->models[j].environment,at);
        if(!shadows[i].unk0)goto fail_tables;shadows[i].flag=flags>>31;
    }
    s->spline_owners=owners;s->splines=splines;s->spline_count=ns;s->shadows=shadows;
    s->header_models=models;
    s->header=(UnkStageDat){.unk0=maps,.unk4=nm,.unk8=models,.unkC=s->count,.unk18=lights,.unk1C=nl,.unk10=splines,.unk14=ns,.unk20=shadows,.unk24=nh};
    /* The material override bit was applied before model conversion. Leaving
     * that processing list empty prevents the original loader applying disk
     * field offsets to widened native descriptors a second time. */
    s->header_loaded=1;return &s->header;
fail_tables:
    if(owners)for(unsigned i=0;i<ns;i++)melee_scene_free(owners[i]);
    free(owners);free(splines);free(shadows);free(models);return NULL;
}

HSD_LightDesc* melee_stage_models_find_light(MeleeStageModels* s,uint32_t offset){
    LightOverrideEntry* entries;unsigned count;if(!melee_stage_models_light_overrides(s,&entries,&count))return NULL;
    for(unsigned i=0;i<s->count;i++){HSD_LightDesc* d=melee_environment_find_light(s->models[i].environment,offset);if(d)return d;}
    for(unsigned i=0;i<s->extra_count;i++){HSD_LightDesc* d=melee_lights_find(s->extra_lights[i],offset);if(d)return d;}return NULL;
}

HSD_LightAnim* melee_stage_models_find_light_animation(MeleeStageModels* s,u32 offset){
    if(s)for(unsigned i=0;i<s->count;i++){HSD_LightAnim* a=melee_environment_find_light_animation(s->models[i].environment,offset);if(a)return a;}
    return NULL;
}

HSD_ImageDesc* melee_stage_models_find_image(MeleeStageModels* s,const char* name){
    u32 at;if(!s||!name||!melee_archive_find(&s->archive,name,&at))return NULL;
    for(unsigned i=0;i<s->count;i++){
        if(!melee_stage_models_get(s,i))return NULL;
        HSD_ImageDesc* image=melee_scene_find_image(s->models[i].scenes[0],at);
        if(image)return image;
    }
    return NULL;
}
