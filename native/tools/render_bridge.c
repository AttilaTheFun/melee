#include "RenderBridge.h"
#include "melee_animation.h"
#include "melee_visibility.h"
#include "melee_action.h"
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static MeleeFighterAnimation* animation;
static char* animation_name;
static uint8_t* file(const char* path,size_t* size)
{
    FILE* f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);rewind(f);
    uint8_t* bytes=n>0?malloc((size_t)n):NULL;
    if(!bytes || fread(bytes,1,(size_t)n,f)!=(size_t)n){free(bytes);fclose(f);return NULL;}
    fclose(f);*size=(size_t)n;return bytes;
}
MeleeHostBool melee_render_test_init(void)
{
    static _Alignas(32) uint8_t heap[32*1024*1024];
    if(!OSInitAlloc(heap,heap+sizeof(heap),1))return false;
    HSD_SetHeap(OSCreateHeap(heap,heap+sizeof(heap)));HSD_AObjInitAllocData();HSD_FObjInitAllocData();HSD_VecInitAllocData();return true;
}
MeleeModel* melee_render_test_load(const char* costume,const char* symbol,const char* motion,float frame)
{
    size_t size;uint8_t* bytes=file(costume,&size);MeleeArchive a;uint32_t root;MeleeModel* model=NULL;
    if(bytes && melee_archive_open(&a,bytes,size) && melee_archive_find(&a,symbol,&root))model=melee_model_decode(&a,root);
    free(bytes);if(!model)return NULL;
    bytes=file(motion,&size);if(!bytes || size<32)goto fail;
    uint32_t first=((uint32_t)bytes[0]<<24)|((uint32_t)bytes[1]<<16)|((uint32_t)bytes[2]<<8)|bytes[3];const char* name;
    if(first>size || !melee_archive_open(&a,bytes,first) || !melee_archive_public(&a,0,&name,&root))goto fail;
    animation_name=strdup(name);if(!animation_name)goto fail;
    animation=melee_fighter_animation_decode(&a,root);
    if(!animation || !melee_model_bind(model,animation) || !melee_model_request(model,frame) || !melee_model_step(model))goto fail;
    free(bytes);return model;
fail:free(bytes);melee_render_test_release(model);return NULL;
}
void melee_render_test_release(MeleeModel* model)
{ melee_model_free(model);melee_fighter_animation_free(animation);animation=NULL;free(animation_name);animation_name=NULL; }
typedef struct {MeleeVisibility* visibility;size_t skipped,applied;uint64_t opcodes;} PreviewEvents;
static MeleeHostBool preview_event(void* context,const MeleeActionEvent* event)
{
    PreviewEvents* p=context;
    if(event->opcode>=31 && event->opcode<=33){p->applied++;return melee_visibility_command(p->visibility,event->words[0]);}
    /* This CLI is a model diagnostic, not a game runner. Report every event
     * class whose effects are not executed instead of treating it as supported. */
    p->skipped++;p->opcodes|=UINT64_C(1)<<event->opcode;return true;
}
MeleeHostBool melee_render_test_fighter(MeleeModel* model,const char* path,const char* symbol,float frame)
{
    uint32_t kind;
    if(!strcmp(symbol,"ftDataMario"))kind=0;
    else if(!strcmp(symbol,"ftDataFox"))kind=1;
    else if(!strcmp(symbol,"ftDataPeach"))kind=9;
    else if(!strcmp(symbol,"ftDataGamewatch"))kind=24;
    else return false;
    if(!model || !animation_name || !isfinite(frame) || frame<0 || frame>3600)return false;
    size_t size,count=melee_model_drawable_count(model);uint8_t* bytes=file(path,&size);MeleeArchive a;
    uint32_t root,desc,table,script,name;MeleeHostBool present,okay=false;
    MeleeVisibility* visibility[3]={0};MeleeAction* action=NULL;
    if(!bytes || !melee_archive_open(&a,bytes,size) || !melee_archive_find(&a,symbol,&root) ||
       !melee_archive_pointer(&a,root+8,&desc,&present) || !present ||
       !melee_archive_pointer(&a,root+12,&table,&present) || !present || table>a.data_size || a.data_size-table<72 ||
       !melee_archive_pointer(&a,table+48,&name,&present) || !present || name>=a.data_size ||
       !memchr(a.bytes+32+name,0,a.data_size-name) || strcmp((const char*)a.bytes+32+name,animation_name) ||
       !melee_archive_pointer(&a,table+60,&script,&present) || !present)goto done;
    visibility[0]=melee_visibility_decode(&a,desc,0,0,count);
    visibility[1]=melee_visibility_decode(&a,desc,0,1,count);
    visibility[2]=melee_visibility_decode(&a,desc,0,3,count);
    action=melee_action_decode(&a,script);
    if(!visibility[0] || !visibility[1] || !visibility[2] || !action || !melee_visibility_init_fighter(visibility[0],kind))goto done;
    PreviewEvents events={.visibility=visibility[0]};
    for(unsigned step=0;step<=(unsigned)ceilf(frame);step++){
        float now=fminf((float)step,frame),delta=step?now-(step-1):0;
        MeleeActionResult result=melee_action_step(action,now,delta,4096,preview_event,&events);
        if(result!=MELEE_ACTION_WAIT && result!=MELEE_ACTION_DONE)goto done;
    }
    size_t hidden=0;
    for(size_t i=0;i<count;i++){
        MeleeHostBool hide=melee_visibility_controls(visibility[1],i) || melee_visibility_controls(visibility[2],i);
        if(melee_visibility_controls(visibility[0],i))hide=melee_visibility_hidden(visibility[0],i);
        if(!melee_model_set_drawable_hidden(model,i,hide))goto done;hidden+=hide;
    }
    printf("Original fighter reset/model events: %zu/%zu drawables hidden, %zu model events; %zu non-model events not executed (opcode mask %llx)\n",hidden,count,events.applied,events.skipped,(unsigned long long)events.opcodes);
    okay=true;
done:free(bytes);melee_action_free(action);for(unsigned i=0;i<3;i++)melee_visibility_free(visibility[i]);return okay;
}
