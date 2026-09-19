#include "RenderBridge.h"
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <stdio.h>
int main(int argc,char** argv)
{
    if((argc!=4 && argc!=6) || !melee_render_test_init())return 2;
    MeleeModel* model=melee_render_test_load(argv[1],argv[2],argv[3],0);
    if(!model)return 1;
    if(argc==6 && !melee_render_test_fighter(model,argv[4],argv[5],0)){melee_render_test_release(model);return 1;}
    for(unsigned frame=0;frame<60;frame++)if(!melee_model_step(model)){melee_render_test_release(model);return 1;}
    size_t vertices=0,draws=0,hidden=0;
    for(size_t i=0;i<melee_model_part_count(model);i++){
        const MeleeModelPart* p=melee_model_part(model,i);vertices+=p->vertex_count;draws+=p->draw_count;
        hidden+=p->hidden;
    }
    printf("Owned model: %zu drawables, %zu parts, %zu vertices, %zu draws; 60 native animation/skin updates passed\n",melee_model_drawable_count(model),melee_model_part_count(model),vertices,draws);
    if(argc==6)printf("%zu hidden polygon parts after pose updates\n",hidden);
    melee_render_test_release(model);
    printf("Pools: AObj=%u FObj=%u Vec=%u\n",HSD_AObjGetAllocData()->used,HSD_FObjGetAllocData()->used,HSD_VecGetAllocData()->used);
    return HSD_AObjGetAllocData()->used || HSD_FObjGetAllocData()->used || HSD_VecGetAllocData()->used ? 1:0;
}
