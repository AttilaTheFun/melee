#include "melee_item_foods.h"
#include "melee_scene.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
struct MeleeItemFoods {itFoodsNativeAttributes attrs;MeleeScene** models;};
void melee_item_foods_free(MeleeItemFoods* f){
    if(!f)return;
    if(f->models)for(s32 i=0;i<f->attrs.count;i++)melee_scene_free(f->models[i]);
    free(f->models);free(f->attrs.entries);free(f);
}
itFoodsNativeAttributes* melee_item_foods_attributes(MeleeItemFoods* f){return f?&f->attrs:NULL;}
MeleeItemFoods* melee_item_foods_decode(const MeleeArchive* a,uint32_t at){
    u32 count;if(!a||!melee_archive_u32(a,at,&count)||!count||count>256||
        (uint64_t)at+4+16*count>a->data_size)return NULL;
    MeleeItemFoods* f=calloc(1,sizeof(*f));if(!f)return NULL;f->attrs.count=count;
    f->attrs.entries=calloc(count,sizeof(*f->attrs.entries));f->models=calloc(count,sizeof(*f->models));
    if(!f->attrs.entries||!f->models)goto fail;
    for(u32 i=0;i<count;i++){
        u32 row=at+4+16*i,joint,heal;MeleeHostBool present;
        itFoodsNativeEntry* entry=&f->attrs.entries[i];
        if(!melee_archive_pointer(a,row,&joint,&present)||!present||!melee_archive_u32(a,row+4,&heal)||heal>INT32_MAX||
            !melee_archive_f32(a,row+8,&entry->offset_x)||!melee_archive_f32(a,row+12,&entry->offset_y)||
            !isfinite(entry->offset_x)||!isfinite(entry->offset_y))goto fail;
        entry->heal_amount=heal;f->models[i]=melee_scene_decode(a,joint);if(!f->models[i])goto fail;
        entry->joint=melee_scene_joint_descriptor(f->models[i]);melee_scene_release_objects(f->models[i]);
    }
    return f;
fail:melee_item_foods_free(f);return NULL;
}
