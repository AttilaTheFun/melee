#include "melee_item_scripts.h"
#include <melee/lb/lbcommand.h>
#include <melee/it/types.h>
#include <melee/it/itanimlist.h>
#include <melee/it/iteffect.h>
#include <melee/it/itcoll.h>
#include <melee/it/item.h>
#include <sysdolphin/baselib/gobj.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void lbBgFlash_80021C48(int a,int b){(void)a;(void)b;}
static unsigned captured;static s32 effect_id,effect_bone,sound_id;static Vec3 effect_pos,effect_range;static float effect_arg;static u32 damage;static u8 pan,volume;
void it_80278800(Item_GObj* g,s32 id,s32 bone,Vec3* pos,Vec3* range,s32 flag,float arg){(void)g;assert(flag==0);captured++;effect_id=id;effect_bone=bone;effect_pos=*pos;effect_range=*range;effect_arg=arg;}
void it_80272460(HitCapsule* h,u32 value,Item_GObj* g){(void)h;(void)g;captured++;damage=value;}
void Item_8026AE84(Item* i,enum_t id,u8 p,u8 v){(void)i;captured++;sound_id=id;pan=p;volume=v;}
void Item_8026AF0C(Item* i,enum_t id,u8 p,u8 v){Item_8026AE84(i,id,p,v);}
void Item_8026AFA0(Item* i,enum_t id,u8 p,u8 v){Item_8026AE84(i,id,p,v);}
void Item_8026B034(Item* i){(void)i;captured++;}
void Item_8026B074(Item* i){(void)i;captured++;}
static unsigned length(unsigned op){return op==5||op==7?2:op==10?5:op==11?6:op==16?3:1;}
static u32 ref(const MeleeArchive* a,u32 at){u32 v;MeleeHostBool p;assert(melee_archive_pointer(a,at,&v,&p));return p?v:UINT32_MAX;}
static u32 word(const MeleeArchive* a,u32 at){u32 v;assert(melee_archive_u32(a,at,&v));return v;}
/* Pair the retail command graph with native pointers without assuming the
 * archive's unrelated gaps are retained in the native allocation. */
