#include "../../src/melee/mp/mplib.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
void OSReport(char*s,...){(void)s;}
void HSD_Panic(char*s,u32 n,char*m){(void)s;(void)n;(void)m;abort();}
#undef __assert
void __assert(char*s,u32 n,char*m){(void)s;(void)n;(void)m;abort();}
void PSVECNormalize(Vec*src,Vec*dst){float length=sqrtf(src->x*src->x+src->y*src->y+src->z*src->z);dst->x=src->x/length;dst->y=src->y/length;dst->z=src->z/length;}
int main(void){
 CollLine lines[4]={0};MapLine raw[4]={0};CollVtx vertices[2]={0};CollJoint joint={0};MapJoint inner={0};MapCollData header={0};header.line_count=4;mpLib_804D64B4=&header;
 groundCollLine=lines;groundCollVtx=vertices;groundCollJoint=&joint;jointListStart=&joint;joint.inner=&inner;inner.floor_start=2;inner.floor_count=1;didCheckBounding=true;
 vertices[0].pos=(Vec2){-10,0};vertices[1].pos=(Vec2){10,0};raw[2]=(MapLine){0,1,-1,-1,-1,-1,0,123};lines[2].x0=&raw[2];lines[2].flags=CollLine_Floor|LINE_FLAG_ENABLED;
 Vec3 hit,normal;int id=-1;u32 flags=0;
 assert(mpCheckFloor(0,5,0,-5,0,&hit,&id,&flags,&normal,-1,-1,-1,NULL,NULL));assert(id==2&&hit.x==0&&hit.y==0&&flags==123&&normal.y==1);
 assert(!mpCheckFloor(0,5,0,-5,0,&hit,&id,&flags,&normal,2,-1,-1,NULL,NULL));
 mpFloorGetRight(2,&hit);assert(hit.x==10&&hit.y==0);mpFloorGetLeft(2,&hit);assert(hit.x==-10&&hit.y==0);
 puts("Native collision indexes: nonzero floor index, skip ID and both endpoint helpers passed");
}
