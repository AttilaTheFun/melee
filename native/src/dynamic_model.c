#include "melee_dynamic_model.h"
#include "melee_scene.h"
#include <stdlib.h>
#define LIMIT 64
#define NONE UINT32_MAX
struct MeleeDynamicModel {
    DynamicModelDesc desc;
    unsigned counts[3];
    MeleeScene* scenes[LIMIT];
    HSD_AnimJoint* joints[LIMIT+1];
    HSD_MatAnimJoint* materials[LIMIT+1];
    HSD_ShapeAnimJoint* shapes[LIMIT+1];
};
static int ref(const MeleeArchive* a,u32 at,u32* value){MeleeHostBool present;if(!melee_archive_pointer(a,at,value,&present))return 0;if(!present)*value=NONE;return 1;}
static int variants(const MeleeArchive* a,u32 at,u32* roots,unsigned* count){
    *count=0;if(at==NONE)return 1;
    for(unsigned i=0;i<=LIMIT;i++){
        u32 r;if((uint64_t)at+4*i+4>a->data_size||!ref(a,at+4*i,&r))return 0;
        if(r==NONE)return 1;if(i==LIMIT)return 0;roots[(*count)++]=r;
    }return 0;
}
void melee_dynamic_model_free(MeleeDynamicModel* m){if(m){for(unsigned i=0;i<LIMIT;i++)melee_scene_free(m->scenes[i]);free(m);}}
DynamicModelDesc* melee_dynamic_model_descriptor(MeleeDynamicModel* m){return m?&m->desc:NULL;}
unsigned melee_dynamic_model_animation_count(const MeleeDynamicModel* m,unsigned kind){return m&&kind<3?m->counts[kind]:0;}
MeleeDynamicModel* melee_dynamic_model_decode(const MeleeArchive* a,const char* symbol){
    u32 root;if(!a||!symbol||!melee_archive_find(a,symbol,&root))return NULL;
    return melee_dynamic_model_decode_at(a,root);
}
MeleeDynamicModel* melee_dynamic_model_decode_at(const MeleeArchive* a,u32 root){
    u32 r[4],roots[3][LIMIT];
    if(!a||root>a->data_size||a->data_size-root<16)return NULL;
    for(unsigned k=0;k<4;k++)if(!ref(a,root+4*k,r+k))return NULL;
    if(r[0]==NONE)return NULL;
    MeleeDynamicModel* m=calloc(1,sizeof(*m));if(!m)return NULL;unsigned count=1;
    for(unsigned k=0;k<3;k++){if(!variants(a,r[k+1],roots[k],m->counts+k))goto fail;if(m->counts[k]>count)count=m->counts[k];}
    for(unsigned i=0;i<count;i++){
        MeleeScene* s=m->scenes[i]=melee_scene_decode(a,r[0]);if(!s)goto fail;
        for(unsigned k=0;k<3;k++)if(i<m->counts[k]){
            int okay=k==0?melee_scene_bind_joints(s,roots[k][i]):k==1?melee_scene_bind_materials(s,roots[k][i]):melee_scene_bind_shapes(s,roots[k][i]);
            if(!okay)goto fail;
        }
        if(!i)m->desc.joint=melee_scene_joint_descriptor(s);
        m->joints[i]=melee_scene_animation_descriptor(s);m->materials[i]=melee_scene_material_descriptor(s);m->shapes[i]=melee_scene_shape_descriptor(s);
        melee_scene_release_objects(s);
    }
    m->desc.anims=r[1]==NONE?NULL:m->joints;m->desc.matanims=r[2]==NONE?NULL:m->materials;m->desc.shapeanims=r[3]==NONE?NULL:m->shapes;
    return m;
fail:melee_dynamic_model_free(m);return NULL;
}