struct Location {u32 disk;union CmdUnion* native;};
static struct Location locations[4096];
static unsigned location_count;
static void pair_graph(const MeleeArchive* a,u32 pc,union CmdUnion* native){
    for(;;){
        for(unsigned i=0;i<location_count;i++){
            if(locations[i].disk==pc){assert(locations[i].native==native);return;}
            assert(locations[i].native!=native);
        }
        assert(location_count<4096);locations[location_count++]=(struct Location){pc,native};
        u32 op=word(a,pc)>>26;assert(op==native->unk0.opcode);
        if(op==5||op==7)pair_graph(a,ref(a,pc+4),native[1].Command_05.ptr);
        if(op==0||op==6||op==7)return;
        pc+=4*length(op);native+=length(op);
    }
}
static size_t disk_pc(union CmdUnion* native){
    for(unsigned i=0;i<location_count;i++)if(locations[i].native==native)return locations[i].disk/4;
    assert(!"Native control flow left the paired command graph");return 0;
}
int main(int argc,char** argv){
    assert(argc==2);FILE* f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
    u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
    MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;assert(melee_archive_find(&a,"itPublicData",&root));
    Item item_data={0};item_data.xC3C=1;item_data.xC40=1;HSD_GObj object={0};object.user_data=&item_data;
    u32 table=ref(&a,root+4);unsigned scripts=0,events=0,calls=0,jumps=0;
    for(unsigned item=0;item<43;item++){
        u32 article=ref(&a,table+4*item),states=ref(&a,article+12);if(states==UINT32_MAX)continue;
        assert(article>states&&(article-states)%16==0&&(article-states)/16<=16);
        for(u32 state=states;state<article;state+=16){
            u32 at=ref(&a,state+12);if(at==UINT32_MAX)continue;
            u8* sourcecopy=malloc(size);assert(sourcecopy);memcpy(sourcecopy,bytes,size);MeleeArchive input=a;input.bytes=sourcecopy;
            MeleeItemScript* owner=melee_item_script_decode(&input,at);assert(owner);scripts++;
            memset(sourcecopy,0xa5,size);free(sourcecopy);
            union CmdUnion* start=melee_item_script_root(owner);
            location_count=0;pair_graph(&a,at,start);
            CommandInfo c={0};c.u=start;
            for(unsigned frame=0;frame<120&&c.u;frame++){
                c.frame_count=frame;if(c.timer!=F32_MAX)c.timer-=frame?1:0;
                unsigned budget=1000;
                while(c.u){
                    assert(budget--);if(c.timer==F32_MAX){if(frame>=1)break;c.timer=0;}else if(c.timer>0)break;
                    size_t pc=disk_pc(c.u);assert(pc<a.data_size/4);u32 raw=word(&a,pc*4),op=raw>>26;assert(op==c.u->unk0.opcode);
                    if(op<10){
                        if(op==5||op==7){u32 target=ref(&a,(pc+1)*4);assert(disk_pc(c.u[1].Command_05.ptr)==target/4);calls+=op==5;jumps+=op==7;}
                        if(op==5)assert(c.loop_count<5);if(op==6)assert(c.loop_count>0);
                        assert(Command_Execute(&c,op));continue;
                    }
                    if(op==10){
                        assert(((raw>>16)&1023)==((c.u->Command_00.value>>16)&1023));
                        for(unsigned k=1;k<5;k++){u32 v=word(&a,(pc+k)*4);u16 halves[2];memcpy(halves,c.u+k,4);assert(halves[0]==v>>16&&halves[1]==(v&65535));}
                    }else if(op==12||op==13){assert(c.u->set_hitbox_scale.idx==((raw>>23)&7)&&c.u->set_hitbox_scale.value==(raw&0x7fffff));}
                    else if(op==16){assert(c.u->sound_effect_0.behavior==((raw>>18)&255));assert(c.u[1].sound_effect_1.sfx_id==word(&a,(pc+1)*4));assert(!memcmp(c.u+2,bytes+32+(pc+2)*4,4));}
                    else if(op==21||op==23){assert(c.u->unk33.unk0==((raw>>13)&8191)&&c.u->unk33.unk1==(raw&8191));}
                    else if(op==24){assert(c.u->unk13.unk1==((raw>>18)&255)&&c.u->unk13.unk2==(raw&0x3ffff));}
                    else if(op!=11){assert(c.u->Command_00.value==(raw&0x3ffffff));}
                    if(op==10||op==12||op==16){
                        union CmdUnion* before=c.u;unsigned previous=captured;
                        if(op==10){
                            it_80278F2C(&object,&c);assert(effect_id==(word(&a,(pc+1)*4)>>16)&&effect_bone==((raw>>16)&1023));
                            assert(effect_arg==(word(&a,(pc+1)*4)&65535));
                            float actual[6]={effect_pos.x,effect_pos.y,effect_pos.z,effect_range.x,effect_range.y,effect_range.z};
                            for(unsigned k=0;k<6;k++){u32 v=word(&a,(pc+2+k/2)*4);s16 scalar=(s16)(k%2?v:v>>16);assert(fabsf(actual[k]-0.003906f*scalar)<0.00001f);}
                        }else if(op==12){it_80279544(&object,&c);assert(damage==(raw&8191));}
                        else{it_8027978C(&object,&c);assert(sound_id==(s32)word(&a,(pc+1)*4));u32 v=word(&a,(pc+2)*4);assert(pan==((v>>8)&255)&&volume==(v&255));}
                        assert(captured==previous+1&&c.u==before+length(op));
                    }else c.u+=length(op);
                    events++;
                }
            }
            /* Looping article scripts may remain live after the bounded sample. */
            melee_item_script_free(owner);
        }
    }
    free(bytes);assert(calls&&jumps&&events);printf("Item scripts: %u state scripts, %u events, %u calls, %u jumps; native flow and decoded payloads passed\n",scripts,events,calls,jumps);
}
