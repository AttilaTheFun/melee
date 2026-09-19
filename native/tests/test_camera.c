#include "melee_camera.h"
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/wobj.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(unsigned char* p, uint32_t v)
{ p[0]=v>>24; p[1]=v>>16; p[2]=v>>8; p[3]=v; }
static void number(unsigned char* p, float v)
{ uint32_t bits; memcpy(&bits,&v,4); word(p,bits); }
int main(int argc, char** argv)
{
    assert(argc==1 || argc==2);
    size_t length; unsigned char* bytes; uint32_t root=0;
    if (argc==2) {
        FILE* f=fopen(argv[1],"rb"); assert(f);
        assert(!fseek(f,0,SEEK_END)); long n=ftell(f); assert(n>32); rewind(f);
        length=n; bytes=malloc(length); assert(bytes);
        assert(fread(bytes,1,length,f)==length); fclose(f);
    } else {
        length=32+116+12; bytes=calloc(1,length); assert(bytes);
        word(bytes,length); word(bytes+4,116); word(bytes+8,3);
        unsigned char* d=bytes+32;
        word(d+4,1); word(d+8,640); word(d+12,480);
        word(d+16,640); word(d+20,480); word(d+24,64); word(d+28,84);
        number(d+40,10); number(d+44,5000); number(d+48,45); number(d+52,1.25);
        number(d+64+12,64); word(d+36,104); number(d+108,1);
        word(d+116,24); word(d+120,28); word(d+124,36);
    }
    MeleeArchive a; assert(melee_archive_open(&a,bytes,length));
    if (argc==2) {
        uint32_t scene, cameras; MeleeHostBool present;
        assert(melee_archive_find(&a,"ScNtcCommon_scene_data",&scene));
        assert(melee_archive_pointer(&a,scene+4,&cameras,&present) && present);
        assert(melee_archive_pointer(&a,cameras,&root,&present) && present);
    }
    MeleeCamera* c=melee_camera_decode(&a,root); assert(c);
    HSD_CObjDesc* d=melee_camera_descriptor(c);
    assert(d->common.projection_type==PROJ_PERSPECTIVE);
    assert(d->common.viewport.xmin==0 && d->common.viewport.xmax==640);
    assert(d->common.viewport.ymin==0 && d->common.viewport.ymax==480);
    assert(d->common.scissor.right==640 && d->common.scissor.bottom==480);
    assert(d->common.nnear==10 && d->common.ffar==5000);
    assert(d->common.eyepos->pos.z==64 && d->common.interest->pos.z==0);
    assert((uintptr_t)d->common.eyepos>UINT32_MAX);
    if (argc==1) assert(d->common.up_vector && d->common.up_vector->y==1);
    word(bytes+32+root+4,7); assert(!melee_camera_decode(&a,root));
    word(bytes+32+root+4,1);
    word(bytes+32+root+48,0x7fc00000); assert(!melee_camera_decode(&a,root));
    if (argc==1) {
        number(bytes+32+48,10); number(bytes+32+52,-10);
        number(bytes+32+56,-20); number(bytes+32+60,20);
        for (unsigned p=2;p<=3;p++) {
            word(bytes+32+4,p); MeleeCamera* other=melee_camera_decode(&a,0); assert(other);
            HSD_CObjDesc* od=melee_camera_descriptor(other);
            assert(od->common.projection_type==p && od->frustum.left==-20 && od->frustum.right==20);
            melee_camera_free(other);
        }
        word(bytes+32+24,103); assert(!melee_camera_decode(&a,0));
    }
    memset(bytes,0xa5,length); free(bytes);
    assert(d->common.eyepos->pos.z==64 && d->common.interest->pos.x==0);
    if (argc==1) assert(d->common.up_vector->y==1);
    assert(d->perspective.fov>40 && d->perspective.fov<46);
    melee_camera_free(c); melee_camera_free(NULL);
    puts("Owned native camera descriptors: projections, retail values, bounds and source lifetime passed");
}
