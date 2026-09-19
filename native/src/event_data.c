#include "melee_event_data.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define COUNT 51
_Static_assert(sizeof(gm_801BAB40_src)==28 && sizeof(struct gm_evbonus)==24, "Event scalar records");
#define NONE UINT32_MAX
struct MeleeEventData {
    struct gm_804D6900_t entries[COUNT];
    struct gm_804D6900_t* table[COUNT];
    struct gm_evinit rules[COUNT];
    struct gm_evbonus bonuses[COUNT];
    struct gm_evstage_table stages[COUNT];
    gm_801BAB40_src players[COUNT][12];
    union {
        struct gm_804D6900_x4_t pair;
        float vec[3];
        unsigned char chars[33];
        u32 sounds[12];
    } aux[COUNT];
};
static int range(const MeleeArchive* a,uint32_t at,size_t n){return at<=a->data_size&&n<=a->data_size-at;}
static int ref(const MeleeArchive* a,uint32_t slot,uint32_t* out){
    MeleeHostBool present;if(!melee_archive_pointer(a,slot,out,&present))return 0;
    if(!present)*out=NONE;return 1;
}
static int scalar(const MeleeArchive* a,uint32_t at,float* out){return melee_archive_f32(a,at,out)&&isfinite(*out);}
static u16 short_at(const MeleeArchive* a,uint32_t at){const u8* p=a->bytes+32+at;return (u16)p[0]<<8|p[1];}
static int player(const MeleeArchive* a,uint32_t at,gm_801BAB40_src* storage,gm_801BAB40_src** out){
    *out=NULL;if(at==NONE)return 1;if(!range(a,at,28))return 0;
    memcpy(storage,a->bytes+32+at,12);storage->x12=short_at(a,at+12);storage->hp=short_at(a,at+14);
    if(!scalar(a,at+16,&storage->x18)||!scalar(a,at+20,&storage->x1C)||!scalar(a,at+24,&storage->x20))return 0;
    *out=storage;return 1;
}
static int rules(const MeleeArchive* a,uint32_t at,struct gm_evinit* r){
    if(!range(a,at,40))return 0;const u8* p=a->bytes+32+at;
    r->x0_0=p[0]>>5;r->x0_3=(p[0]>>2)&7;r->x0_6=(p[0]>>1)&1;r->x0_7=p[0]&1;
    r->x1_0=p[1]>>7;r->x1_1=(p[1]>>6)&1;r->x1_2=(p[1]>>5)&1;
    r->x1_3=(p[1]>>4)&1;r->x1_4=(p[1]>>3)&1;r->x1_5=p[1]&7;
    r->is_teams=p[2];r->item_freq=(s8)p[3];r->sd_penalty=(s8)p[4];r->unk5=p[5];r->stkind=short_at(a,at+6);
    uint32_t time,hi,lo,value;
    if(!melee_archive_u32(a,at+8,&time)||!melee_archive_u32(a,at+16,&hi)||
       !melee_archive_u32(a,at+20,&lo)||!melee_archive_u32(a,at+24,&value))return 0;
    r->time_limit=time;memcpy(r->padC,p+12,4);r->x10=(u64)hi<<32|lo;r->x18=(s32)value;
    return scalar(a,at+28,&r->x1C)&&scalar(a,at+32,&r->game_speed)&&scalar(a,at+36,&r->unk24);
}
static int auxiliary(MeleeEventData* d,const MeleeArchive* a,unsigned i,uint32_t at){
    if(at==NONE)return 1;
    d->entries[i].x4=(struct gm_804D6900_x4_t*)&d->aux[i];
    switch(i){
    case 0: {uint32_t x,y;if(!range(a,at,8)||!melee_archive_u32(a,at,&x)||!melee_archive_u32(a,at+4,&y))return 0;d->aux[i].pair.x0=(s32)x;d->aux[i].pair.x4=(s32)y;return 1;}
    case 4: {uint32_t value;if(!melee_archive_u32(a,at,&value))return 0;d->aux[i].pair.x0=(s32)value;return 1;}
    case 12:if(!range(a,at,12))return 0;for(unsigned k=0;k<3;k++)if(!scalar(a,at+4*k,&d->aux[i].vec[k]))return 0;return 1;
    case 13:case 25:case 46:
        for(unsigned k=0;k<33;k++){
            if(at>a->data_size||k>=a->data_size-at)return 0;
            u8 c=a->bytes[32+at+k];if(c>33)return 0;d->aux[i].chars[k]=c;if(c==33)return 1;
        }
        return 0;
    case 36:if(!range(a,at,48))return 0;for(unsigned k=0;k<12;k++)if(!melee_archive_u32(a,at+4*k,&d->aux[i].sounds[k]))return 0;return 1;
    case 43: {
        uint32_t value,p;if(!range(a,at,8)||!melee_archive_u32(a,at,&value)||!ref(a,at+4,&p)||p==NONE)return 0;
        gm_801BAB40_src* converted;
        if(!player(a,p,&d->players[i][11],&converted))return 0;
        d->aux[i].pair.x0=(s32)value;d->aux[i].pair.x4=(intptr_t)converted;return 1;
    }
    default:return 0;
    }
}
void melee_event_data_free(MeleeEventData* d){free(d);}
struct gm_804D6900_t** melee_event_data_table(MeleeEventData* d){return d?d->table:NULL;}
MeleeEventData* melee_event_data_decode(const MeleeArchive* a){
    uint32_t root;
    if(!a||a->extern_count||!melee_archive_find(a,"sqEventInitDataLevelTbl",&root)||
       !range(a,root,COUNT*4)||a->data_size-root!=COUNT*4)return NULL;
    MeleeEventData* d=calloc(1,sizeof(*d));if(!d)return NULL;
    for(unsigned i=0;i<COUNT;i++){
        uint32_t at,aux,init,bonus,stage;
        if(!ref(a,root+4*i,&at)||at==NONE||!range(a,at,40)||
           !ref(a,at+4,&aux)||!ref(a,at+8,&init)||init==NONE||
           !ref(a,at+12,&bonus)||!ref(a,at+16,&stage))goto fail;
        struct gm_804D6900_t* e=&d->entries[i];d->table[i]=e;
        const u8* p=a->bytes+32+at;e->kind=p[0];e->flags=p[1];memcpy(e->pad2,p+2,2);
        if(!rules(a,init,&d->rules[i])||!auxiliary(d,a,i,aux))goto fail;e->evinit=&d->rules[i];
        if(bonus!=NONE){
            if(!range(a,bonus,24))goto fail;
            struct gm_evbonus* b=&d->bonuses[i];memcpy(b,a->bytes+32+bonus,8);memcpy(&b->flags,a->bytes+32+bonus+20,4);
            if(!scalar(a,bonus+8,&b->x8)||!scalar(a,bonus+12,&b->xC)||!scalar(a,bonus+16,&b->x10))goto fail;e->evbonus=b;
        }
        if(stage!=NONE){
            if(!range(a,stage,40))goto fail;
            struct gm_evstage_table* st=&d->stages[i];st->count=a->bytes[32+stage];st->pad1=a->bytes[33+stage];
            if(!st->count||st->count>6)goto fail;
            for(unsigned k=0;k<7;k++)st->stage[k]=short_at(a,stage+2+2*k);
            for(unsigned k=0;k<6;k++){
                uint32_t at;if(!ref(a,stage+16+4*k,&at)||!player(a,at,&d->players[i][5+k],&st->entries[k]))goto fail;
            }
            e->evstage_table=st;
        }
        for(unsigned k=0;k<5;k++){
            uint32_t at_player;if(!ref(a,at+20+4*k,&at_player)||!player(a,at_player,&d->players[i][k],&e->player_init[k]))goto fail;
        }
    }
    return d;
fail:free(d);return NULL;
}
