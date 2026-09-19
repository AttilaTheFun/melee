#include "melee_action.h"
#include <melee/lb/types.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern u32 ftAction_CommandWordCount(u32);
static u32 ref(const MeleeArchive* a,u32 at){u32 v;MeleeHostBool present;assert(melee_archive_pointer(a,at,&v,&present)&&present);return v;}
int main(int argc,char**argv){
 assert(argc==3);unsigned motions=(unsigned)strtoul(argv[2],NULL,10);assert(motions&&motions<=1024);
 FILE* f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);assert(n>0);u8* b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
 MeleeArchive a;assert(melee_archive_open(&a,b,n));u32 root;const char* name;assert(melee_archive_public(&a,0,&name,&root)&&!strncmp(name,"ftData",6));
 u32 table=ref(&a,root+12),roots[1024];size_t count=0;
 for(unsigned i=0;i<motions;i++){u32 script;MeleeHostBool present;assert(melee_archive_pointer(&a,table+24*i+12,&script,&present));if(present)roots[count++]=script;}
 MeleeAction* o=melee_fighter_actions_decode(&a,roots,count);assert(o);
 size_t slots=a.data_size/4,top=count,nodes=0,branches=0,hitboxes=0;u32* pending=malloc((slots+count)*sizeof(*pending));u8* seen=calloc(slots,1);assert(pending&&seen);memcpy(pending,roots,count*sizeof(*roots));
 while(top){u32 pc=pending[--top];while(pc<a.data_size&&!seen[pc/4]){
  seen[pc/4]=1;nodes++;u32 word;assert(melee_archive_u32(&a,pc,&word));u32 op=word>>26,length=ftAction_CommandWordCount(op);assert(length);
  union CmdUnion* command=melee_fighter_actions_script(o,pc);assert(command&&command->Command_00.code==op);
  for(u32 i=1;i<length;i++)assert(!melee_fighter_actions_script(o,pc+4*i));
  if(op==11){u32 next;assert(melee_archive_u32(&a,pc+20,&next));assert(command->create_hitbox_0.next_command_grabbed_only==((next>>19)&1));hitboxes++;}
  if(op==5||op==7){u32 target=ref(&a,pc+4);assert(command[1].Command_05.ptr==melee_fighter_actions_script(o,target));branches++;assert(top<slots+count);pending[top++]=target;if(op==7)break;}
  if(op==0||op==6)break;pc+=4*length;
 }}
 /* Destroy every borrowed byte, then inspect every reachable owned slot and
  * relocated pointer again. */
 memset(b,0xa5,n);free(b);
 for(size_t i=0;i<slots;i++)if(seen[i]){union CmdUnion* c=melee_fighter_actions_script(o,(u32)i*4);assert(c&&c->Command_00.code<=58);if(c->Command_00.code==5||c->Command_00.code==7)assert(c[1].Command_05.ptr->Command_00.code<=58);}
 assert(!melee_fighter_actions_script(o,1)&&!melee_fighter_actions_script(o,a.data_size));
 melee_action_free(o);free(pending);free(seen);
 printf("%s: %zu script roots, %zu native commands, %zu branches, %zu hitboxes; shared arena and source-free pointers passed\n",argv[1],count,nodes,branches,hitboxes);
}
