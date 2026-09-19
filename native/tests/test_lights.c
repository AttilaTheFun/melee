#include "melee_lights.h"
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/wobj.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(unsigned char* p,uint32_t v)
{ p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v; }
static void number(unsigned char* p,float v)
{ uint32_t bits;memcpy(&bits,&v,4);word(p,bits); }
int main(int argc,char** argv)
{
    assert(argc==1||argc==2); size_t length; unsigned char* bytes;
    uint32_t roots[2]={0,0}; unsigned count=1;
    if(argc==2){
        FILE* f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));
        long size=ftell(f);assert(size>32);rewind(f);length=size;
        bytes=malloc(length);assert(bytes&&fread(bytes,1,length,f)==length);fclose(f);
    }else{
        length=148;bytes=calloc(1,length);assert(bytes);
        word(bytes,length);word(bytes+4,100);word(bytes+8,3);
        unsigned char* d=bytes+32;
        word(d+4,28);word(d+8,4<<16);word(d+12,0x4c4c4cff);
        word(d+36,13<<16);word(d+40,0xffffffff);word(d+44,56);word(d+52,76);
        number(d+68,64);number(d+76,50);
        word(d+100,4);word(d+104,44);word(d+108,52);
    }
    MeleeArchive a;assert(melee_archive_open(&a,bytes,length));
    if(argc==2){
        uint32_t scene,list,entry;MeleeHostBool present;
        assert(melee_archive_find(&a,"ScNtcCommon_scene_data",&scene));
        assert(melee_archive_pointer(&a,scene+8,&list,&present)&&present);
        for(unsigned i=0;i<2;i++){
            assert(melee_archive_pointer(&a,list+4*i,&entry,&present)&&present);
            assert(melee_archive_pointer(&a,entry,&roots[i],&present)&&present);
        }
        count=2;
    }
    MeleeLights* owned[2]={0};
    for(unsigned i=0;i<count;i++){
        owned[i]=melee_lights_decode(&a,roots[i]);assert(owned[i]);
        assert(melee_lights_count(owned[i])==(argc==1?2:1));
    }
    HSD_LightDesc* ambient=melee_lights_descriptor(owned[0]);
    HSD_LightDesc* infinite=argc==1?ambient->next:melee_lights_descriptor(owned[1]);
    assert(ambient->flags==4 && ambient->color.r==76 && ambient->color.a==255);
    assert(infinite->flags==13 && infinite->color.r==255 && infinite->position);
    assert((uintptr_t)infinite->position>UINT32_MAX && infinite->u.shininess);
    Vec3 expected=infinite->position->pos;float shininess=*infinite->u.shininess;
    if(argc==1){
        unsigned char* d=bytes+32;
        for(unsigned type=2;type<=3;type++)for(unsigned raw=0;raw<=1;raw++){
            word(d+36,(type<<16)|raw);
            memset(d+76,0,24);
            if(raw){for(unsigned k=0;k<6;k++)number(d+76+4*k,k+1);}
            else if(type==2){number(d+76,.5f);number(d+80,100);word(d+84,GX_DA_MEDIUM);}
            else{number(d+76,45);word(d+80,GX_SP_COS);number(d+84,.5f);number(d+88,100);word(d+92,GX_DA_MEDIUM);}
            MeleeLights* other=melee_lights_decode(&a,0);assert(other);
            HSD_LightDesc* p=melee_lights_descriptor(other)->next;
            if(raw)assert(p->u.attn->k2==6);
            else if(type==2)assert(p->u.point->ref_dist==100&&p->u.point->dist_func==GX_DA_MEDIUM);
            else assert(p->u.spot->cutoff==45&&p->u.spot->spot_func==GX_SP_COS);
            melee_lights_free(other);
        }
        word(d+76,0x7fc00000);assert(!melee_lights_decode(&a,0));
        number(d+76,1);word(d+44,99);assert(!melee_lights_decode(&a,0));word(d+44,56);
        word(bytes+8,4);word(d+112,32);assert(melee_archive_open(&a,bytes,length));
        assert(!melee_lights_decode(&a,0));
    }
    memset(bytes,0xa5,length);free(bytes);
    assert(!memcmp(&infinite->position->pos,&expected,sizeof(expected)));
    assert(*infinite->u.shininess==shininess);
    for(unsigned i=0;i<count;i++)melee_lights_free(owned[i]);
    melee_lights_free(NULL);
    puts("Owned light descriptors: retail lights, linked lifetime, attenuation variants and invalid references passed");
}
