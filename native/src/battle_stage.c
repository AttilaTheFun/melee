#include "melee_battle_stage.h"
#include "melee_stage_models.h"
#include "melee_stage_collision.h"
#include "melee_stage_params.h"
#include "melee_item_colors.h"
#include "melee_dynamic_model.h"
#include "melee_item_scripts.h"
#include "melee_particle_bank.h"
#include "melee_environment.h"
#include "melee_item_article.h"
#include "melee_sis_bank.h"
#include <melee/gr/grcastle.h>
#include <melee/gr/grkongo.h>
#include <melee/gr/grzebes.h>
#include <melee/gr/grvenom.h>
#include <melee/gr/grcorneria.h>
#include <melee/gr/grmutecity.h>
#include <melee/gr/gricemt.h>
#include <melee/gr/grpushon.h>
#include <melee/gr/grshrineroute.h>
#include <sysdolphin/baselib/particle.h>
#include <sysdolphin/baselib/debug.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define NONE UINT32_MAX
typedef enum { STAGE_SCALARS, STAGE_BRINSTAR, STAGE_CASTLE, STAGE_YORSTER, STAGE_GREENS, STAGE_SHRINE, STAGE_VENOM, STAGE_CORNERIA, STAGE_MUTECITY, STAGE_BIGBLUE, STAGE_FOURSIDE, STAGE_ICEMT, STAGE_FLATZONE, STAGE_RCRUISE, STAGE_PURA, STAGE_TARGET, STAGE_TARGET_EMPTY, STAGE_FIGUREGET, STAGE_PUSHON, STAGE_KINOKO, STAGE_MAZE, STAGE_ZEBES_ROUTE, STAGE_BIGBLUE_ROUTE, STAGE_HEAL, STAGE_HOMERUN, STAGE_INISHIE1, STAGE_INISHIE2 } StageVariant;
typedef struct {
    HSD_Archive bridge;
    MeleeStageModels* models;UnkStageDat* header;
    MapCollData* collision;GroundParam* params;union ColorOverlay_x8_t* hazards[4];MeleeItemColors* colors;
    MeleeDynamicModel* quake;MeleeItemScripts* scripts;MeleeParticleBank* particles;
    MeleeSisBank* text;MeleeEnvironment* lighting;struct GroundItemData* items[5];MeleeItemArticle* route_articles[4];struct GroundItemData route_items[4];struct GroundItemData item, apple_item;MeleeItemArticle* article;MeleeItemArticle* apple_article;void* hazard_params;void* hazard_hit;DynamicsDesc flags[4];
} BattleStage;
static void destroy(HSD_Archive* b){
    if(!b)return;BattleStage* s=b->top_ptr;
    for(unsigned i=0;i<4;i++)free(s->flags[i].params);
    for(unsigned i=0;i<4;i++)melee_item_article_free(s->route_articles[i]);
    melee_sis_bank_free(s->text);melee_item_article_free(s->article);melee_item_article_free(s->apple_article);melee_item_colors_free(s->colors);melee_environment_free(s->lighting);melee_particle_bank_free(s->particles);melee_item_scripts_free(s->scripts);
    melee_dynamic_model_free(s->quake);melee_stage_params_free(s->params);melee_stage_collision_free(s->collision);melee_stage_models_free(s->models);free(s->hazard_params);free(s->hazard_hit);free(s);
}
static void* lookup(HSD_Archive* b,const char* name){
    BattleStage* s=b->top_ptr;if(!name)return NULL;
    if(!strcmp(name,"GrdIzumi_cd_wt_GrdIzumiDummy1_1_image_desc"))return melee_stage_models_find_image(s->models,name);
    if(!strcmp(name,"SIS_GrPStadiumData")||!strcmp(name,"SIS_GrCorneriaData")||!strcmp(name,"SIS_GrHomerunData"))return melee_sis_bank_table(s->text);
    if(!strcmp(name,"GrdPStadiumBG_OVDummy_mat6962_GrdPStadiumDummy_0_image_desc"))return melee_stage_models_find_image(s->models,name);
    const char* flag_names[]={"dynamicsdata_flag3","dynamicsdata_flag4","dynamicsdata_flag6","dynamicsdata_shipflag"};
    for(unsigned i=0;i<4;i++)if(!strcmp(name,flag_names[i]))return s->flags[i].params?&s->flags[i]:NULL;
    if(!strcmp(name,"map_head"))return s->header;
    if(!strcmp(name,"coll_data"))return s->collision;
    if(!strcmp(name,"grGroundParam"))return s->params;
    if(!strcmp(name,"yakumono_param"))return s->hazard_params?s->hazard_params:s->hazards;
    if(!strcmp(name,"itemdata"))return s->items;
    if(!strcmp(name,"ALDYakuAll"))return melee_item_scripts_table(s->scripts);
    if(!strcmp(name,"quake_model_set"))return melee_dynamic_model_descriptor(s->quake);
    if(!strcmp(name,"map_plit"))return melee_environment_lights(s->lighting);
    /* Opaque non-NULL markers; bank registration uses the native owner API. */
    if(!strcmp(name,"map_ptcl")||!strcmp(name,"map_texg"))return s->particles;
    return NULL;
}
static HSD_Archive* decode(const MeleeArchive* a,unsigned color_count,unsigned param_words,unsigned halfwords,unsigned stage_item,unsigned stadium,StageVariant variant){
    if(!a)return NULL;BattleStage* s=calloc(1,sizeof(*s));if(!s)return NULL;s->bridge.top_ptr=s;
    u32 item,target,cmd,tex,light;MeleeHostBool present;
    if(!melee_archive_find(a,"itemdata",&item)||!melee_archive_pointer(a,item,&target,&present))goto fail;
    if(variant==STAGE_KINOKO||variant==STAGE_INISHIE2){
        const unsigned kinoko_kinds[]={It_Kind_Nokonoko,It_Kind_Patapata,It_Kind_ZGShell,It_Kind_ZRShell},kinoko_counts[]={6,5,4,5};
        const unsigned birdo_kinds[]={It_Kind_Kyasarin,It_Kind_Kyasarin_Egg},birdo_counts[]={4,3};
        unsigned count=variant==STAGE_INISHIE2?2:4;
        const unsigned* kinds=variant==STAGE_INISHIE2?birdo_kinds:kinoko_kinds;
        const unsigned* counts=variant==STAGE_INISHIE2?birdo_counts:kinoko_counts;
        for(unsigned i=0;i<count;i++){
            u32 entry,kind,article;
            if(!melee_archive_pointer(a,item+4*i,&entry,&present)||!present||!melee_archive_u32(a,entry,&kind)||kind!=kinds[i]||
               !melee_archive_pointer(a,entry+4,&article,&present)||!present)goto fail;
            s->route_articles[i]=melee_item_article_decode(a,kind,article,counts[i]);if(!s->route_articles[i])goto fail;
            s->route_items[i].unk0=kind;s->route_items[i].unk4=melee_item_article_descriptor(s->route_articles[i]);s->items[i]=&s->route_items[i];
        }
        if(!melee_archive_pointer(a,item+4*count,&target,&present)||present)goto fail;
    }else if(stage_item){
        u32 kind,article,end;if(!present||!melee_archive_u32(a,target,&kind)||kind!=stage_item||!melee_archive_pointer(a,target+4,&article,&present)||!present)goto fail;
        if(!melee_archive_pointer(a,item+((variant==STAGE_GREENS||variant==STAGE_CORNERIA)?8:4),&end,&present)||present)goto fail;
        s->article=melee_item_article_decode(a,kind,article,stage_item==It_Kind_Likelike?10:stage_item==It_Kind_Mato?1:stage_item==It_Kind_Tools?10:stage_item==It_Kind_Whitebea?8:stage_item==It_Kind_Arwing_Laser?6:stage_item==It_Kind_Heiho?3:stage_item==It_Kind_Klap?4:stage_item==It_Kind_WhispyApple?3:7);if(!s->article)goto fail;
        s->item.unk0=kind;s->item.unk4=melee_item_article_descriptor(s->article);s->items[0]=&s->item;
    }else if(present)goto fail;
    if(variant==STAGE_GREENS||variant==STAGE_CORNERIA){
        u32 entry,kind,article;
        if(!melee_archive_pointer(a,item+4,&entry,&present)||!present||!melee_archive_u32(a,entry,&kind)||kind!=(variant==STAGE_GREENS?It_Kind_WhispyHealApple:It_Kind_GreatFox_Laser)||
           !melee_archive_pointer(a,entry+4,&article,&present)||!present)goto fail;
        s->apple_article=melee_item_article_decode(a,kind,article,variant==STAGE_GREENS?3:2);if(!s->apple_article)goto fail;
        s->apple_item.unk0=kind;s->apple_item.unk4=melee_item_article_descriptor(s->apple_article);s->items[1]=&s->apple_item;
    }
    s->models=melee_stage_models_decode(a);if(!s->models)goto fail;
    /* GrNBr model 33 is never instantiated by grBb_Route_StageCallbacks or
     * grBigBlueRoute_8020B9D4 callers. Its dormant PATH tracks refer to ordinary
     * empty joints, not splines. Keep its static hierarchy, omit that animation. */
    if(variant==STAGE_BIGBLUE_ROUTE&&
       (melee_stage_models_count(s->models)!=38||!melee_stage_models_omit_unused_joint_animation(s->models,33)))goto fail;
    if(!(s->header=melee_stage_models_header(s->models)))goto fail;
    s->collision=melee_stage_collision_decode(a);s->params=melee_stage_params_decode(a);
    if(!s->collision||!s->params)goto fail;
    u32 colors;if(!melee_archive_find(a,"yakumono_param",&colors))goto fail;
    if(color_count){
        s->colors=melee_stage_colors_decode(a,colors,color_count);if(!s->colors)goto fail;
        for(unsigned i=0;i<color_count;i++){s->hazards[i]=melee_item_colors_entries(s->colors)[i].unk;if(!s->hazards[i])goto fail;}
    }else{
        /* Preserve scalar parameter bits; only Dream Land starts with halfwords. */
        if(colors>a->data_size||4*param_words>a->data_size-colors)goto fail;
        s->hazard_params=malloc(4*param_words);if(!s->hazard_params)goto fail;
        u8* dst=s->hazard_params;
        for(unsigned i=0;i<halfwords;i++){
            const u8* src=a->bytes+32+colors+2*i;u16 bits=((u16)src[0]<<8)|src[1];memcpy(dst+2*i,&bits,2);
        }
        for(unsigned i=halfwords/2;i<param_words;i++){u32 bits;if(!melee_archive_u32(a,colors+4*i,&bits))goto fail;memcpy(dst+4*i,&bits,4);}
    }
    if(variant==STAGE_MAZE){
        struct grShrineRoute_YakumonoParam* p=calloc(1,sizeof(*p));if(!p)goto fail;
        free(s->hazard_params);s->hazard_params=p;
        s->colors=melee_stage_colors_decode(a,colors,4);if(!s->colors)goto fail;
        p->x0=melee_item_colors_entries(s->colors)[0].unk;p->x4=melee_item_colors_entries(s->colors)[1].unk;
        p->x8=melee_item_colors_entries(s->colors)[2].unk;p->xC=melee_item_colors_entries(s->colors)[3].unk;
        u32 hit;if(!melee_archive_pointer(a,colors+16,&hit,&present)||!present)goto fail;
        s->hazard_hit=malloc(sizeof(*p->x10));if(!s->hazard_hit)goto fail;p->x10=s->hazard_hit;
        for(unsigned i=0;i<9;i++){u32 bits;if(!melee_archive_u32(a,hit+4*i,&bits))goto fail;memcpy((u8*)p->x10+4*i,&bits,4);}
        for(unsigned i=0;i<5;i++){
            u32 bits;if(!melee_archive_u32(a,colors+20+4*i,&bits))goto fail;
            if(i<4){float v;memcpy(&v,&bits,4);if(!isfinite(v))goto fail;}
            memcpy((u8*)&p->x14+4*i,&bits,4);
        }
        /* GrNSr stores 64 records; unused generator slots remain zero. */
        if((uint64_t)colors+40+64*4>a->data_size)goto fail;
        for(unsigned i=0;i<64;i++){
            const u8* src=a->bytes+32+colors+40+4*i;
            u8* dst=(u8*)&p->spawn_desc+4*i;u16 kind=((u16)src[0]<<8)|src[1];
            memcpy(dst,&kind,2);dst[2]=src[2];dst[3]=src[3];
        }
    }
    if(variant==STAGE_KINOKO){
        /* 80 spawn records contain a kind halfword and two independent bytes. */
        u8* dst=s->hazard_params;
        for(unsigned i=0;i<80;i++){
            const u8* src=a->bytes+32+colors+4+4*i;
            u16 kind=((u16)src[0]<<8)|src[1];memcpy(dst+4+4*i,&kind,2);
            dst[6+4*i]=src[2];dst[7+4*i]=src[3];
        }
    }
    if(variant==STAGE_TARGET){
        /* Surface-hit descriptors; every descriptor contains nine scalar
         * words (lbColl_80008D30_arg1), not a DynamicsDesc pointer layout. */
        void** pointers=calloc(param_words,sizeof(*pointers));if(!pointers)goto fail;
        free(s->hazard_params);s->hazard_params=pointers;
        s->hazard_hit=calloc(param_words,36);if(!s->hazard_hit)goto fail;
        for(unsigned i=0;i<param_words;i++){
            u32 at;MeleeHostBool found;
            if(!melee_archive_pointer(a,colors+4*i,&at,&found)||!found)goto fail;
            pointers[i]=(u8*)s->hazard_hit+36*i;
            for(unsigned j=0;j<9;j++){
                u32 bits;if(!melee_archive_u32(a,at+4*j,&bits))goto fail;
                memcpy((u8*)pointers[i]+4*j,&bits,4);
            }
        }
    }
    if(variant==STAGE_HOMERUN){
        s->text=melee_sis_bank_decode_table(a,"SIS_GrHomerunData",4);if(!s->text)goto fail;
    }
    if(variant==STAGE_TARGET_EMPTY){
        u32 unused;MeleeHostBool found;
        if(!melee_archive_pointer(a,colors,&unused,&found)||found||unused)goto fail;
        free(s->hazard_params);s->hazard_params=calloc(1,sizeof(void*));
        if(!s->hazard_params)goto fail;
    }
    if(variant==STAGE_PUSHON){
        struct grPushon_YakumonoParam* p=calloc(1,sizeof(*p));if(!p)goto fail;
        u8* raw=s->hazard_params;
        memcpy(&p->x18,raw+0x18,4);
        memcpy(p->x1c,raw+0x1c,sizeof(p->x1c));
        memcpy(p->x10c,raw+0x10c,sizeof(p->x10c));
        free(raw);s->hazard_params=p;
        /* Each entry combines a word and two signed halfwords. */
        for(unsigned i=0;i<30;i++)for(unsigned j=0;j<2;j++){
            const u8* src=a->bytes+32+colors+0x1c+8*i+4+2*j;
            u16 bits=((u16)src[0]<<8)|src[1];
            memcpy((u8*)&p->x1c[i]+4+2*j,&bits,2);
        }
        s->hazard_hit=calloc(6,36);if(!s->hazard_hit)goto fail;
        p->x0=s->hazard_hit;
        p->x4=(DynamicsDesc*)((u8*)s->hazard_hit+36);
        p->x8=(DynamicsDesc*)((u8*)s->hazard_hit+72);
        p->xC=(DynamicsDesc*)((u8*)s->hazard_hit+108);
        p->x10=(DynamicsDesc*)((u8*)s->hazard_hit+144);
        p->x14=(DynamicsDesc*)((u8*)s->hazard_hit+180);
        /* Surface callbacks consume nine-word hit records, despite their
         * historical DynamicsDesc return type. */
        for(unsigned i=0;i<6;i++){
            u32 at;MeleeHostBool found;
            if(!melee_archive_pointer(a,colors+4*i,&at,&found)||!found)goto fail;
            for(unsigned j=0;j<9;j++){
                u32 bits;if(!melee_archive_u32(a,at+4*j,&bits))goto fail;
                memcpy((u8*)s->hazard_hit+36*i+4*j,&bits,4);
            }
        }
    }
    if(stage_item==It_Kind_Klap){
        for(unsigned i=0;i<8;i++){
            const u8* src=a->bytes+32+colors+0x44+2*i;
            u16 bits=((u16)src[0]<<8)|src[1];memcpy((u8*)s->hazard_params+0x44+2*i,&bits,2);
        }
    }
    if(stage_item==It_Kind_Klap){
        s->colors=melee_stage_colors_decode(a,colors+0x84,1);if(!s->colors)goto fail;
        struct grKongo_YakumonoParam* native=calloc(1,sizeof(*native));if(!native)goto fail;
        memcpy(native,s->hazard_params,0x84);
        native->unk84=melee_item_colors_entries(s->colors)[0].unk;
        memcpy(&native->unk88,(u8*)s->hazard_params+0x88,0x34);
        free(s->hazard_params);s->hazard_params=native;
    }
    if(stage_item==It_Kind_Tincle){
        /* Great Bay mixes signed halfword timers/item weights with floats. */
        const unsigned ranges[][2]={{0,2},{0x44,4},{0x70,4},{0x7c,20}};
        for(unsigned i=0;i<4;i++)for(unsigned j=0;j<ranges[i][1];j++){
            unsigned offset=ranges[i][0]+2*j;
            const u8* src=a->bytes+32+colors+offset;
            u16 bits=((u16)src[0]<<8)|src[1];memcpy((u8*)s->hazard_params+offset,&bits,2);
        }
    }
    if(variant==STAGE_INISHIE2){
        for(unsigned i=0;i<12;i++){
            unsigned off=i<10?2*i:0x48+2*(i-10);const u8* q=a->bytes+32+colors+off;
            u16 v=((u16)q[0]<<8)|q[1];memcpy((u8*)s->hazard_params+off,&v,2);
        }
    }
    if(variant==STAGE_INISHIE1){
        for(unsigned i=0;i<6;i++){
            unsigned off=0x14+2*i;const u8* q=a->bytes+32+colors+off;
            u16 v=((u16)q[0]<<8)|q[1];memcpy((u8*)s->hazard_params+off,&v,2);
        }
    }
    if(variant==STAGE_ICEMT){
        struct grIceMt_YakumonoParam* p=calloc(1,sizeof(*p));if(!p)goto fail;
        memcpy(p,s->hazard_params,0xac);
        free(s->hazard_params);s->hazard_params=p;
        const unsigned ranges[][2]={{0,4},{0x34,4},{0x98,2},{0xa4,4}};
        for(unsigned i=0;i<4;i++)for(unsigned j=0;j<ranges[i][1];j++){
            unsigned off=ranges[i][0]+2*j;const u8* q=a->bytes+32+colors+off;
            u16 v=((u16)q[0]<<8)|q[1];memcpy((u8*)p+off,&v,2);
        }
        s->hazard_hit=calloc(40,sizeof(s16));if(!s->hazard_hit)goto fail;
        s16** arrays[]={&p->field_ixs,&p->xB0,&p->xB4};
        const unsigned counts[]={16,12,12};unsigned used=0;
        for(unsigned i=0;i<3;i++){
            u32 at;MeleeHostBool found;
            if(!melee_archive_pointer(a,colors+0xac+4*i,&at,&found)||!found||
               at>a->data_size||counts[i]*2>a->data_size-at)goto fail;
            *arrays[i]=(s16*)s->hazard_hit+used;used+=counts[i];
            for(unsigned j=0;j<counts[i];j++){
                const u8* q=a->bytes+32+at+2*j;u16 v=((u16)q[0]<<8)|q[1];memcpy(*arrays[i]+j,&v,2);
            }
            if((*arrays[i])[counts[i]-1]!=-1)goto fail;
        }
        for(unsigned i=0;i<2;i++){
            const u8* q=a->bytes+32+colors+0xb8+2*i;u16 v=((u16)q[0]<<8)|q[1];memcpy((u8*)&p->xB8+2*i,&v,2);
        }
        for(unsigned i=0;i<32;i++){
            const u8* q=a->bytes+32+colors+0xbc+4*i;
            p->spawn[i].kind=((u16)q[0]<<8)|q[1];p->spawn[i].x2=q[2];p->spawn[i].respawn=q[3];
        }
    }
    if(variant==STAGE_FOURSIDE){
        /* Three UFO probability/count halfwords and trailing padding. */
        for(unsigned i=0;i<4;i++){
            unsigned offset=0x44+2*i;const u8* src=a->bytes+32+colors+offset;
            u16 bits=((u16)src[0]<<8)|src[1];memcpy((u8*)s->hazard_params+offset,&bits,2);
        }
    }
    if(variant==STAGE_CASTLE){
        /* Castle timers and entry IDs are signed halfwords amid float words. */
        const unsigned ranges[][2]={{0,8},{0x40,4},{0x54,4},{0x12C,4}};
        for(unsigned r=0;r<4;r++)for(unsigned j=0;j<ranges[r][1];j++){
            unsigned at=ranges[r][0]+2*j;const u8* src=a->bytes+32+colors+at;
            u16 value=((u16)src[0]<<8)|src[1];memcpy((u8*)s->hazard_params+at,&value,2);
        }
        for(unsigned j=0;j<9;j++)for(unsigned k=0;k<2;k++){
            unsigned at=0x5C+20*j+2*k;const u8* src=a->bytes+32+colors+at;
            u16 value=((u16)src[0]<<8)|src[1];memcpy((u8*)s->hazard_params+at,&value,2);
        }
        s->colors=melee_stage_colors_decode(a,colors+0x114,1);if(!s->colors)goto fail;
        struct grCastle_YakumonoParam* native=calloc(1,sizeof(*native));if(!native)goto fail;
        memcpy(native,s->hazard_params,0x114);
        native->x114=melee_item_colors_entries(s->colors)[0].unk;
        memcpy(&native->x118,(u8*)s->hazard_params+0x118,0x2C);
        free(s->hazard_params);s->hazard_params=native;
        const char* names[]={"dynamicsdata_flag3","dynamicsdata_flag4","dynamicsdata_flag6"};
        const unsigned counts[]={3,4,6};
        for(unsigned j=0;j<3;j++){
            u32 at,params,count;
            if(!melee_archive_find(a,names[j],&at)||!melee_archive_pointer(a,at,&params,&present)||!present||
               !melee_archive_u32(a,at+4,&count)||count!=counts[j])goto fail;
            s->flags[j].count=count;
            s->flags[j].params=calloc(count,sizeof(*s->flags[j].params));if(!s->flags[j].params)goto fail;
            for(unsigned k=0;k<3;k++){float f;if(!melee_archive_f32(a,at+8+4*k,&f)||!isfinite(f))goto fail;memcpy((u8*)&s->flags[j].pos+4*k,&f,4);}
            for(unsigned k=0;k<count*15;k++){float f;if(!melee_archive_f32(a,params+4*k,&f)||!isfinite(f))goto fail;memcpy((u8*)s->flags[j].params+4*k,&f,4);}
        }
    }
    if(variant==STAGE_RCRUISE){
        u32 at,params,count;
        if(!melee_archive_find(a,"dynamicsdata_shipflag",&at)||!melee_archive_pointer(a,at,&params,&present)||!present||
           !melee_archive_u32(a,at+4,&count)||count!=6)goto fail;
        DynamicsDesc* flag=&s->flags[3];flag->count=count;
        flag->params=calloc(count,sizeof(*flag->params));if(!flag->params)goto fail;
        for(unsigned k=0;k<3;k++){float f;if(!melee_archive_f32(a,at+8+4*k,&f)||!isfinite(f))goto fail;memcpy((u8*)&flag->pos+4*k,&f,4);}
        for(unsigned k=0;k<count*15;k++){float f;if(!melee_archive_f32(a,params+4*k,&f)||!isfinite(f))goto fail;memcpy((u8*)flag->params+4*k,&f,4);}
    }
    if(variant==STAGE_BRINSTAR){
        for(unsigned i=0;i<120;i++){
            const u8* src=a->bytes+32+colors+0xA0+2*i;
            u16 bits=((u16)src[0]<<8)|src[1];memcpy((u8*)s->hazard_params+0xA0+2*i,&bits,2);
        }
        u32 hit;if(!melee_archive_pointer(a,colors+0x2C,&hit,&present)||!present)goto fail;
        s->hazard_hit=malloc(36);if(!s->hazard_hit)goto fail;
        for(unsigned i=0;i<9;i++){u32 word;if(!melee_archive_u32(a,hit+4*i,&word))goto fail;memcpy((u8*)s->hazard_hit+4*i,&word,4);}
        grZe_YakumonoParam* native=calloc(1,sizeof(*native));if(!native)goto fail;
        memcpy(native,s->hazard_params,0x2C);native->acid_hit=s->hazard_hit;
        memcpy(&native->x30,(u8*)s->hazard_params+0x30,0x160);
        free(s->hazard_params);s->hazard_params=native;
    }
    if(variant==STAGE_MUTECITY){
        s->colors=melee_stage_colors_decode(a,colors,2);if(!s->colors)goto fail;
        s->hazard_hit=malloc(72);if(!s->hazard_hit)goto fail;
        for(unsigned j=0;j<2;j++){
            u32 hit;if(!melee_archive_pointer(a,colors+8+4*j,&hit,&present)||!present)goto fail;
            for(unsigned i=0;i<9;i++){u32 word;if(!melee_archive_u32(a,hit+4*i,&word))goto fail;memcpy((u8*)s->hazard_hit+36*j+4*i,&word,4);}
        }
        struct grMc_YakumonoParam* native=calloc(1,sizeof(*native));if(!native)goto fail;
        native->x0=melee_item_colors_entries(s->colors)[0].unk;
        native->x4=melee_item_colors_entries(s->colors)[1].unk;
        native->x8=s->hazard_hit;native->xC=(void*)((u8*)s->hazard_hit+36);
        memcpy(native->pad10,(u8*)s->hazard_params+16,64);
        free(s->hazard_params);s->hazard_params=native;
    }
    if(variant==STAGE_CORNERIA){
        s->colors=melee_stage_colors_decode(a,colors+0x84,1);if(!s->colors)goto fail;
        struct grCorneria_YakumonoParam* native=calloc(1,sizeof(*native));if(!native)goto fail;
        memcpy(native,s->hazard_params,0x84);native->x84=melee_item_colors_entries(s->colors)[0].unk;
        memcpy(&native->x88,(u8*)s->hazard_params+0x88,4);
        free(s->hazard_params);s->hazard_params=native;
        s->text=melee_sis_bank_decode_table(a,"SIS_GrCorneriaData",53);if(!s->text)goto fail;
    }
    if(variant==STAGE_VENOM){
        s->colors=melee_stage_colors_decode(a,colors+0x38,1);if(!s->colors)goto fail;
        struct grVenom_YakumonoParam* native=calloc(1,sizeof(*native));if(!native)goto fail;
        memcpy(native,s->hazard_params,0x38);native->x38=melee_item_colors_entries(s->colors)[0].unk;
        free(s->hazard_params);s->hazard_params=native;
        s->text=melee_sis_bank_decode_table(a,"SIS_GrCorneriaData",53);if(!s->text)goto fail;
    }
    if(stadium){
        u8* dst=s->hazard_params;const u8* src=a->bytes+32+colors;
        memcpy(dst+28,src+28,4); // RGBA bytes, not a scalar word.
        for(unsigned i=0;i<5;i++){u16 bits=((u16)src[72+2*i]<<8)|src[73+2*i];memcpy(dst+72+2*i,&bits,2);}
        memcpy(dst+82,src+82,2);
        if(stadium==1){s->text=melee_sis_bank_decode_table(a,"SIS_GrPStadiumData",22);if(!s->text)goto fail;}
        if(stadium==2){
            u32 unused;
            const char* absent[]={"map_ptcl","map_texg","map_plit","quake_model_set","SIS_GrPStadiumData"};
            for(unsigned i=0;i<5;i++)if(melee_archive_find(a,absent[i],&unused))goto fail;
            s->scripts=melee_item_scripts_decode(a);if(!s->scripts)goto fail;
            s->bridge.flags=HSD_ARCHIVE_NATIVE;s->bridge.native_public_lookup=lookup;s->bridge.native_destroy=destroy;return &s->bridge;
        }
    }
    s->quake=melee_dynamic_model_decode(a,"quake_model_set");s->scripts=melee_item_scripts_decode(a);
    if(!s->quake||!s->scripts)goto fail;
    if(variant==STAGE_TARGET||variant==STAGE_TARGET_EMPTY){
        MeleeHostBool has_cmd=melee_archive_find(a,"map_ptcl",&cmd),has_tex=melee_archive_find(a,"map_texg",&tex);
        if(has_cmd!=has_tex)goto fail;
        if(has_cmd){
            if(cmd>=tex)goto fail;
            s->particles=melee_particle_bank_decode(a,cmd,tex-cmd,tex,a->data_size-tex);
            if(!s->particles)goto fail;
        }
    }else if(variant==STAGE_HOMERUN||variant==STAGE_INISHIE2||variant==STAGE_INISHIE1||variant==STAGE_HEAL||variant==STAGE_BIGBLUE_ROUTE||variant==STAGE_ZEBES_ROUTE||variant==STAGE_YORSTER||variant==STAGE_SHRINE||variant==STAGE_VENOM||variant==STAGE_MUTECITY||variant==STAGE_BIGBLUE||variant==STAGE_FOURSIDE||variant==STAGE_FLATZONE||variant==STAGE_RCRUISE||variant==STAGE_PURA||variant==STAGE_TARGET||variant==STAGE_FIGUREGET||variant==STAGE_PUSHON){
        if(melee_archive_find(a,"map_ptcl",&cmd)||melee_archive_find(a,"map_texg",&tex))goto fail;
    }else{
        if(!melee_archive_find(a,"map_ptcl",&cmd)||!melee_archive_find(a,"map_texg",&tex)||cmd>=tex)goto fail;
        s->particles=melee_particle_bank_decode(a,cmd,tex-cmd,tex,a->data_size-tex);if(!s->particles)goto fail;
    }
    if(!melee_archive_find(a,"map_plit",&light))goto fail;
    s->lighting=melee_environment_decode(a,NONE,light,NONE);if(!s->lighting)goto fail;
    LightList** list=melee_environment_lights(s->lighting);
    for(unsigned i=0;list&&list[i];i++){
        u32 entry,desc;if(!melee_archive_pointer(a,light+4*i,&entry,&present)||!present||!melee_archive_pointer(a,entry,&desc,&present)||!present)goto fail;
        HSD_LightDesc* canonical=melee_stage_models_find_light(s->models,desc);if(!canonical)goto fail;
        list[i]->desc=canonical;
        if(list[i]->anims){
            u32 table;if(!melee_archive_pointer(a,entry+4,&table,&present)||!present)goto fail;
            for(unsigned j=0;list[i]->anims[j];j++){
                u32 at;if(!melee_archive_pointer(a,table+4*j,&at,&present)||!present)goto fail;
                HSD_LightAnim* anim=melee_stage_models_find_light_animation(s->models,at);
                /* Particle lights may own animation records absent from model lists. */
                if(anim)list[i]->anims[j]=anim;
            }
        }
    }
    s->bridge.flags=HSD_ARCHIVE_NATIVE;s->bridge.native_public_lookup=lookup;s->bridge.native_destroy=destroy;return &s->bridge;
fail:destroy(&s->bridge);return NULL;
}
MeleeHostBool melee_battle_stage_register_particles(HSD_Archive* b,int bank){
    if(!b||b->native_public_lookup!=lookup)return false;BattleStage* s=b->top_ptr;
    psInitDataBankNative(bank,melee_particle_bank_commands(s->particles),melee_particle_bank_command_count(s->particles),
        melee_particle_bank_textures(s->particles),melee_particle_bank_texture_count(s->particles));
    return true;
}

