#include <melee/it/itCommonItems.h>
#include <melee/it/types.h>
#include <melee/it/item.h>
#include <melee/it/kinds/itfoods.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static int selected;static HSD_Joint* chosen;static unsigned transitions;
int HSD_Randi(int count){assert(count==28);return selected;}
void it_80273318(Item_GObj* g,HSD_Joint* model){assert(g);chosen=model;}
void Item_80268E5C(HSD_GObj* g,enum_t state,Item_StateChangeFlags flags){assert(g&&state==0&&flags==ITEM_ANIM_UPDATE);transitions++;}
void HSD_JObjSetMtxDirtySub(HSD_JObj* j){j->flags|=JOBJ_MTX_DIRTY;}
#undef __assert
void __assert(char* file,u32 line,char* message){fprintf(stderr,"%s:%u: %s\n",file,line,message);abort();}
int main(void){
    itFoodsNativeEntry entries[28]={0};HSD_Joint models[28]={0};itFoodsNativeAttributes attrs={28,entries};
    Article article={0};article.x4_specialAttributes=&attrs;Item item={0};item.xC4_article_data=&article;
    HSD_JObj joint={0};HSD_GObj object={0};object.user_data=&item;object.hsd_obj=&joint;
    for(unsigned i=0;i<28;i++){entries[i].joint=&models[i];entries[i].heal_amount=i+1;entries[i].offset_x=(float)i/4;entries[i].offset_y=-(float)i/2;}
    for(selected=0;selected<28;selected++){
        itFoods_Logic18_Spawned(&object);assert(chosen==&models[selected]);assert(item.xDD4_itemVar.foods.x0==selected&&item.xDD4_itemVar.foods.heal_amount==selected+1);
        for(int facing=-1;facing<=1;facing+=2){Vec3 pos={12,34,56};it_8028F9D8(&object,&pos,facing);
            assert(item.pos.x==pos.x+facing*entries[selected].offset_x&&item.pos.y==pos.y+entries[selected].offset_y&&item.pos.z==pos.z);
            assert(joint.translate.x==item.pos.x&&joint.translate.y==item.pos.y&&joint.translate.z==item.pos.z);
        }
    }
    assert(transitions==28);puts("Original food spawn and position: all 28 indices, healing, selected model and both facing directions passed");
}
