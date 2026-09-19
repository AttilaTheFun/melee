#include "melee_particle_bank.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void put(u8* p,u32 v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void test_palettes(void)
{
    for(unsigned layout=0;layout<3;layout++){
        u8 bytes[32+512]={0};u8* c=bytes+32;u8* t=c+128;
        put(c+4,1);put(c+8,12);
        put(t,1);put(t+4,8);put(t+8,2);put(t+12,8);
        put(t+20,8);put(t+24,8);
        /* Retail Captain Falcon uses metadata above the low format byte.
         * psdisp explicitly casts tlutfmt to u8 before calling GX. */
        put(t+16,0x01000002);
        /* Zero means per-image palettes, an explicit count is also legal,
         * and bit zero selects one shared palette. */
        t[29]=layout==1?2:0;t[31]=layout==2?1:0;
        put(t+32,64);put(t+36,96);put(t+40,128);put(t+44,160);
        memset(t+128,0x37,32);memset(t+160,0x91,32);
        MeleeArchive a={.bytes=bytes,.size=sizeof(bytes),.data_size=512};
        MeleeParticleBank* owner=melee_particle_bank_decode(&a,0,128,128,384);assert(owner);
        HSD_PSTexGroup* group=melee_particle_bank_textures(owner)[0];
        assert(group->tlutfmt==0x01000002);
        /* Preserve unused invalid palettes, but never expose their offsets as
         * pointers. The render boundary must reject the marker before GX. */
        put(t+40,0x80a8812a);
        MeleeParticleBank* deferred=melee_particle_bank_decode(&a,0,128,128,384);assert(deferred);
        HSD_PSTexGroup* dg=melee_particle_bank_textures(deferred)[0];
        assert(!melee_particle_bank_palette_resolved(dg->texTable[2]));
        assert(melee_particle_bank_palette_resolved(dg->texTable[0]));
        assert(melee_particle_bank_palette_resolved(NULL));
        melee_particle_bank_free(deferred);put(t+40,128);
        put(t+16,0x01000003);
        assert(!melee_particle_bank_decode(&a,0,128,128,384));
        put(t+16,0x01000002);
        memset(bytes,0xa5,sizeof(bytes));
        assert(group->num==2&&group->texTable[2][0]==0x37);
        if(layout!=2)assert(group->texTable[3][0]==0x91);
        melee_particle_bank_free(owner);
    }
}
int main(int argc,char** argv){
    test_palettes();
    assert(argc==1||argc==2);u8* bytes;long size;
    if(argc==2){FILE* f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));size=ftell(f);rewind(f);
        bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
    }else{
        unsigned data_size=4096+24768;const char symbol[]="effCommonDataTable";
        size=32+data_size+16+sizeof(symbol);bytes=calloc(1,size);assert(bytes);
        put(bytes,size);put(bytes+4,data_size);put(bytes+8,2);put(bytes+12,1);
        u8* d=bytes+32;put(d,1024);put(d+4,4096);
        u8* c=d+1024;put(c,0x00420000);put(c+8,592);
        for(unsigned i=0;i<592;i++)put(c+12+4*i,2380);
        put(c+2380+40,0xbf800000);put(c+2380+44,0x41a00000);
        u8* t=d+4096;put(t,36);for(unsigned i=0;i<36;i++)put(t+4+4*i,148);
        put(t+148,3);put(t+152,3);put(t+156,2);put(t+160,64);put(t+164,64);
        for(unsigned i=0;i<3;i++)put(t+172+4*i,192+8192*i);
        put(d+data_size,0);put(d+data_size+4,4);memcpy(d+data_size+16,symbol,sizeof(symbol));
    }
    MeleeArchive a;uint32_t root,cmd,tex;MeleeHostBool present;
    assert(melee_archive_open(&a,bytes,size)&&melee_archive_find(&a,"effCommonDataTable",&root));
    assert(melee_archive_pointer(&a,root,&cmd,&present)&&present&&melee_archive_pointer(&a,root+4,&tex,&present)&&present&&tex>cmd);
    MeleeParticleBank* b=melee_particle_bank_decode(&a,cmd,tex-cmd,tex,a.data_size-tex);assert(b);
    assert(melee_particle_bank_command_count(b)==592&&melee_particle_bank_texture_count(b)==36);
    HSD_PSCmdList** commands=melee_particle_bank_commands(b);
    for(unsigned i=0;i<592;i++){
        uint32_t offset;assert(melee_archive_u32(&a,cmd+12+4*i,&offset));
        if(!offset){assert(!commands[i]);continue;}
        uint32_t bits;assert(melee_archive_u32(&a,cmd+offset+8,&bits));assert(commands[i]->kind==((bits&0xf1ffffff)|0x08000000));
        for(unsigned k=0;k<12;k++){float value;assert(melee_archive_f32(&a,cmd+offset+12+4*k,&value));float actual;memcpy(&actual,(u8*)commands[i]+12+4*k,4);assert(value==actual);}
    }
    assert(!melee_particle_bank_decode(&a,cmd,12,tex,a.data_size-tex));
    uint32_t group,image;assert(melee_archive_u32(&a,tex+4,&group)&&melee_archive_u32(&a,tex+group+24,&image));
    u8 saved_pixels[8192];assert(a.data_size-tex-image>=sizeof(saved_pixels));memcpy(saved_pixels,bytes+32+tex+image,sizeof(saved_pixels));
    uint32_t badslot=tex+4+4*35,old;assert(melee_archive_u32(&a,badslot,&old));
    put(bytes+32+badslot,a.data_size);assert(!melee_particle_bank_decode(&a,cmd,tex-cmd,tex,a.data_size-tex));put(bytes+32+badslot,old);
    uint32_t first_command;assert(melee_archive_u32(&a,cmd+12,&first_command));assert(melee_archive_u32(&a,cmd+first_command+12,&old));
    put(bytes+32+cmd+first_command+12,0x7fc00000);assert(!melee_particle_bank_decode(&a,cmd,tex-cmd,tex,a.data_size-tex));put(bytes+32+cmd+first_command+12,old);
    for(unsigned i=0;i<melee_particle_bank_texture_count(b);i++){
        u32 offset;assert(melee_archive_u32(&a,tex+4+4*i,&offset));
        HSD_PSTexGroup* group=melee_particle_bank_textures(b)[i];
        if(!offset){assert(!group);continue;}
        unsigned palettes=group->fmt>=8&&group->fmt<=10?
            (group->palflag&1?1:group->palnum?group->palnum:group->num):0;
        for(unsigned p=0;p<palettes;p++){
            u32 source;assert(melee_archive_u32(&a,tex+offset+24+4*(group->num+p),&source));
            if(!source){assert(!group->texTable[group->num+p]);continue;}
            size_t length=group->fmt==8?32:group->fmt==9?512:32768;
            assert(!memcmp(group->texTable[group->num+p],bytes+32+tex+source,length));
        }
    }
    memset(bytes,0xa5,size);free(bytes);
    assert(commands[0]->size==20&&commands[0]->random==-1);
    HSD_PSTexGroup* first=melee_particle_bank_textures(b)[0];assert(first&&first->num==3&&first->width==64&&first->height==64&&first->texTable[0]);
    assert(!memcmp(first->texTable[0],saved_pixels,sizeof(saved_pixels)));
    melee_particle_bank_free(b);puts("Common particle bank: 592 command headers, 36 texture groups, bounds and owned lifetime passed");
}