HSD_Archive* melee_battle_stage_decode(const MeleeArchive* a){return decode(a,2,0,0,false,0,STAGE_SCALARS);}
HSD_Archive* melee_final_stage_decode(const MeleeArchive* a){return decode(a,4,0,0,false,0,STAGE_SCALARS);}

HSD_Archive* melee_dreamland_stage_decode(const MeleeArchive* a){return decode(a,0,13,4,false,0,STAGE_SCALARS);}

HSD_Archive* melee_fountain_stage_decode(const MeleeArchive* a){return decode(a,0,21,0,false,0,STAGE_SCALARS);}

HSD_Archive* melee_story_stage_decode(const MeleeArchive* a){return decode(a,0,9,0,It_Kind_Heiho,0,STAGE_SCALARS);}

HSD_Archive* melee_stadium_stage_decode(const MeleeArchive* a,MeleeHostBool transformation){return decode(a,0,21,0,false,transformation?2:1,STAGE_SCALARS);}

HSD_Archive* melee_greatbay_stage_decode(const MeleeArchive* a){return decode(a,0,41,0,It_Kind_Tincle,0,STAGE_SCALARS);}

HSD_Archive* melee_kongo_stage_decode(const MeleeArchive* a){return decode(a,0,47,0,It_Kind_Klap,0,STAGE_SCALARS);}

