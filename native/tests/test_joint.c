#include "melee_joint.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t file[32+384+9*4];
static void word(uint8_t* p, uint32_t v)
{ p[0]=v>>24; p[1]=v>>16; p[2]=v>>8; p[3]=v; }
static void number(uint8_t* p, float v)
{ uint32_t bits; memcpy(&bits,&v,4); word(p,bits); }
static void fixture(void)
{
    memset(file,0,sizeof(file)); word(file,sizeof(file)); word(file+4,384); word(file+8,9);
    uint8_t* d=file+32;
    for(unsigned node=0;node<4;node++) {
        for(unsigned i=0;i<3;i++) {
            number(d+node*64+20+i*4,(float)(node*3+i)/10);
            number(d+node*64+32+i*4,1);
            number(d+node*64+44+i*4,(float)(node*3+i));
        }
    }
    word(d+64,304); word(d+72,0); word(d+76,128);
    word(d+8,192); word(d+136,192); word(d+132,1<<12);
    word(d+80,320); word(d+120,256); word(d+124,324);
    memcpy(d+304,"joint_class",12);
    for(unsigned i=0;i<12;i++) number(d+256+i*4,(float)i);
    const uint32_t slots[]={64,72,76,8,136,80,120,124,124};
    for(unsigned i=0;i<9;i++) word(d+384+i*4,slots[i]);
}
static MeleeJointGraph* decode(void)
{ MeleeArchive a; assert(melee_archive_open(&a,file,sizeof(file))); return melee_joint_decode(&a,64); }
static void deep_graph(void)
{
    const unsigned count=2048, data_size=count*64, size=32+data_size+(count-1)*4;
    uint8_t* bytes=calloc(1,size); assert(bytes);
    word(bytes,size); word(bytes+4,data_size); word(bytes+8,count-1);
    for(unsigned i=0;i<count-1;i++) {
        word(bytes+32+i*64+8,(i+1)*64);
        word(bytes+32+data_size+i*4,i*64+8);
    }
    MeleeArchive a; assert(melee_archive_open(&a,bytes,size));
    MeleeJointGraph* g=melee_joint_decode(&a,0); assert(g && melee_joint_count(g)==count);
    const MeleeJointNode* node=melee_joint_root(g);
    for(unsigned i=0;i<count;i++) { assert(node && node->offset==i*64); node=node->child; }
    assert(!node); melee_joint_free(g); free(bytes);
}
int main(void)
{
    fixture(); MeleeJointGraph* g=decode(); assert(g && melee_joint_count(g)==4);
    const MeleeJointNode* r=melee_joint_root(g);
    assert(r->offset==64 && r->child->offset==0 && r->next->offset==128);
    assert(r->child->child==r->next->child && r->next->flags==(1<<12));
    assert(r->child->child->offset==192 && !r->child->child->next);
    assert(r->payload_offset==320 && r->constraints_offset==324);
    assert(r->child->payload_offset==UINT32_MAX);
    assert(!strcmp(r->class_name,"joint_class") && r->has_inverse_bind);
    assert(r->inverse_bind[2][3]==11 && r->position[2]==5 && r->rotation[0]==0.3f);
    assert(!r->child->class_name && !r->child->has_inverse_bind);
    memset(file,0,sizeof(file)); /* All decoded joint fields are owned. */
    assert(!strcmp(r->class_name,"joint_class") && r->inverse_bind[2][3]==11);
    melee_joint_free(g);
    fixture(); word(file+32+200,64); word(file+32+384+8*4,200); assert(!decode());
    fixture(); word(file+32+72,384); assert(!decode());
    fixture(); word(file+32+72,380); assert(!decode());
    fixture(); word(file+32+72,3); assert(!decode());
    fixture(); number(file+32+84,NAN); assert(!decode());
    fixture(); word(file+32+120,360); assert(!decode());
    fixture(); word(file+32+64,383); file[32+383]='x'; assert(!decode());
    fixture(); word(file+32+384+1*4,124); /* Zero without relocation is null. */
    g=decode(); assert(g && melee_joint_count(g)==3 && !melee_joint_root(g)->child); melee_joint_free(g);
    fixture(); word(file+32+384+2*4,124); assert(!decode()); /* Nonzero without relocation. */
    assert(!melee_joint_decode(NULL,0)); melee_joint_free(NULL);
    deep_graph();
    puts("Joint decoding: native transforms, shared references, relocated zero, owned storage and malformed graphs passed");
}
