#include "melee_vertex.h"
#include "melee_joint.h"
#include <stdio.h>
#include <stdlib.h>
static MeleeArchive archive;
static uint8_t* visited;
static size_t polygons,draws,vertices;
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
        polygons++;draws+=melee_draw_count(batch);vertices+=melee_vertex_count(batch);
        melee_vertex_free(batch);at=pointer(at+4);
    }
}
static void drawable(uint32_t at)
{
    while(!seen(at,1,16)){polygon(pointer(at+12));at=pointer(at+4);}
}
int main(int argc,char** argv)
{
    if(argc!=3){fprintf(stderr,"Usage: verify-vertices COSTUME JOINT_SYMBOL\n");return 2;}
    FILE* f=fopen(argv[1],"rb");if(!f)return 1;
    if(fseek(f,0,SEEK_END)){fclose(f);return 1;}long end=ftell(f);rewind(f);
    uint8_t* bytes=end>0?malloc((size_t)end):NULL;
    if(!bytes || fread(bytes,1,(size_t)end,f)!=(size_t)end){free(bytes);fclose(f);return 1;}fclose(f);
    uint32_t root;
    if(!melee_archive_open(&archive,bytes,(size_t)end) || archive.extern_count || !melee_archive_find(&archive,argv[2],&root))fail(0);
    MeleeJointGraph* graph=melee_joint_decode(&archive,root);if(!graph)fail(root);
    visited=calloc((size_t)archive.data_size/4+1,1);if(!visited)fail(0);
    for(size_t i=0;i<melee_joint_count(graph);i++){
        const MeleeJointNode* joint=melee_joint_node(graph,i);
        if(!(joint->flags&((1<<5)|(1<<14))))drawable(joint->payload_offset);
    }
    printf("%s: %zu polygon objects, %zu draws, %zu decoded vertices\n",argv[2],polygons,draws,vertices);
    melee_joint_free(graph);free(visited);free(bytes);return 0;
}
