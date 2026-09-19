#include "melee_particle_bank.h"
#include "melee_texture.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
/* Some retail banks contain unused, out-of-range palette entries. Keep a
 * non-dereferenceable marker and reject it at the rendering boundary. */
static u8 unresolved_palette;
int melee_particle_bank_palette_resolved(const void* p){return p!=&unresolved_palette;}
struct MeleeParticleBank {unsigned commands_count,textures_count;HSD_PSCmdList** commands;HSD_PSTexGroup** textures;u8* bytes;};
static uint32_t u32be(const u8* p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static unsigned u16be(const u8* p){return (unsigned)p[0]<<8|p[1];}
static int span(size_t size,uint32_t at,size_t n){return at<=size&&n<=size-at;}
void melee_particle_bank_free(MeleeParticleBank* b){
    if(!b)return;
    if(b->commands)for(unsigned i=0;i<b->commands_count;i++)free(b->commands[i]);
    if(b->textures)for(unsigned i=0;i<b->textures_count;i++)free(b->textures[i]);
    free(b->commands);free(b->textures);free(b->bytes);free(b);
}
unsigned melee_particle_bank_command_count(const MeleeParticleBank* b){return b?b->commands_count:0;}
unsigned melee_particle_bank_texture_count(const MeleeParticleBank* b){return b?b->textures_count:0;}
HSD_PSCmdList** melee_particle_bank_commands(MeleeParticleBank* b){return b?b->commands:NULL;}
HSD_PSTexGroup** melee_particle_bank_textures(MeleeParticleBank* b){return b?b->textures:NULL;}
MeleeParticleBank* melee_particle_bank_decode(const MeleeArchive* a,uint32_t cmd,size_t cn,uint32_t tex,size_t tn){
    if(!a||!span(a->data_size,cmd,cn)||!span(a->data_size,tex,tn)||cn<12||tn<4)return NULL;
    MeleeParticleBank* b=calloc(1,sizeof(*b));if(!b)return NULL;
    b->bytes=malloc(a->data_size);if(!b->bytes)goto fail;memcpy(b->bytes,a->bytes+32,a->data_size);
    const u8* c=b->bytes+cmd;u8* t=b->bytes+tex;
    unsigned version=u16be(c),start=0,entries,header;
    if(version==0){entries=u32be(c+4);header=8;}
    else if(version>=0x40&&version<=0x43){start=u32be(c+4);entries=u32be(c+8);header=12;}
    else goto fail;
    if(start>65536||entries>65536-start||!span(cn,header,entries*4u))goto fail;
    b->commands_count=start+entries;b->textures_count=u32be(t);
    if(!b->commands_count||b->textures_count>65536||!span(tn,4,b->textures_count*4u))goto fail;
    b->commands=calloc(b->commands_count,sizeof(*b->commands));
    b->textures=calloc(b->textures_count?b->textures_count:1,sizeof(*b->textures));if(!b->commands||!b->textures)goto fail;
    for(unsigned i=0;i<entries;i++){
        uint32_t at=u32be(c+header+4*i);if(!at)continue;
        size_t end=cn;
        if(at<header+4*entries||!span(cn,at,61))goto fail;
        for(unsigned j=0;j<entries;j++){uint32_t next=u32be(c+header+4*j);if(next>at&&next<end)end=next;}
        if(end-at<61)goto fail;
        HSD_PSCmdList* d=calloc(1,end-at<sizeof(*d)?sizeof(*d):end-at);if(!d)goto fail;b->commands[start+i]=d;
        d->type=u16be(c+at);d->texGroup=u16be(c+at+2);d->genLife=u16be(c+at+4);d->life=u16be(c+at+6);
        d->kind=(u32be(c+at+8)&0xf1ffffff)|0x08000000;
        for(unsigned k=0;k<12;k++){uint32_t bits=u32be(c+at+12+4*k);float f;memcpy(&f,&bits,4);if(!isfinite(f))goto fail;memcpy((u8*)d+12+4*k,&f,4);}
        memcpy(d->cmdList,c+at+60,end-at-60);
    }
    for(unsigned i=0;i<b->textures_count;i++){
        uint32_t at=u32be(t+4+4*i);if(!at)continue;
        if(at<4+4*b->textures_count||!span(tn,at,24))goto fail;
        unsigned n=u32be(t+at),fmt=u32be(t+at+4),palettefmt=u32be(t+at+8),w=u32be(t+at+12),h=u32be(t+at+16);
        unsigned pn=u16be(t+at+20),pf=u16be(t+at+22),palettes=fmt>=8&&fmt<=10?((pf&1)?1:(pn?pn:n)):0;
        size_t image_size=melee_texture_level_size(w,h,fmt);
        if(!n||n>65536||palettes>65536-n||!image_size||(palettes&&(palettefmt&0xff)>2)||!span(tn,at+24,(n+palettes)*4u))goto fail;
        HSD_PSTexGroup* d=calloc(1,offsetof(HSD_PSTexGroup,texTable)+(n+palettes)*sizeof(u8*));if(!d)goto fail;b->textures[i]=d;
        d->num=n;d->fmt=fmt;d->tlutfmt=palettefmt;d->width=w;d->height=h;d->palnum=pn;d->palflag=pf;
        for(unsigned k=0;k<n+palettes;k++){
            uint32_t offset=u32be(t+at+24+4*k);if(!offset)continue;
            size_t bytes=k<n?image_size:fmt==8?32:fmt==9?512:32768;
            if(!span(tn,offset,bytes)){
                if(k<n)goto fail;
                d->texTable[k]=&unresolved_palette;
            }else d->texTable[k]=t+offset;
        }
    }
    return b;
fail:melee_particle_bank_free(b);return NULL;
}
