#include "melee_item_colors.h"
#include <melee/lb/lb_013B.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static union CmdUnion* arena;
static u32* raw;
static size_t word_count;
static unsigned calls[3], flashes;
void lbBgFlash_80021C48(int a,int b){(void)a;(void)b;++flashes;}
static int signed16(u32 v){return (int)((v&65535)^32768)-32768;}
static void callback(Fighter_GObj* g,CommandInfo* command,int op)
{
    (void)g;if(op<21||op>23)fprintf(stderr,"bad op %d pc %td raw %08x\n",op,command->u-arena,raw[command->u-arena]);assert(op>=21&&op<=23);unsigned length=op==21?5:op==22?3:1;
    union CmdUnion* c=command->u;size_t pc=c-arena;
    assert(pc<word_count&&length<=word_count-pc);u32 r=raw[pc];assert(r>>26==op);
    if(op==21){
        assert(c[0].spawn_gfx_0.boneId==((r>>18)&255));
        assert(c[0].spawn_gfx_0.useCommonBoneIDs==((r>>17)&1));
        assert(c[0].spawn_gfx_0.destroyOnStateChange==((r>>16)&1));
        assert(c[0].spawn_gfx_0.useUnkBone==((r>>15)&1));
        assert(c[0].spawn_gfx_0.unk1==(r&32767));
        assert(c[1].spawn_gfx_1.gfxID==(raw[pc+1]>>16));
        assert(c[1].spawn_gfx_1.unkFloat==(raw[pc+1]&65535));
        assert(c[2].spawn_gfx_2.offsetZ==signed16(raw[pc+2]>>16));
        assert(c[2].spawn_gfx_2.offsetY==signed16(raw[pc+2]));
        assert(c[3].spawn_gfx_3.offsetX==signed16(raw[pc+3]>>16));
        assert(c[3].spawn_gfx_3.rangeZ==(raw[pc+3]&65535));
        assert(c[4].spawn_gfx_4.rangeY==(raw[pc+4]>>16));
        assert(c[4].spawn_gfx_4.rangeX==(raw[pc+4]&65535));
    }else if(op==22){
        assert(c[0].sound_effect_0.behavior==((r>>18)&255));
        assert(c[0].sound_effect_0.unknown==(r&0x3ffff));
        assert(c[1].sound_effect_1.sfx_id==raw[pc+1]);
        assert(c[2].sound_effect_2.padding==(raw[pc+2]>>16));
        assert(c[2].sound_effect_2.volume==((raw[pc+2]>>8)&255));
        assert(c[2].sound_effect_2.panning==(raw[pc+2]&255));
    }else{
        assert(c[0].unk21.unk1==((r>>25)&1));assert(c[0].unk21.unk2==((r>>17)&255));
    }
    ++calls[op-21];command->u+=length;
}
int main(int argc,char** argv)
{
    ColorOverlay edge={0};edge.x7C_color_enable=true;
    edge.x30_color_red=256;edge.x34_color_green=-1;edge.x38_color_blue=255.9f;
    assert(!lb_80014258(NULL,&edge,NULL));
    assert(edge.x2C_hex.r==0&&edge.x2C_hex.g==255&&edge.x2C_hex.b==255);
    alarm(30);assert(argc==2);FILE* f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));
    long size=ftell(f);assert(size>0);rewind(f);u8* bytes=malloc(size);
    assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
    MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;
    assert(melee_archive_find(&a,"ftLoadCommonData",&root));
    word_count=a.data_size/4;raw=calloc(word_count,4);assert(raw);
    for(unsigned i=0;i<word_count;i++)assert(melee_archive_u32(&a,i*4,&raw[i]));
    MeleeItemColors* owners[2];union CmdUnion* arenas[2];unsigned counts[]={123,6};
    for(unsigned k=0;k<2;k++){
        u32 table,script;MeleeHostBool present;
        assert(melee_archive_pointer(&a,root+24+k*4,&table,&present)&&present);
        owners[k]=melee_fighter_colors_decode(&a,table,counts[k]);assert(owners[k]);
        struct Fighter_804D653C_t* entries=melee_item_colors_entries(owners[k]);
        assert(melee_archive_pointer(&a,table+8,&script,&present)&&present);
        arenas[k]=(union CmdUnion*)entries[1].unk-script/4;
        for(unsigned i=0;i<counts[k];i++){
            assert(entries[i].unk4==bytes[32+table+8*i+4]);
            assert(entries[i].unk5==bytes[32+table+8*i+5]);
        }
        /* Item callbacks must continue to reject fighter-only opcodes. */
        assert(!melee_item_colors_decode(&a,table,counts[k]));
        MeleeArchive truncated=a;truncated.data_size=table+8*counts[k]-1;
        assert(!melee_fighter_colors_decode(&truncated,table,counts[k]));
    }
    memset(bytes,0xa5,size);free(bytes);unsigned finished=0;
    for(unsigned k=0;k<2;k++){
        arena=arenas[k];struct Fighter_804D653C_t* entries=melee_item_colors_entries(owners[k]);
        for(unsigned i=1;i<counts[k];i++){
            ColorOverlay co={0};assert(lb_800144C8(&co,entries,i,0));
            for(unsigned frame=0;frame<600;frame++)if(lb_80014258(NULL,&co,callback)){++finished;break;}
            lb_80014498(&co);
        }
        melee_item_colors_free(owners[k]);
    }
    assert(calls[0]&&calls[1]&&calls[2]);free(raw);
    printf("Fighter colors: 127 scripts, %u completed, callbacks effect=%u sound=%u rumble=%u flash=%u; original color interpreter, payloads and owned lifetime passed\n",finished,calls[0],calls[1],calls[2],flashes);
}