HSD_Archive* melee_japes_stage_decode(const MeleeArchive* a){return decode(a,0,8,0,0,0,STAGE_SCALARS);}

HSD_Archive* melee_brinstar_stage_decode(const MeleeArchive* a){return decode(a,0,100,0,0,0,STAGE_BRINSTAR);}

HSD_Archive* melee_castle_stage_decode(const MeleeArchive* a){return decode(a,0,81,0,0,0,STAGE_CASTLE);}

HSD_Archive* melee_yorster_stage_decode(const MeleeArchive* a){return decode(a,0,8,0,0,0,STAGE_YORSTER);}

HSD_Archive* melee_greens_stage_decode(const MeleeArchive* a){return decode(a,0,31,0,It_Kind_WhispyApple,0,STAGE_GREENS);}

HSD_Archive* melee_shrine_stage_decode(const MeleeArchive* a){return decode(a,0,1,0,0,0,STAGE_SHRINE);}

HSD_Archive* melee_venom_stage_decode(const MeleeArchive* a){return decode(a,0,15,0,It_Kind_Arwing_Laser,0,STAGE_VENOM);}

HSD_Archive* melee_corneria_stage_decode(const MeleeArchive* a){return decode(a,0,35,0,It_Kind_Arwing_Laser,0,STAGE_CORNERIA);}

