#include "melee_pose.h"
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static uint32_t word(const uint8_t* p) { return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }
static uint8_t* read_file(const char* path,size_t* size)
{
    FILE* f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long end=ftell(f);rewind(f);
    uint8_t* bytes=end>0?malloc((size_t)end):NULL;
    if(!bytes || fread(bytes,1,(size_t)end,f)!=(size_t)end){free(bytes);fclose(f);return NULL;}
    fclose(f);*size=(size_t)end;return bytes;
}
int main(int argc,char** argv)
{
    if(argc!=4){fprintf(stderr,"Usage: verify-poses COSTUME JOINT_SYMBOL MOTION_BUNDLE\n");return 2;}
    static _Alignas(32) uint8_t arena[8*1024*1024];
    if(!OSInitAlloc(arena,arena+sizeof(arena),1))return 1;
    HSD_SetHeap(OSCreateHeap(arena,arena+sizeof(arena)));HSD_AObjInitAllocData();HSD_FObjInitAllocData();HSD_VecInitAllocData();
    size_t costume_size,bundle_size;uint8_t* costume=read_file(argv[1],&costume_size);uint8_t* bundle=read_file(argv[3],&bundle_size);
    if(!costume || !bundle){free(costume);free(bundle);return 1;}
    MeleeArchive archive;uint32_t root;MeleeJointGraph* graph=NULL;
    if(melee_archive_open(&archive,costume,costume_size) && melee_archive_find(&archive,argv[2],&root))graph=melee_joint_decode(&archive,root);
    if(!graph){free(costume);free(bundle);return 1;}
    size_t cursor=0,accepted=0,rejected=0,frames=0;int invalid=0;
    while(cursor<bundle_size){
        if(bundle_size-cursor<32){invalid=1;break;}
        uint32_t size=word(bundle+cursor);const char* name;
        if(size>bundle_size-cursor || !melee_archive_open(&archive,bundle+cursor,size) || !melee_archive_public(&archive,0,&name,&root)){invalid=1;break;}
        MeleeFighterAnimation* animation=melee_fighter_animation_decode(&archive,root);
        MeleePose* pose=melee_pose_create(graph);
        if(!animation || !pose || !melee_pose_bind(pose,animation,NULL)){
            printf("Unsupported pose binding: %s (motion=%zu, skeleton=%zu)\n",name,melee_fighter_animation_joint_count(animation),melee_joint_count(graph));++rejected;
        }else{
            FigaTree* tree=melee_fighter_animation_tree(animation);
            if(tree->frames>100000 || !melee_pose_request(pose,0)){invalid=1;}
            else for(unsigned frame=0;frame<=(unsigned)ceilf(tree->frames)+1;frame++){
                if(!melee_pose_step(pose)){invalid=1;break;}++frames;
            }
            ++accepted;
        }
        melee_pose_free(pose);melee_fighter_animation_free(animation);
        if(size==bundle_size-cursor)break;
        cursor+=((size_t)size+31)&~(size_t)31;
    }
    printf("Poses: %zu accepted, %zu unsupported, %zu frames; invalid=%d; pools AObj=%u FObj=%u Vec=%u\n",accepted,rejected,frames,invalid,HSD_AObjGetAllocData()->used,HSD_FObjGetAllocData()->used,HSD_VecGetAllocData()->used);
    melee_joint_free(graph);free(costume);free(bundle);
    return invalid || rejected || HSD_AObjGetAllocData()->used || HSD_FObjGetAllocData()->used || HSD_VecGetAllocData()->used ? 1:0;
}
