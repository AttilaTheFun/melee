#include "melee_event_data.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void put(u8* p,u32 v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static u32 word(const u8* p){return (u32)p[0]<<24|(u32)p[1]<<16|(u32)p[2]<<8|p[3];}
static u16 half(const u8* p){return (u16)p[0]<<8|p[1];}
static void floating(float f,const u8* p){u32 bits;memcpy(&bits,&f,4);assert(bits==word(p));}
static void player(const MeleeArchive* a,u32 at,gm_801BAB40_src* p){
    if(!at){assert(!p);return;}assert(p);const u8* raw=a->bytes+32+at;
    assert(!memcmp(p,raw,12)&&p->x12==half(raw+12)&&p->hp==half(raw+14));
    floating(p->x18,raw+16);floating(p->x1C,raw+20);floating(p->x20,raw+24);
}
int main(int argc,char** argv){
    assert(argc==1||argc==2);u8* bytes;size_t size;
    if(argc==2){FILE* f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));size=ftell(f);rewind(f);
        bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
    }else{
        const char name[]="sqEventInitDataLevelTbl";size=32+312+53*4+8+sizeof(name);bytes=calloc(1,size);assert(bytes);
        put(bytes,size);put(bytes+4,312);put(bytes+8,53);put(bytes+12,1);u8* d=bytes+32;
        d[0]=0xae;d[1]=0xd5;d[3]=255;d[4]=254;d[7]=32;put(d+8,180);put(d+16,0x12345678);put(d+20,0x90abcdef);
        for(unsigned i=0;i<3;i++)put(d+28+4*i,0x3f800000);
        d[41]=0x20;put(d+60,80);d[80]=3;d[82]=4;d[93]=33;
        for(unsigned i=0;i<3;i++)put(d+96+4*i,0x3f800000);
        for(unsigned i=0;i<51;i++){put(d+108+4*i,40);put(d+312+4*i,108+4*i);}
        put(d+312+51*4,48);put(d+312+52*4,60);
        put(d+312+53*4,108);memcpy(d+312+53*4+8,name,sizeof(name));
    }
    MeleeArchive a;u32 root;assert(melee_archive_open(&a,bytes,size)&&melee_archive_find(&a,"sqEventInitDataLevelTbl",&root));
    MeleeEventData* owner=melee_event_data_decode(&a);assert(owner);struct gm_804D6900_t** table=melee_event_data_table(owner);
    struct gm_evinit saved[51];
    for(unsigned i=0;i<51;i++){
        u32 at=word(bytes+32+root+4*i);const u8* e=bytes+32+at;struct gm_804D6900_t* d=table[i];
        assert(d&&d->kind==e[0]&&d->flags==e[1]);const u8* r=bytes+32+word(e+8);struct gm_evinit* t=d->evinit;assert(t);
        assert(t->x0_0==r[0]>>5&&t->x0_3==((r[0]>>2)&7)&&t->x0_6==((r[0]>>1)&1)&&t->x0_7==(r[0]&1));
        assert(t->x1_0==r[1]>>7&&t->x1_1==((r[1]>>6)&1)&&t->x1_2==((r[1]>>5)&1)&&t->x1_3==((r[1]>>4)&1)&&t->x1_4==((r[1]>>3)&1)&&t->x1_5==(r[1]&7));
        assert(t->is_teams==r[2]&&t->item_freq==(s8)r[3]&&t->sd_penalty==(s8)r[4]&&t->unk5==r[5]&&t->stkind==half(r+6));
        assert(t->time_limit==word(r+8)&&t->x10==((u64)word(r+16)<<32|word(r+20))&&(u32)t->x18==word(r+24));
        floating(t->x1C,r+28);floating(t->game_speed,r+32);floating(t->unk24,r+36);saved[i]=*t;
        for(unsigned k=0;k<5;k++)player(&a,word(e+20+4*k),d->player_init[k]);
        u32 st=word(e+16);if(st){assert(d->evstage_table&&d->evstage_table->count==bytes[32+st]);
            for(unsigned k=0;k<7;k++)assert(d->evstage_table->stage[k]==half(bytes+32+st+2+2*k));
            for(unsigned k=0;k<6;k++)player(&a,word(bytes+32+st+16+4*k),d->evstage_table->entries[k]);
        }else assert(!d->evstage_table);
        u32 bonus=word(e+12);if(bonus){const u8* b=bytes+32+bonus;assert(d->evbonus&&!memcmp(d->evbonus,b,8));
            floating(d->evbonus->x8,b+8);floating(d->evbonus->xC,b+12);floating(d->evbonus->x10,b+16);assert(!memcmp(&d->evbonus->flags,b+20,4));
        }else assert(!d->evbonus);
        u32 aux;MeleeHostBool has_aux;assert(melee_archive_pointer(&a,at+4,&aux,&has_aux));if(has_aux){const u8* p=bytes+32+aux;assert(d->x4);
            if(i==0)assert((u32)d->x4->x0==word(p)&&d->x4->x4==(s32)word(p+4));
            else if(i==4)assert((u32)d->x4->x0==word(p));
            else if(i==12){for(unsigned k=0;k<3;k++)floating(((float*)d->x4)[k],p+4*k);}
            else if(i==36){for(unsigned k=0;k<12;k++)assert(((u32*)d->x4)[k]==word(p+4*k));}
            else if(i==43){assert((u32)d->x4->x0==word(p));player(&a,word(p+4),(gm_801BAB40_src*)d->x4->x4);}
            else {assert(i==13||i==25||i==46);unsigned k=0;do{assert(((u8*)d->x4)[k]==p[k]);}while(p[k++]!=33&&k<33);}
        }else assert(!d->x4);
    }
    u32 first=word(bytes+32+root),init=word(bytes+32+first+8),old=word(bytes+32+init+28);
    put(bytes+32+init+28,0x7fc00000);assert(!melee_event_data_decode(&a));put(bytes+32+init+28,old);
    put(bytes+32+root,a.data_size);assert(!melee_event_data_decode(&a));
    memset(bytes,0xa5,size);free(bytes);
    for(unsigned i=0;i<51;i++)assert(!memcmp(table[i]->evinit,&saved[i],sizeof(saved[i])));
    melee_event_data_free(owner);puts("Event data: 51 native records, bitfields, players, stages, auxiliary layouts and owned lifetime passed");
}
