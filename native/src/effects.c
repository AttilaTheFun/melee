#include "melee_effects.h"
#include "melee_scene.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NONE UINT32_MAX
struct MeleeEffects {MeleeArchive archive;u8* bytes;uint32_t root;unsigned count;EF_EffectDesc* models;MeleeScene** scenes;MeleeParticleBank* particles;};
static int pointer(const MeleeArchive* a,uint32_t slot,uint32_t* out){MeleeHostBool p;if(!melee_archive_pointer(a,slot,out,&p))return 0;if(!p)*out=NONE;return 1;}
void melee_effects_free(MeleeEffects* e){if(!e)return;if(e->scenes)for(unsigned i=0;i<e->count;i++)melee_scene_free(e->scenes[i]);free(e->scenes);free(e->models);melee_particle_bank_free(e->particles);free(e->bytes);free(e);}
unsigned melee_effects_count(const MeleeEffects* e){return e?e->count:0;}
MeleeParticleBank* melee_effects_particles(MeleeEffects* e){return e?e->particles:NULL;}
MeleeEffects* melee_effects_decode(const MeleeArchive* a,const char* symbol){
    uint32_t root,cmd,tex;if(!a||a->extern_count||!melee_archive_find(a,symbol,&root)||!pointer(a,root,&cmd)||!pointer(a,root+4,&tex))return NULL;
    MeleeEffects* e=calloc(1,sizeof(*e));if(!e)return NULL;e->root=root;e->bytes=malloc(a->size);if(!e->bytes)goto fail;
    memcpy(e->bytes,a->bytes,a->size);if(!melee_archive_open(&e->archive,e->bytes,a->size))goto fail;
    uint32_t boundary=a->data_size;if(cmd!=NONE&&cmd<boundary)boundary=cmd;if(tex!=NONE&&tex<boundary)boundary=tex;
    /* These retail tables end before texture payload, not at the first
     * referenced joint. Ness has three records; Donkey's seven correspond
     * to effects 1222..1228. Link has four records before its texture
     * payload begins at 0x60. Peach has one record before texture payload
     * at 0x20. Kirby Fox has one record before texture data at 0x20; Kirby Donkey has two before payload at 0x30. These archives store no record count. */
    unsigned known_count=!strcmp(symbol,"effNessDataTable")?3:
        !strcmp(symbol,"effDonkeyDataTable")?7:
        !strcmp(symbol,"effLinkDataTable")?4:
        !strcmp(symbol,"effPeachDataTable")?1:
        !strcmp(symbol,"effKirbyFoxDataTable")?1:
        !strcmp(symbol,"effKirbyDonkeyDataTable")?2:
        !strcmp(symbol,"effKirbyMarsDataTable")?2:
        !strcmp(symbol,"effKirbyEmblemDataTable")?2:
        !strcmp(symbol,"effKirbyZeldaDataTable")?2:0;
    if(known_count){
        uint64_t end=(uint64_t)root+8+known_count*20;
        if(end>boundary)goto fail;boundary=(uint32_t)end;
    }
    uint64_t cursor=(uint64_t)root+8;
    while(cursor+20<=boundary){
        if(e->count==1000)goto fail;float lifetime;
        if(!melee_archive_f32(a,cursor,&lifetime)||!isfinite(lifetime))goto fail;
        for(unsigned k=0;k<4;k++){uint32_t ref;if(!pointer(a,cursor+4+4*k,&ref))goto fail;
            if(ref!=NONE){if(ref<cursor+20)goto fail;if(ref<boundary)boundary=ref;}}
        e->count++;cursor+=20;
    }
    if(!e->count)goto fail;
    for(;cursor<boundary;cursor++)if(a->bytes[32+cursor])goto fail;
    e->models=calloc(e->count,sizeof(*e->models));e->scenes=calloc(e->count,sizeof(*e->scenes));if(!e->models||!e->scenes)goto fail;
    for(unsigned i=0;i<e->count;i++)melee_archive_f32(a,root+8+20*i,&e->models[i].lifetime);
    if(cmd!=NONE||tex!=NONE){if(cmd==NONE||tex==NONE||cmd>=tex)goto fail;
        e->particles=melee_particle_bank_decode(a,cmd,tex-cmd,tex,a->data_size-tex);if(!e->particles)goto fail;}
    return e;
fail:melee_effects_free(e);return NULL;
}
EF_EffectDesc* melee_effects_model(MeleeEffects* e,unsigned i){
    if(!e||i>=e->count)return NULL;if(e->scenes[i])return &e->models[i];
    uint32_t r[4];for(unsigned k=0;k<4;k++)if(!pointer(&e->archive,e->root+12+20*i+4*k,&r[k]))return NULL;
    if(r[0]==NONE)return r[1]==NONE&&r[2]==NONE&&r[3]==NONE?&e->models[i]:NULL;
    MeleeScene* s=melee_scene_decode(&e->archive,r[0]);if(!s){fprintf(stderr,"Effect %u joint decode failed\n",i);return NULL;}
    for(unsigned k=1;k<4;k++)if(r[k]!=NONE){
        int okay=k==1?melee_scene_bind_joints(s,r[k]):k==2?melee_scene_bind_materials(s,r[k]):melee_scene_bind_shapes(s,r[k]);
        if(!okay){fprintf(stderr,"Effect %u animation kind %u decode failed\n",i,k);melee_scene_free(s);return NULL;}
    }
    e->models[i].model_desc=(StaticModelDesc){melee_scene_joint_descriptor(s),melee_scene_animation_descriptor(s),melee_scene_material_descriptor(s),melee_scene_shape_descriptor(s)};
    melee_scene_release_objects(s);e->scenes[i]=s;return &e->models[i];
}
