/* Explicit model diagnostic: non-model action events are counted, not executed.
 * The production action API still rejects events without a real handler. */
#include "fighter_selection.h"
#include "melee_visibility.h"
#include "melee_action.h"
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/tobj.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftmaterial.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
struct FighterSelectionProbe {
    MeleeVisibility* visibility;
    MeleeAction* action;
    Fighter* appearance;
    HSD_GObj appearance_object;
    HSD_TObj* expressions[16];size_t expression_count;
    HSD_DObj* objects[124];size_t count,applied,skipped;uint64_t opcodes;
};
static int collect(FighterSelectionProbe* p,HSD_JObj* joint){
    for(;joint;joint=joint->next){
        for(HSD_DObj* d=joint->u.dobj;d;d=d->next){if(p->count==124)return 0;p->objects[p->count++]=d;}
        if(!collect(p,joint->child))return 0;
    }
    return 1;
}
static int pointer(MeleeArchive* a,uint32_t slot,uint32_t* out){
    MeleeHostBool present;return melee_archive_pointer(a,slot,out,&present)&&present;
}
FighterSelectionProbe* fighter_selection_probe_create(MeleeScene* scene,const char* path,const char* symbol,const char* motion){
    const char* symbols[]={"ftDataMario","ftDataFox","ftDataKirby","ftDataPeach","ftDataGamewatch"};
    const unsigned kinds[]={0,1,4,9,24};unsigned kind=UINT32_MAX;
    for(unsigned i=0;i<5;i++)if(!strcmp(symbol,symbols[i]))kind=kinds[i];
    if(kind==UINT32_MAX)return NULL;
    FighterSelectionProbe* p=calloc(1,sizeof(*p));if(!p)return NULL;
    uint8_t* bytes=NULL;MeleeVisibility* other[3]={0};FILE* f=fopen(path,"rb");
    if(!f)goto fail;
    if(fseek(f,0,SEEK_END)){fclose(f);goto fail;}long size=ftell(f);rewind(f);
    if(size<=32){fclose(f);goto fail;}bytes=malloc(size);
    if(!bytes||fread(bytes,1,size,f)!=(size_t)size){fclose(f);goto fail;}fclose(f);
    MeleeArchive a;uint32_t root,desc,table,name,script;
    if(!collect(p,melee_scene_root(scene))||!melee_archive_open(&a,bytes,size)||!melee_archive_find(&a,symbol,&root)||
       !pointer(&a,root+8,&desc)||!pointer(&a,root+12,&table)||table>a.data_size||a.data_size-table<72||
       !pointer(&a,table+48,&name)||name>=a.data_size||!memchr(a.bytes+32+name,0,a.data_size-name)||
       strcmp((const char*)a.bytes+32+name,motion)||!pointer(&a,table+60,&script))goto fail;
    p->visibility=melee_visibility_decode(&a,desc,0,0,p->count);
    other[0]=melee_visibility_decode(&a,desc,0,1,p->count);other[1]=melee_visibility_decode(&a,desc,0,3,p->count);
    if(kind==24){
        uint32_t items,lookup,groups;
        if(!pointer(&a,root+0x48,&items)||!pointer(&a,items+40,&lookup)||!melee_archive_u32(&a,desc,&groups))goto fail;
        other[2]=melee_visibility_decode_lookup(&a,groups,lookup,p->count);if(!other[2])goto fail;
    }
    p->action=melee_action_decode(&a,script);
    if(!p->visibility||!other[0]||!other[1]||!p->action)goto fail;
    for(size_t i=0;i<p->count;i++)if(melee_visibility_controls(other[0],i)||melee_visibility_controls(other[1],i)||melee_visibility_controls(other[2],i))HSD_DObjSetFlags(p->objects[i],DOBJ_HIDDEN);
    if(!melee_visibility_bind(p->visibility,p->objects,p->count)||!melee_visibility_init_fighter(p->visibility,kind))goto fail;
    uint32_t expressionCount,expressionTable,indices;
    if(!melee_archive_u32(&a,desc+8,&expressionCount)||expressionCount>16)goto fail;
    p->expression_count=expressionCount;
    if(expressionCount){
        if(!pointer(&a,desc+12,&expressionTable)||!pointer(&a,expressionTable,&indices)||indices>a.data_size||a.data_size-indices<expressionCount*2)goto fail;
        for(unsigned k=0;k<expressionCount;k++){
            unsigned wanted=(unsigned)a.bytes[32+indices+k*2]<<8|a.bytes[33+indices+k*2],index=0;
            for(size_t i=0;i<p->count;i++)for(HSD_TObj* t=p->objects[i]->mobj?p->objects[i]->mobj->tobj:NULL;t;t=t->next,index++)if(index==wanted)p->expressions[k]=t;
            if(!p->expressions[k]||!p->expressions[k]->aobj)goto fail;
        }
        for(unsigned k=0;k<expressionCount;k++)HSD_AObjSetRate(p->expressions[k]->aobj,0);
    }
    if(kind==24){
        uint32_t attrs,common;float width,modelScale;
        if(!pointer(&a,root+4,&attrs)||attrs>a.data_size||a.data_size-attrs<24||
           !pointer(&a,root,&common)||!melee_archive_f32(&a,attrs,&width)||!isfinite(width)||width<=0||
           !melee_archive_f32(&a,common+0x8c,&modelScale)||!isfinite(modelScale)||modelScale<=0)goto fail;
        p->appearance=calloc(1,sizeof(*p->appearance));if(!p->appearance)goto fail;
        p->appearance_object.hsd_obj=melee_scene_root(scene);p->appearance_object.user_data=p->appearance;
        p->appearance->x34_scale=(Vec3){1,1,width};p->appearance->co_attrs.model_scaling=modelScale;
        memcpy(&p->appearance->x610_color_rgba[1],a.bytes+32+attrs+20,4);
        GXColor diffuse;memcpy(&diffuse,a.bytes+32+attrs+4,4);
        ftMaterial_800BFB4C(&p->appearance_object,&diffuse);
        Fighter_UpdateModelScale(&p->appearance_object);
        printf("Original Game & Watch appearance: width %.5f, model scale %.5f, diffuse %u/%u/%u/%u; outline pass pending\n",width,modelScale,diffuse.r,diffuse.g,diffuse.b,diffuse.a);
    }
    for(unsigned i=0;i<3;i++)melee_visibility_free(other[i]);free(bytes);return p;
fail:
    for(unsigned i=0;i<3;i++)melee_visibility_free(other[i]);free(bytes);
    fighter_selection_probe_free(p);return NULL;
}
static MeleeHostBool event(void* context,const MeleeActionEvent* e){
    FighterSelectionProbe* p=context;
    if(e->opcode>=31&&e->opcode<=33){p->applied++;return melee_visibility_command(p->visibility,e->words[0]);}
    p->skipped++;p->opcodes|=UINT64_C(1)<<e->opcode;return true;
}
MeleeHostBool fighter_selection_probe_step(FighterSelectionProbe* p,float frame,float delta){
    if(!p)return false;MeleeActionResult r=melee_action_step(p->action,frame,delta,4096,event,p);
    if(p->appearance)Fighter_UpdateModelScale(&p->appearance_object);
    return r==MELEE_ACTION_WAIT||r==MELEE_ACTION_DONE;
}
void fighter_selection_probe_free(FighterSelectionProbe* p){
    if(!p)return;size_t hidden=0;for(size_t i=0;i<p->count;i++)hidden+=!!(p->objects[i]->flags&DOBJ_HIDDEN);
    printf("Original scene selection: %zu/%zu drawables hidden, %zu model events; %zu non-model events not executed (mask %llx)\n",hidden,p->count,p->applied,p->skipped,(unsigned long long)p->opcodes);
    printf("Costume expressions: %zu texture controls held at their initial frame\n",p->expression_count);
    melee_visibility_free(p->visibility);melee_action_free(p->action);free(p->appearance);free(p);
}
