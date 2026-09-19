#include "melee_joint.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char** argv)
{
    if(argc!=3) { fprintf(stderr,"Usage: verify-joints ARCHIVE JOINT_SYMBOL\n"); return 2; }
    FILE* f=fopen(argv[1],"rb"); if(!f) return 1;
    if(fseek(f,0,SEEK_END)) { fclose(f); return 1; }
    long size=ftell(f); rewind(f);
    uint8_t* bytes=size>0 ? malloc((size_t)size) : NULL;
    if(!bytes || fread(bytes,1,(size_t)size,f)!=(size_t)size) { free(bytes); fclose(f); return 1; }
    fclose(f);
    MeleeArchive archive; uint32_t root; MeleeJointGraph* graph=NULL;
    if(melee_archive_open(&archive,bytes,(size_t)size) && melee_archive_find(&archive,argv[2],&root))
        graph=melee_joint_decode(&archive,root);
    if(!graph) { fprintf(stderr,"Invalid or unsupported joint graph: %s\n",argv[2]); free(bytes); return 1; }
    size_t matrices=0,payloads=0,constraints=0,instances=0;
    for(size_t i=0;i<melee_joint_count(graph);i++) {
        const MeleeJointNode* n=melee_joint_node(graph,i);
        matrices+=n->has_inverse_bind; payloads+=n->payload_offset!=UINT32_MAX;
        constraints+=n->constraints_offset!=UINT32_MAX; instances+=!!(n->flags&(1<<12));
    }
    printf("%s: %zu joints, %zu inverse-bind matrices, %zu payload references, %zu constraint references, %zu instances\n",
        argv[2],melee_joint_count(graph),matrices,payloads,constraints,instances);
    melee_joint_free(graph); free(bytes); return 0;
}
