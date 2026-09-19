#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "melee_ax_voice.h"
#undef __assert
#include "../../src/sysdolphin/baselib/synth.c"
static struct { uintptr_t src,dest;size_t size;int type;HSD_DevComCallback cb;void* args; } request;
int HSD_DevComRequest(int file,uintptr_t src,uintptr_t dest,size_t size,int type,int pri,HSD_DevComCallback cb,void* args)
{ request.src=src;request.dest=dest;request.size=size;request.type=type;request.cb=cb;request.args=args;return 1; }
static void word(u8* p,u32 n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
int main(int argc,char** argv)
{
    u8 header[128]={0};memcpy(header," HALPST",7);word(header+8,32000);word(header+12,2);
    for(unsigned i=0;i<2;i++){header[16+56*i+1]=1;word(header+16+56*i+4,2);word(header+16+56*i+8,65535);word(header+16+56*i+12,2);}
    AXPBADDR addresses[2];AXPBADPCM adpcm[2];u32 rate,channels;
    assert(native_hps_header(header,&rate,&channels,addresses,adpcm));assert(rate==32000&&channels==2);
    word(header+12,3);assert(!native_hps_header(header,&rate,&channels,addresses,adpcm));word(header+12,2);
    word(header+8,0);assert(!native_hps_header(header,&rate,&channels,addresses,adpcm));word(header+8,32000);
    header[0]=0;assert(!native_hps_header(header,&rate,&channels,addresses,adpcm));header[0]=' ';
    melee_ax_voice_pool_init();
    struct HSD_SynthSFXNode* node=&hsd_SynthSFXNodes[1];
    node->x0=HSD_Synth_804D7760=65;node->flags=8;
    node->voice[0]=AXAcquireVoice(29,NULL,0);assert(node->voice[0]);
    node->unk28=node->user_vol[0].x8_float=node->x18[0]=node->x18[1]=1;
    HSD_Synth_804C28E0_1784[0].x1784=1;HSD_Synth_804D7780=0x10000;
    HSD_SynthPStreamHeaderCallback(0,0,header,false);
    assert(node->voice_count==2&&node->voice[1]&&node->voice[1]!=node->voice[0]);
    assert(node->x14==1&&request.src==128&&request.type==0x21&&request.size==32&&!(request.dest&31));
    u8* block=(u8*)request.dest;memset(block,0,32);word(block,65536);word(block+4,65535);word(block+8,0x100a0);
    block[12]=0x00;block[13]=0x23;block[14]=0xab;block[15]=0xcd;
    request.cb(0,0,NULL,false);
    assert(request.src==160&&request.type==0x23&&request.size==65536);
    unsigned index=HSD_Synth_804D7768;
    assert(lbl_804C4540[index].x8==0x100a0);
    AXPBADPCMLOOP* loop=(AXPBADPCMLOOP*)lbl_804C4540[index].pad;
    assert(loop->loop_pred_scale==0x23&&loop->loop_yn1==0xabcd);
    request.cb(0,0,NULL,false);
    for(unsigned i=0;i<2;i++){
        assert(node->voice[i]->pb.state==1&&node->voice[i]->pb.src.ratioHi==1&&node->voice[i]->pb.src.ratioLo==0);
        u32 base=(HSD_Synth_804D7780+(index<<16))*2;
        assert(((u32)node->voice[i]->pb.addr.currentAddressHi<<16|node->voice[i]->pb.addr.currentAddressLo)==base+i*65536+2);
    }
    assert(!(node->flags&8));
    HSD_Synth_8038ADD0();
    assert(request.src==0x100a0&&request.type==0x21&&request.size==32);
    assert(HSD_Synth_804D7768==(index+1)%3);
    block=(u8*)request.dest;memset(block,0,32);word(block,65536);word(block+4,65535);word(block+8,UINT32_MAX);
    request.cb(0,(HSD_DevComArg)request.args,NULL,false);
    assert(request.src==0x100c0&&request.type==0x23&&request.size==65536);

    /* Finished channels point outside the three stream buffers while node
     * reclamation waits for its callback batch. Never index using that address. */
    node->voice[0]->pb.state=0;
    node->voice[0]->pb.addr.currentAddressHi=0;
    node->voice[0]->pb.addr.currentAddressLo=0x8000;
    HSD_Synth_8038ADD0();
    assert(HSD_Synth_804D7774==index); /* The second channel is still running. */
    node->voice[1]->pb.state=0;
    node->voice[1]->pb.addr.currentAddressHi=0;
    node->voice[1]->pb.addr.currentAddressLo=0x8000;
    HSD_Synth_8038ADD0();assert(HSD_Synth_804D7774==index);
    node->voice_count=1;HSD_Synth_8038ADD0();assert(HSD_Synth_804D7774==index);
    u8 malformed[32]={0};word(malformed,65537);word(malformed+4,65535);word(malformed+8,UINT32_MAX);
    memcpy(&lbl_804C4540[0],malformed,32);assert(!native_hps_block(0,2));
    word(malformed,65536);word(malformed+4,131071);
    memcpy(&lbl_804C4540[0],malformed,32);assert(native_hps_block(0,1));
    memcpy(&lbl_804C4540[0],malformed,32);assert(!native_hps_block(0,2));
    if(argc==2){
        FILE* f=fopen(argv[1],"rb");assert(f);assert(fread(header,1,128,f)==128);
        assert(native_hps_header(header,&rate,&channels,addresses,adpcm)&&channels==2&&rate==32000);
        unsigned count=0;u32 offset=128;
        while(offset!=UINT32_MAX){
            assert(!fseek(f,offset,SEEK_SET));assert(fread(&lbl_804C4540[0],1,32,f)==32);
            assert(native_hps_block(0,2));u32 next=(u32)lbl_804C4540[0].x8;
            /* The opening track is finite. Other HPS tracks may loop. */
            assert(next==UINT32_MAX||next>offset);offset=next;assert(++count<10000);
        }
        fclose(f);assert(count>1);printf("Retail opening HPS: %u blocks decoded\n",count);
    }
    puts("HPS startup: stereo voices, endian conversion, pitch, block transfers and loop context passed");return 0;
}
