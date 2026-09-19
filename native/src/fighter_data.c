#include "melee_fighter_data.h"
#include "melee_fighter_common.h"
#include "melee_fighter_parts.h"
#include "melee_fighter_shakes.h"
#include "melee_fighter_modifiers.h"
#include "melee_fighter_aux.h"
#include "melee_fighter_models.h"
#include "melee_fighter_cpu.h"
#include "melee_item_colors.h"
#include <stdlib.h>
struct MeleeFighterData {
    void* entries[23];
    void* respawn[2];
    ftCommonData common;
    MeleeFighterModifiers modifiers;
    MeleeFighterAux aux;
    MeleeFighterParts* parts;
    MeleeFighterShakes* shakes;
    MeleeFighterModels* models;
    MeleeFighterCpu* cpu;
    MeleeItemColors* colors[2];
};
void melee_fighter_data_free(MeleeFighterData* owner)
{
    if(owner){
        melee_fighter_parts_free(owner->parts);melee_fighter_shakes_free(owner->shakes);
        melee_fighter_models_free(owner->models);melee_fighter_cpu_free(owner->cpu);
        for(unsigned i=0;i<2;i++)melee_item_colors_free(owner->colors[i]);free(owner);
    }
}
void** melee_fighter_data_entries(MeleeFighterData* owner){return owner?owner->entries:NULL;}
MeleeFighterData* melee_fighter_data_decode(const MeleeArchive* a)
{
    MeleeFighterData* d=calloc(1,sizeof(*d));if(!d)return NULL;
    if(!melee_fighter_common_decode(a,&d->common)||
       !melee_fighter_modifiers_decode(a,&d->modifiers)||
       !melee_fighter_aux_decode(a,&d->aux))goto fail;
    d->parts=melee_fighter_parts_decode(a);if(!d->parts)goto fail;
    d->shakes=melee_fighter_shakes_decode(a);if(!d->shakes)goto fail;
    d->models=melee_fighter_models_decode(a);if(!d->models)goto fail;
    d->cpu=melee_fighter_cpu_decode(a);if(!d->cpu)goto fail;
    u32 root;if(!melee_archive_find(a,"ftLoadCommonData",&root))goto fail;
    for(unsigned i=0;i<2;i++){
        u32 table;MeleeHostBool present;
        if(!melee_archive_pointer(a,root+24+4*i,&table,&present)||!present)goto fail;
        d->colors[i]=melee_fighter_colors_decode(a,table,i?6:123);if(!d->colors[i])goto fail;
    }
    d->respawn[0]=melee_fighter_models_joint(d->models,0);
    d->respawn[1]=melee_fighter_models_respawn_animation(d->models);
    struct Fighter_ShakeTable_t* shakes=melee_fighter_shakes_tables(d->shakes);
    void* entries[23]={&d->common,d->modifiers.throws,d->modifiers.swing,d->modifiers.staling,
        melee_fighter_parts_tables(d->parts),melee_fighter_parts_accessories(d->parts),
        melee_item_colors_entries(d->colors[0]),melee_item_colors_entries(d->colors[1]),
        d->respawn,shakes,shakes+3,shakes+4,&d->modifiers.scale,&d->modifiers.bunny,
        &d->modifiers.metal,&d->modifiers.gravity,melee_fighter_models_joint(d->models,1),
        d->aux.colors[0],d->aux.colors[1],d->aux.colors[2],melee_fighter_models_joint(d->models,2),
        &d->aux.crowd,melee_fighter_cpu_header(d->cpu)};
    for(unsigned i=0;i<23;i++)d->entries[i]=entries[i];return d;
fail:melee_fighter_data_free(d);return NULL;
}
