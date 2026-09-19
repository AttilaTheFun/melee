#include "melee_skin.h"
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include <math.h>
#include "melee_joint.h"
#include <stdio.h>
#include <stdlib.h>
static MeleeArchive archive;
static uint8_t* visited;
static size_t polygons,draws,vertices;
static MeleePose* pose;
static size_t owner;
static float low[3]={INFINITY,INFINITY,INFINITY},high[3]={-INFINITY,-INFINITY,-INFINITY};
static _Noreturn void fail(uint32_t at){fprintf(stderr,"Invalid or unsupported geometry at data offset 0x%x\n",at);exit(1);}
static uint32_t word(uint32_t at){uint32_t v;if(!melee_archive_u32(&archive,at,&v))fail(at);return v;}
static uint32_t pointer(uint32_t at)
{
    uint32_t value;MeleeHostBool present;
    if(!melee_archive_pointer(&archive,at,&value,&present))fail(at);
    return present?value:UINT32_MAX;
}
static void range(uint32_t at,size_t size){if(at>archive.data_size || size>archive.data_size-at)fail(at);}
static int seen(uint32_t at,uint8_t bit,size_t size)
{
    if(at==UINT32_MAX)return 1;
    range(at,size);if(at&3)fail(at);
    if(visited[at/4]&bit)return 1;visited[at/4]|=bit;return 0;
}
static void polygon(uint32_t at)
{
    while(!seen(at,2,24)){
        uint32_t descriptor=pointer(at+8),display=pointer(at+16);
        size_t length=(word(at+12)&65535)*32;
        if(!length){at=pointer(at+4);continue;}
        range(display,length);
        MeleeVertexAttribute attributes[26];size_t count=0;
        for(;;){
            range(descriptor,4);uint32_t attr=word(descriptor);if(attr==255)break;
            if(count==26)fail(descriptor);range(descriptor,24);
            uint32_t layout=word(descriptor+16),array=pointer(descriptor+20);
            attributes[count++]=(MeleeVertexAttribute){.attribute=attr,.mode=word(descriptor+4),.components=word(descriptor+8),.format=word(descriptor+12),.fraction=layout>>24,.stride=layout&255,
                .array=array==UINT32_MAX?NULL:archive.bytes+32+array,.array_size=array==UINT32_MAX?0:archive.data_size-array};
            descriptor+=24;
        }
        MeleeVertexBatch* batch=melee_vertex_decode(archive.bytes+32+display,length,attributes,count,0);
        if(!batch)fail(at);
        MeleeSkin* skin=melee_skin_decode(&archive,at,pose,owner);
        size_t vertex_count=melee_vertex_count(batch);
        MeleeVertex* output=vertex_count?malloc(vertex_count*sizeof(*output)):NULL;
        if(!skin || (vertex_count && !output) || !melee_skin_apply(skin,batch,output,vertex_count))fail(at);
        for(size_t i=0;i<vertex_count;i++)for(unsigned axis=0;axis<3;axis++) {
            low[axis]=fminf(low[axis],output[i].position[axis]);high[axis]=fmaxf(high[axis],output[i].position[axis]);
        }
        free(output);melee_skin_free(skin);
        polygons++;draws+=melee_draw_count(batch);vertices+=vertex_count;
        melee_vertex_free(batch);at=pointer(at+4);
    }
}
static void drawable(uint32_t at)
{
    while(!seen(at,1,16)){polygon(pointer(at+12));at=pointer(at+4);}
}
int main(int argc,char** argv)
{
    if(argc!=3 && argc!=5){fprintf(stderr,"Usage: verify-skin COSTUME JOINT_SYMBOL [MOTION_BUNDLE FRAME]\n");return 2;}
    static _Alignas(32) uint8_t heap[8*1024*1024];
    if(!OSInitAlloc(heap,heap+sizeof(heap),1))return 1;HSD_SetHeap(OSCreateHeap(heap,heap+sizeof(heap)));
    HSD_AObjInitAllocData();HSD_FObjInitAllocData();HSD_VecInitAllocData();
    FILE* f=fopen(argv[1],"rb");if(!f)return 1;
    if(fseek(f,0,SEEK_END)){fclose(f);return 1;}long end=ftell(f);rewind(f);
    uint8_t* bytes=end>0?malloc((size_t)end):NULL;
    if(!bytes || fread(bytes,1,(size_t)end,f)!=(size_t)end){free(bytes);fclose(f);return 1;}fclose(f);
    uint32_t root;
    if(!melee_archive_open(&archive,bytes,(size_t)end) || archive.extern_count || !melee_archive_find(&archive,argv[2],&root))fail(0);
    MeleeJointGraph* graph=melee_joint_decode(&archive,root);if(!graph)fail(root);
    pose=melee_pose_create(graph);if(!pose)fail(root);
    uint8_t* motion_bytes=NULL;MeleeFighterAnimation* motion=NULL;
    if(argc==5){
        FILE* mf=fopen(argv[3],"rb");if(!mf)fail(0);
        if(fseek(mf,0,SEEK_END))fail(0);long length=ftell(mf);rewind(mf);
        motion_bytes=length>0?malloc((size_t)length):NULL;
        if(!motion_bytes || fread(motion_bytes,1,(size_t)length,mf)!=(size_t)length || length<32)fail(0);fclose(mf);
        uint32_t size=((uint32_t)motion_bytes[0]<<24)|((uint32_t)motion_bytes[1]<<16)|((uint32_t)motion_bytes[2]<<8)|motion_bytes[3];
        MeleeArchive ma;const char* symbol;uint32_t mo;
        if(size>(size_t)length || !melee_archive_open(&ma,motion_bytes,size) || !melee_archive_public(&ma,0,&symbol,&mo))fail(0);
        motion=melee_fighter_animation_decode(&ma,mo);
        char* frame_end;float frame=strtof(argv[4],&frame_end);
        if(*frame_end || frame_end==argv[4] || !motion || !melee_pose_bind(pose,motion,NULL) || !melee_pose_request(pose,frame) || !melee_pose_step(pose))fail(0);
        printf("Motion: %s, frame %g\n",symbol,frame);
    }
    visited=calloc((size_t)archive.data_size/4+1,1);if(!visited)fail(0);
    for(size_t i=0;i<melee_joint_count(graph);i++){
        const MeleeJointNode* joint=melee_joint_node(graph,i);
        owner=SIZE_MAX;
        for(size_t k=0;k<melee_pose_count(pose);k++)if(melee_pose_source_offset(pose,k)==joint->offset){owner=k;break;}
        if(owner==SIZE_MAX)fail(joint->offset);
        if(!(joint->flags&((1<<5)|(1<<14))))drawable(joint->payload_offset);
    }
    printf("%s: %zu polygon objects, %zu draws, %zu decoded vertices\n",argv[2],polygons,draws,vertices);
    printf("Skinned bounds: (%g,%g,%g) to (%g,%g,%g)\n",low[0],low[1],low[2],high[0],high[1],high[2]);
    melee_pose_free(pose);melee_fighter_animation_free(motion);free(motion_bytes);
    melee_joint_free(graph);free(visited);free(bytes);
    printf("Pools: AObj=%u FObj=%u Vec=%u\n",HSD_AObjGetAllocData()->used,HSD_FObjGetAllocData()->used,HSD_VecGetAllocData()->used);
    return HSD_AObjGetAllocData()->used || HSD_FObjGetAllocData()->used || HSD_VecGetAllocData()->used ? 1:0;
}
