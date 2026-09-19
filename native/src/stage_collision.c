#include "melee_stage_collision.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static int span(const MeleeArchive* a,uint32_t at,size_t bytes){return at<=a->data_size&&bytes<=a->data_size-at;}
static uint16_t half(const MeleeArchive* a,uint32_t at){const uint8_t* p=a->bytes+32+at;return (uint16_t)((p[0]<<8)|p[1]);}
static int array(const MeleeArchive* a,uint32_t slot,uint32_t n,unsigned stride,uint32_t* at){
    MeleeHostBool present;return melee_archive_pointer(a,slot,at,&present)&&(!n||(present&&span(a,*at,(size_t)n*stride)));
}
static int interval(int start,int count,int limit){return count>=0&&(!count||(start>=0&&start<=limit&&count<=limit-start));}
static int ranges(const MeleeArchive* a,uint32_t at,int limit,s16* out){
    for(unsigned i=0;i<10;i++){uint16_t bits=half(a,at+2*i);memcpy(out+i,&bits,2);}
    for(unsigned i=0;i<10;i+=2)if(!interval(out[i],out[i+1],limit))return 0;
    return 1;
}
void melee_stage_collision_free(MapCollData* c){if(c){free(c->verts);free(c->lines);free(c->joints);free(c);}}
MapCollData* melee_stage_collision_decode(const MeleeArchive* a){
    uint32_t root,nv,nl,nj,v,l,j,tail;
    if(!a||!melee_archive_find(a,"coll_data",&root)||!span(a,root,48)||
       !melee_archive_u32(a,root+4,&nv)||nv>2048||!melee_archive_u32(a,root+12,&nl)||nl>1536||
       !melee_archive_u32(a,root+40,&nj)||nj>256||!melee_archive_u32(a,root+44,&tail)||
       !array(a,root,nv,8,&v)||!array(a,root+8,nl,16,&l)||!array(a,root+36,nj,40,&j))return NULL;
    MapCollData* c=calloc(1,sizeof(*c));if(!c)return NULL;
    c->vert_count=nv;c->line_count=nl;c->joint_count=nj;memcpy(&c->x2C,&tail,4);
    s16 r[10];if(!ranges(a,root+16,nl,r))goto fail;
#define SET_RANGES(dst) do { (dst)->floor_start=r[0];(dst)->floor_count=r[1];\
    (dst)->ceiling_start=r[2];(dst)->ceiling_count=r[3];\
    (dst)->right_wall_start=r[4];(dst)->right_wall_count=r[5];\
    (dst)->left_wall_start=r[6];(dst)->left_wall_count=r[7];\
    (dst)->dynamic_start=r[8];(dst)->dynamic_count=r[9]; } while(0)
    SET_RANGES(c);
    c->verts=calloc(nv?nv:1,sizeof(*c->verts));c->lines=calloc(nl?nl:1,sizeof(*c->lines));c->joints=calloc(nj?nj:1,sizeof(*c->joints));
    if(!c->verts||!c->lines||!c->joints)goto fail;
    for(unsigned i=0;i<nv;i++){
        Vec2* p=c->verts+i;
        if(!melee_archive_f32(a,v+8*i,&p->x)||!isfinite(p->x)||!melee_archive_f32(a,v+8*i+4,&p->y)||!isfinite(p->y))goto fail;
    }
    for(unsigned i=0;i<nl;i++){
        MapLine* p=c->lines+i;uint32_t at=l+16*i;
        p->v0_idx=half(a,at);p->v1_idx=half(a,at+2);if(p->v0_idx>=nv||p->v1_idx>=nv)goto fail;
        s16 links[4];for(unsigned k=0;k<4;k++){uint16_t bits=half(a,at+4+2*k);memcpy(links+k,&bits,2);if(links[k]<-1||links[k]>=(int)nl)goto fail;}
        p->prev_id0=links[0];p->next_id0=links[1];p->prev_id1=links[2];p->next_id1=links[3];
        p->hi_flags=half(a,at+12);p->lo_flags=half(a,at+14);
    }
    for(unsigned i=0;i<nj;i++){
        MapJoint* p=c->joints+i;uint32_t at=j+40*i;
        if(!ranges(a,at,nl,r))goto fail;SET_RANGES(p);
        float b[4];for(unsigned k=0;k<4;k++)if(!melee_archive_f32(a,at+20+4*k,b+k)||!isfinite(b[k]))goto fail;
        if(b[0]>b[2]||b[1]>b[3])goto fail;
        p->left_bound=b[0];p->bottom_bound=b[1];p->right_bound=b[2];p->top_bound=b[3];
        uint16_t bits=half(a,at+36);memcpy(&p->vtx_start,&bits,2);bits=half(a,at+38);memcpy(&p->vtx_count,&bits,2);
        if(!interval(p->vtx_start,p->vtx_count,nv))goto fail;
    }
    return c;
fail:melee_stage_collision_free(c);return NULL;
}
