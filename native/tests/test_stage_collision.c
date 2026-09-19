#include "melee_stage_collision.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(u8* p,u32 v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static u32 read32(const u8* p){return ((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];}
static void shorts(const void* native,const u8* disk,unsigned count){for(unsigned i=0;i<count;i++){u16 n;memcpy(&n,(const u8*)native+2*i,2);assert(n==((disk[2*i]<<8)|disk[2*i+1]));}}
static void floats(const void* native,const u8* disk,unsigned count){for(unsigned i=0;i<count;i++){u32 n;memcpy(&n,(const u8*)native+4*i,4);assert(n==read32(disk+4*i));}}
int main(int argc,char** argv){
    size_t size=194;u8* b=calloc(1,size);assert(b);
    word(b,size);word(b+4,120);word(b+8,3);word(b+12,1);
    u8* d=b+32;
    word(d,48);word(d+4,2);word(d+8,64);word(d+12,1);d[19]=1;word(d+36,80);word(d+40,1);
    word(d+56,0x3f800000);d[67]=1;memset(d+68,0xff,8);d[83]=1;word(d+108,0x3f800000);d[119]=2;
    word(d+120,0);word(d+124,8);word(d+128,36);memcpy(d+140,"coll_data",10);
    if(argc==2){free(b);FILE* f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);size=n;b=malloc(size);assert(b&&fread(b,1,size,f)==size);fclose(f);}else assert(argc==1);
    MeleeArchive a;assert(melee_archive_open(&a,b,size));u32 root;assert(melee_archive_find(&a,"coll_data",&root));d=b+32;
    MapCollData* c=melee_stage_collision_decode(&a);assert(c);
    unsigned nv=c->vert_count,nl=c->line_count,nj=c->joint_count;
    u32 vo=read32(d+root),lo=read32(d+root+8),jo=read32(d+root+36);
    floats(c->verts,d+vo,2*nv);
    for(unsigned i=0;i<nl;i++)shorts(c->lines+i,d+lo+16*i,8);
    for(unsigned i=0;i<nj;i++){MapJoint* p=c->joints+i;shorts(p,d+jo+40*i,10);floats(&p->left_bound,d+jo+40*i+20,4);shorts(&p->vtx_start,d+jo+40*i+36,2);}
    assert(nv&&nl&&nj);
    u32 saved=read32(d+root+4);word(d+root+4,2049);assert(!melee_stage_collision_decode(&a));word(d+root+4,saved);
    saved=read32(d+vo);word(d+vo,0x7fc00000);assert(!melee_stage_collision_decode(&a));word(d+vo,saved);
    saved=read32(d+lo);word(d+lo,0xffff0000);assert(!melee_stage_collision_decode(&a));word(d+lo,saved);
    saved=read32(d+lo+4);word(d+lo+4,0x7fff0000);assert(!melee_stage_collision_decode(&a));word(d+lo+4,saved);
    saved=read32(d+root+16);word(d+root+16,0x00007fff);assert(!melee_stage_collision_decode(&a));word(d+root+16,saved);
    saved=read32(d+root+36);word(d+root+36,a.data_size);assert(!melee_stage_collision_decode(&a));word(d+root+36,saved);
    float x=c->verts[0].x;memset(b,0xa5,size);free(b);assert(c->verts[0].x==x);
    c->lines[0].hi_flags|=1;melee_stage_collision_free(c);
    printf("Stage collision: %u vertices, %u lines, %u joints; scalar conversion, invalid indices/ranges/NaN and mutable ownership passed\n",nv,nl,nj);
}
