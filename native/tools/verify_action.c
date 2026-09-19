#include "melee_action.h"
#include <stdio.h>
#include <stdlib.h>
static size_t events,model_events;
static MeleeHostBool observe(void* unused,const MeleeActionEvent* e)
{
    (void)unused;events++;
    if(e->opcode>=31 && e->opcode<=33)model_events++;
    return true; /* Verification observer only: no game-side effects executed. */
}
int main(int argc,char** argv)
{
    if(argc!=4)return 2;
    char* end;unsigned long index=strtoul(argv[3],&end,10);if(*end || index>UINT32_MAX/24)return 2;
    FILE* f=fopen(argv[1],"rb");if(!f)return 1;
    if(fseek(f,0,SEEK_END)){fclose(f);return 1;}long size=ftell(f);rewind(f);
    uint8_t* bytes=size>0?malloc((size_t)size):NULL;
    if(!bytes || fread(bytes,1,(size_t)size,f)!=(size_t)size){free(bytes);fclose(f);return 1;}fclose(f);
    MeleeArchive archive;uint32_t root,table,script;MeleeHostBool present;
    if(!melee_archive_open(&archive,bytes,(size_t)size) || !melee_archive_find(&archive,argv[2],&root) ||
       !melee_archive_pointer(&archive,root+12,&table,&present) || !present || table>archive.data_size ||
       index> (archive.data_size-table)/24 || archive.data_size-table-index*24<24 ||
       !melee_archive_pointer(&archive,table+(uint32_t)index*24+12,&script,&present) || !present){free(bytes);return 1;}
    MeleeAction* action=melee_action_decode(&archive,script);free(bytes);if(!action)return 1;
    MeleeActionResult result=MELEE_ACTION_WAIT;
    for(unsigned frame=0;frame<180;frame++){
        result=melee_action_step(action,(float)frame,frame?1:0,4096,observe,NULL);
        if(result!=MELEE_ACTION_WAIT && result!=MELEE_ACTION_DONE){fprintf(stderr,"Script fault %d at frame %u\n",result,frame);melee_action_free(action);return 1;}
    }
    melee_action_free(action);
    printf("%s animation %lu: 180 frame advances, %zu observed events (%zu model events), status %s; game effects NOT executed\n",argv[2],index,events,model_events,result==MELEE_ACTION_DONE?"done":"waiting");
}