HSD_Archive* melee_mutecity_stage_decode(const MeleeArchive* a){return decode(a,0,20,0,0,0,STAGE_MUTECITY);}

HSD_Archive* melee_bigblue_stage_decode(const MeleeArchive* a){return decode(a,0,81,0,0,0,STAGE_BIGBLUE);}

HSD_Archive* melee_fourside_stage_decode(const MeleeArchive* a){return decode(a,0,19,0,0,0,STAGE_FOURSIDE);}

HSD_Archive* melee_icemt_stage_decode(const MeleeArchive* a){return decode(a,0,79,0,It_Kind_Whitebea,0,STAGE_ICEMT);}

HSD_Archive* melee_flatzone_stage_decode(const MeleeArchive* a){return decode(a,0,16,0,It_Kind_Tools,0,STAGE_FLATZONE);}

HSD_Archive* melee_kraid_stage_decode(const MeleeArchive* a){return decode(a,0,13,0,0,0,STAGE_SCALARS);}

HSD_Archive* melee_rcruise_stage_decode(const MeleeArchive* a){return decode(a,0,18,0,0,0,STAGE_RCRUISE);}

HSD_Archive* melee_pura_stage_decode(const MeleeArchive* a){return decode(a,0,1,0,0,0,STAGE_PURA);}

