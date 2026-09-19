#include "melee_item_colors.h"
#include <stdlib.h>
#include <string.h>
struct MeleeItemColors {struct Fighter_804D653C_t* entries;union ColorOverlay_x8_t* words;u8* starts;};
void melee_item_colors_free(MeleeItemColors* b){if(b){free(b->entries);free(b->words);free(b->starts);free(b);}}
struct Fighter_804D653C_t* melee_item_colors_entries(MeleeItemColors* b){return b?b->entries:NULL;}
static unsigned length(unsigned op,int fighter){
    if(fighter){if(op==21)return 5;if(op==22)return 3;if(op==23)return 1;}
    if(op==5||op==7||op==13||op==14||op==15||op==18||op==19)return 2;
    if(op==0||op==3||op==4||op==6||(op>=10&&op<=20))return 1;
    return 0;
}
static int signed_bits(u32 v,unsigned bits){u32 sign=1u<<(bits-1);return (int)((v&((1u<<bits)-1))^sign)-(int)sign;}
static MeleeItemColors* decode(const MeleeArchive* a,uint32_t table,unsigned count,int fighter,unsigned stride){
    if(!a||!count||count>256||(uint64_t)table+stride*count>a->data_size)return NULL;
    size_t words=a->data_size/4,n=0;size_t* pending=malloc(words*sizeof(*pending));
    MeleeItemColors* b=calloc(1,sizeof(*b));if(!b){free(pending);return NULL;}
    b->entries=calloc(count,sizeof(*b->entries));b->words=calloc(words,sizeof(*b->words));b->starts=calloc(words,1);
    if(!pending||!b->entries||!b->words||!b->starts)goto fail;
    for(unsigned i=0;i<count;i++){
        u32 root;MeleeHostBool present;if(!melee_archive_pointer(a,table+stride*i,&root,&present))goto fail;
        if(stride==8){const u8* meta=a->bytes+32+table+stride*i+4;b->entries[i].unk4=meta[0];b->entries[i].unk5=meta[1];}
        if(present){if((root&3)||root/4>=words||n>=words)goto fail;b->entries[i].unk=b->words+root/4;pending[n++]=root/4;}
    }
    while(n){size_t pc=pending[--n];for(;;){
        if(pc>=words||b->starts[pc]==2)goto fail;if(b->starts[pc])break;
        u32 raw;if(!melee_archive_u32(a,pc*4,&raw))goto fail;unsigned op=raw>>26,len=length(op,fighter);if(!len||len>words-pc)goto fail;
        for(unsigned i=0;i<len;i++){if(b->starts[pc+i])goto fail;b->starts[pc+i]=i?2:1;}
        union ColorOverlay_x8_t* d=b->words+pc;d->unk.unk=op;d->unk.timer=raw&0x3ffffff;
        if((op==3||op==15||op==19)&&!d->unk.timer)goto fail;
        if(op==13){d->light_rot2.x0_0=(raw>>31)&1;d->light_rot2.x0_1=(raw>>30)&1;d->light_rot2.x0_2=(raw>>29)&1;d->light_rot2.x0_3=(raw>>28)&1;d->light_rot2.x0_4=(raw>>27)&1;d->light_rot2.x0_5=(raw>>26)&1;d->light_rot2.light_enable=(raw>>25)&1;d->light_rot2.x0_7=(raw>>24)&1;d->light_rot2.x=signed_bits(raw>>12,12);d->light_rot2.yz=signed_bits(raw,12);d->unk.unk=op;}
        if(op==16){d->light_rot1.unk=op;d->light_rot1.x=signed_bits(raw>>13,13);d->light_rot1.yz=signed_bits(raw,13);}
        if(op>=21){
            union CmdUnion* c=(union CmdUnion*)d;
            u32 payload[4]={0};for(unsigned i=1;i<len;i++)if(!melee_archive_u32(a,(pc+i)*4,&payload[i-1]))goto fail;
            if(op==21){
                c[0].spawn_gfx_0=(struct spawn_gfx_0){op,(raw>>18)&255,(raw>>17)&1,(raw>>16)&1,(raw>>15)&1,raw&32767};
                c[1].spawn_gfx_1=(struct spawn_gfx_1){payload[0]>>16,payload[0]&65535};
                c[2].spawn_gfx_2=(struct spawn_gfx_2){signed_bits(payload[1]>>16,16),signed_bits(payload[1],16)};
                c[3].spawn_gfx_3=(struct spawn_gfx_3){signed_bits(payload[2]>>16,16),payload[2]&65535};
                c[4].spawn_gfx_4=(struct spawn_gfx_4){payload[3]>>16,payload[3]&65535};
            }else if(op==22){
                unsigned behavior=(raw>>18)&255;if(behavior>6&&(behavior<10||behavior>15))goto fail;
                c[0].sound_effect_0=(struct sound_effect_0){op,behavior,raw&0x3ffff};
                c[1].sound_effect_1.sfx_id=payload[0];
                c[2].sound_effect_2=(struct sound_effect_2){payload[1]>>16,(payload[1]>>8)&255,payload[1]&255};
            }else c[0].unk21=(struct unk21){op,(raw>>25)&1,(raw>>17)&255};
        }
        if(op==5||op==7){
            u32 target;MeleeHostBool present;if(!melee_archive_pointer(a,(pc+1)*4,&target,&present)||!present||(target&3)||target/4>=words)goto fail;
            d[1].branch=(union CmdUnion*)(b->words+target/4);
            if(op==7){pc=target/4;continue;}if(n>=words)goto fail;pending[n++]=target/4;
        }else if(len==2)memcpy(&d[1].light_color,a->bytes+32+(pc+1)*4,4);
        if(op==0||op==6||op==10)break;pc+=len;
    }}
    free(pending);return b;
fail:free(pending);melee_item_colors_free(b);return NULL;
}

MeleeItemColors* melee_item_colors_decode(const MeleeArchive* a,uint32_t table,unsigned count){return decode(a,table,count,0,8);}
MeleeItemColors* melee_fighter_colors_decode(const MeleeArchive* a,uint32_t table,unsigned count){return decode(a,table,count,1,8);}

MeleeItemColors* melee_stage_colors_decode(const MeleeArchive* a,uint32_t table,unsigned count){return decode(a,table,count,0,4);}
