#include "melee_item_scripts.h"
#include <stdlib.h>
#include <string.h>
struct MeleeItemScripts {union CmdUnion* scripts[257];MeleeItemScript* owners[257];unsigned count;};
static int ref(const MeleeArchive* a,u32 at,u32* out,MeleeHostBool* present){return melee_archive_pointer(a,at,out,present);}
static int sign16(u32 v){v&=65535;return (int)(v^32768)-32768;}
static unsigned command_length(u32 op){
    if(op>25)return 0;
    return op==5||op==7?2:op==10?5:op==11?6:op==16?3:1;
}
static int encode(const MeleeArchive* a,u32 at,union CmdUnion* d,u32 op,unsigned n){
    u32 w[6];for(unsigned i=0;i<n;i++)if(!melee_archive_u32(a,at+4*i,w+i))return 0;
    d[0].Command_00=(struct Command_00){op,w[0]&0x03ffffff};
    if(op==9||op==16||op==24)d[0].unk0=(struct unk0){op,(w[0]>>18)&255,w[0]&0x3ffff};
    if(op==10){
        for(unsigned i=1;i<5;i++){
            u16 halves[2]={(u16)(w[i]>>16),(u16)w[i]};memcpy(&d[i],halves,4);
        }
    }
    if(op==11){
        if(((w[0]>>23)&7)>=4)return 0;
        d[0].it_create_hitbox_0=(struct it_create_hitbox_0){op,(w[0]>>23)&7,(w[0]>>20)&7,(w[0]>>13)&127,w[0]&8191};
        d[1].create_hitbox_1=(struct spawn_hitbox_1){w[1]>>16,sign16(w[1])};
        d[2].create_hitbox_2=(struct spawn_hitbox_2){sign16(w[2]>>16),sign16(w[2])};
        d[3].create_hitbox_3=(struct spawn_hitbox_3){w[3]>>23,(w[3]>>14)&511,(w[3]>>5)&511,(w[3]>>4)&1,(w[3]>>3)&1,(w[3]>>2)&1,(w[3]>>1)&1,w[3]&1};
        d[4].it_create_hitbox_4=(struct it_create_hitbox_4){w[4]>>23,(w[4]>>18)&31,(w[4]>>17)&1,(int)(((w[4]>>9)&255)^128)-128,(w[4]>>6)&7,(w[4]>>2)&15,(w[4]>>1)&1,w[4]&1};
        /* it_802790C0 reads this payload through u8*, not native bitfields. */
        memcpy(&d[5],a->bytes+32+at+20,4);
    }
    if(op==12||op==13){
        if(((w[0]>>23)&7)>=4)return 0;
        d[0].set_hitbox_scale=(struct set_hitbox_scale){op,(w[0]>>23)&7,w[0]&0x7fffff};
    }
    if(op==14&&(w[0]&0x03ffffff)>=4)return 0;
    if(op==16){
        u32 behavior=(w[0]>>18)&255;if(behavior>2&&behavior!=10&&behavior!=11)return 0;
        d[1].sound_effect_1.sfx_id=w[1];
        memcpy(&d[2],a->bytes+32+at+8,4);
    }
    if(op==21||op==23)d[0].unk33=(struct unk33){op,(w[0]>>13)&8191,w[0]&8191};
    return 1;
}
/* Convert only reachable command words. An article archive contains many
 * unrelated scripts/models; retaining an archive-sized native arena for every
 * state consumed gigabytes on devices. Sorted reachable commands preserve
 * fallthrough, while explicit branches use the disk-to-native index map. */
struct MeleeItemScript {union CmdUnion* words;size_t count,root;};
void melee_item_script_free(MeleeItemScript* s){if(s){free(s->words);free(s);}}
union CmdUnion* melee_item_script_root(MeleeItemScript* s){return s?s->words+s->root:NULL;}
MeleeItemScript* melee_item_script_decode(const MeleeArchive* a,u32 at){
    if(!a||(at&3)||at>=a->data_size)return NULL;
    MeleeItemScript* s=calloc(1,sizeof(*s));if(!s)return NULL;
    size_t disk_count=a->data_size/4, count=0, used=0;
    u8* starts=calloc(disk_count,1);
    size_t* pending=malloc(disk_count*sizeof(*pending));
    size_t* indices=NULL;
    if(!starts||!pending)goto fail;
    pending[count++]=at/4;
    while(count){
        size_t pc=pending[--count];
        for(;;){
            if(pc>=disk_count||starts[pc]==2)goto fail;
            if(starts[pc]==1)break;
            u32 w;if(!melee_archive_u32(a,pc*4,&w))goto fail;
            u32 op=w>>26;unsigned n=command_length(op);
            if(!n||n>disk_count-pc)goto fail;
            for(unsigned i=0;i<n;i++){if(starts[pc+i])goto fail;starts[pc+i]=i?2:1;}
            used+=n;
            if(op==5||op==7){
                u32 target;MeleeHostBool present;
                if(!ref(a,(pc+1)*4,&target,&present)||!present||(target&3)||target/4>=disk_count)goto fail;
                if(op==7){pc=target/4;continue;}
                if(count>=disk_count)goto fail;pending[count++]=target/4;
            }
            if(op==0||op==6)break;pc+=n;
        }
    }
    free(pending);pending=NULL;
    indices=malloc(disk_count*sizeof(*indices));
    s->words=calloc(used,sizeof(*s->words));s->count=used;
    if(!indices||!s->words)goto fail;
    size_t out=0;
    for(size_t pc=0;pc<disk_count;pc++)if(starts[pc])indices[pc]=out++;
    s->root=indices[at/4];
    for(size_t pc=0;pc<disk_count;pc++)if(starts[pc]==1){
        u32 w;if(!melee_archive_u32(a,pc*4,&w))goto fail;
        u32 op=w>>26;unsigned n=command_length(op);
        union CmdUnion* command=s->words+indices[pc];
        if(!encode(a,pc*4,command,op,n))goto fail;
        if(op==5||op==7){
            u32 target;MeleeHostBool present;
            if(!ref(a,(pc+1)*4,&target,&present)||!present||starts[target/4]!=1)goto fail;
            command[1].Command_05.ptr=s->words+indices[target/4];
        }
    }
    free(indices);free(starts);return s;
fail:free(indices);free(pending);free(starts);melee_item_script_free(s);return NULL;
}
void melee_item_scripts_free(MeleeItemScripts* s){if(s){for(unsigned i=1;i<=s->count;i++)melee_item_script_free(s->owners[i]);free(s);}}
unsigned melee_item_scripts_count(const MeleeItemScripts* s){return s?s->count:0;}
union CmdUnion** melee_item_scripts_table(MeleeItemScripts* s){return s?s->scripts:NULL;}
MeleeItemScripts* melee_item_scripts_decode(const MeleeArchive* a){
    u32 root,target;MeleeHostBool present;
    if(!a||!melee_archive_find(a,"ALDYakuAll",&root)||!ref(a,root,&target,&present)||present)return NULL;
    MeleeItemScripts* s=calloc(1,sizeof(*s));if(!s)return NULL;
    for(unsigned i=1;i<=256;i++){
        if((uint64_t)root+4*i+4>a->data_size||!ref(a,root+4*i,&target,&present))goto fail;
        if(!present)return s;if(i==256)goto fail;
        s->count=i;s->owners[i]=melee_item_script_decode(a,target);if(!s->owners[i])goto fail;s->scripts[i]=melee_item_script_root(s->owners[i]);
    }
fail:melee_item_scripts_free(s);return NULL;
}