HSD_Archive* melee_target_fox_stage_decode(const MeleeArchive* a){return decode(a,0,4,0,It_Kind_Mato,0,STAGE_TARGET);}

HSD_Archive* melee_figureget_stage_decode(const MeleeArchive* a){return decode(a,0,6,0,0,0,STAGE_FIGUREGET);}

HSD_Archive* melee_pushon_stage_decode(const MeleeArchive* a){return decode(a,0,0x214/4,0,0,0,STAGE_PUSHON);}

HSD_Archive* melee_kinoko_route_stage_decode(const MeleeArchive* a){return decode(a,0,81,0,0,0,STAGE_KINOKO);}

HSD_Archive* melee_maze_stage_decode(const MeleeArchive* a){return decode(a,0,74,0,It_Kind_Likelike,0,STAGE_MAZE);}

HSD_Archive* melee_zebes_route_stage_decode(const MeleeArchive* a){return decode(a,0,2,0,0,0,STAGE_ZEBES_ROUTE);}

HSD_Archive* melee_bigblue_route_stage_decode(const MeleeArchive* a){return decode(a,0,20,0,0,0,STAGE_BIGBLUE_ROUTE);}

HSD_Archive* melee_heal_stage_decode(const MeleeArchive* a){return decode(a,0,2,0,0,0,STAGE_HEAL);}

HSD_Archive* melee_inishie1_stage_decode(const MeleeArchive* a){return decode(a,0,21,0,0,0,STAGE_INISHIE1);}

HSD_Archive* melee_inishie2_stage_decode(const MeleeArchive* a){return decode(a,0,19,0,0,0,STAGE_INISHIE2);}

HSD_Archive* melee_target_stage_decode(const MeleeArchive* a,const char* filename)
{
    if(!filename)return NULL;
    const char* name=strrchr(filename,'/');name=name?name+1:filename;
    static const char* const names[]={"Ca","Cl","Dk","Dr","Fc","Fe","Fx","Gn","Gw","Ic","Kb","Kp","Lg","Lk","Mr","Ms","Mt","Ns","Pc","Pe","Pk","Pr","Sk","Ss","Ys","Zd"};
    if(strlen(name)!=9||strncmp(name,"GrT",3)||strcmp(name+5,".dat"))return NULL;
    for(unsigned i=0;i<sizeof(names)/sizeof(*names);i++)if(!strncmp(name+3,names[i],2)){
        unsigned count=!strncmp(name+3,"Mt",2)?8:(!strncmp(name+3,"Fx",2)||!strncmp(name+3,"Fc",2))?4:!strncmp(name+3,"Gn",2)?3:!strncmp(name+3,"Pr",2)?1:0;
        return decode(a,0,count?count:1,0,It_Kind_Mato,0,count?STAGE_TARGET:STAGE_TARGET_EMPTY);
    }
    return NULL;
}

HSD_Archive* melee_homerun_stage_decode(const MeleeArchive* a){return decode(a,0,1,0,0,0,STAGE_HOMERUN);}
