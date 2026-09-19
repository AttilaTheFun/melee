#include "melee_item_article.h"
#include "melee_item_attributes.h"
#include "melee_item_hurtbones.h"
#include "melee_item_dynamics.h"
#include "melee_item_model.h"
#include "melee_item_animation.h"
#include "melee_item_scripts.h"
#include "melee_item_special.h"
#include "melee_item_foods.h"
#include "melee_item_wstar.h"
#include "melee_item_mushroom.h"
#include "melee_item_yoyo.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <melee/it/itCommonItems.h>
#include <melee/it/kinds/itarwinglaser.h>
#define NONE UINT32_MAX
struct MeleeItemArticle {
    it_266F_ItemVars gamewatch_parts;u8* gamewatch_indices[2];
    struct {s32 hp;f32 speed,fall_speed,terminal_speed;s32 value;} whitebear_common;
    union {u32 words[5];Vec3 vector;S32Vec3 integers;itLeadeadAttr_x0 leadead;} likelike_common;
    itNokoNoko_DatAttrs2 noko_common;itPatapataDatAttrs pata_common;
    ItemStateDesc match_coin_states[3];
    Article article;ItemAttr attr;ItemAttr laser_attr;ItemStateArray states;unsigned count;
    MeleeItemModel* model;MeleeScene* variants[26];MeleeScene* animations[64];MeleeItemScript* scripts[64];
    MeleeItemYoyo* yoyo;void* special;MeleeItemFoods* foods;MeleeItemWstar* wstar;MeleeItemMushroom* mushroom;
    HSD_AnimJoint* grapple_anim[5];HSD_MatAnimJoint* grapple_mat[5];HSD_ShapeAnimJoint* grapple_shape[5];
};
Article* melee_item_article_descriptor(MeleeItemArticle* a){return a?&a->article:NULL;}
void melee_item_article_free(MeleeItemArticle* a){
    if(!a)return;for(unsigned i=0;i<2;i++)free(a->gamewatch_indices[i]);for(unsigned i=0;i<64;i++){melee_scene_free(a->animations[i]);melee_item_script_free(a->scripts[i]);}
    for(unsigned i=0;i<26;i++)melee_scene_free(a->variants[i]);
    melee_item_yoyo_free(a->yoyo);melee_item_foods_free(a->foods);melee_item_wstar_free(a->wstar);melee_item_mushroom_free(a->mushroom);free(a->special);
    melee_item_hurtbones_free(a->article.x8_hurtbones);melee_item_dynamics_free(a->article.x14_dynamics);
    melee_item_model_free(a->model);free(a->states.x0_itemStateDesc);free(a);
}
static int ref(const MeleeArchive* a,u32 at,u32* v){MeleeHostBool present;if(!melee_archive_pointer(a,at,v,&present))return 0;if(!present)*v=NONE;return 1;}
MeleeItemArticle* melee_item_article_decode(const MeleeArchive* input,unsigned kind,uint32_t at,unsigned count){
#define ARTICLE_FAIL() do { fprintf(stderr,"Item article kind %u conversion failed at line %d\n",kind,__LINE__); goto fail; } while(0)
    const int gamewatch=(kind>=It_Kind_GameWatch_Greenhouse&&kind<=It_Kind_GameWatch_Chef)||kind==It_Kind_GameWatch_Rescue||kind==It_Kind_Kirby_GameWatchChef||kind==It_Kind_Kirby_GameWatchChefPan;
    const int no_special=kind==It_Kind_Octarock_Stone||kind==It_Kind_Kirby_Hammer||kind==It_Kind_Unk2||kind==It_Kind_Peach_Explode||kind==It_PKind_Porygon2||kind==It_Kind_Yoshi_EggLay||kind==It_Kind_Kirby_YoshiEggLay||kind==It_Kind_Seak_Vanish;
    if(!input||(kind>42&&kind!=It_Kind_Unk4&&kind!=It_Kind_Ness_Yoyo&&kind!=It_Kind_Kuriboh&&kind!=It_Kind_Kyasarin&&kind!=It_Kind_Kyasarin_Egg&&kind!=It_Kind_Ottosea&&kind!=It_Kind_Leadead&&kind!=It_Kind_Octarock&&kind!=It_Kind_Likelike&&kind!=It_Kind_Nokonoko&&kind!=It_Kind_Patapata&&kind!=It_Kind_Whitebea&&kind!=It_Kind_Mato&&!no_special&&!gamewatch&&kind!=It_Kind_Arwing_Laser&&kind!=It_Kind_Heiho&&kind!=It_Kind_Klap&&kind!=It_Kind_WhispyApple&&kind!=It_Kind_WhispyHealApple&&!melee_item_special_size(kind))||count>64||(uint64_t)at+24>input->data_size)return NULL;
    size_t size;u8* bytes=melee_archive_copy_null_externals(input,&size);if(!bytes)return NULL;
    MeleeArchive archive={0};MeleeItemArticle* a=NULL;
    if(!melee_archive_adopt(&archive,bytes,size)){free(bytes);return NULL;}
    const MeleeArchive* src=&archive;u32 r[6];for(unsigned i=0;i<6;i++)if(!ref(src,at+4*i,r+i))ARTICLE_FAIL();
    if(r[0]==NONE||(r[1]==NONE)!=no_special||r[4]==NONE||(r[3]==NONE)!=(count==0))ARTICLE_FAIL();
    a=calloc(1,sizeof(*a));if(!a)ARTICLE_FAIL();a->count=count;
    if(!melee_item_attributes_decode(src,r[0],&a->attr))ARTICLE_FAIL();a->article.x0_common_attr=&a->attr;
    a->model=melee_item_model_decode(src,r[4]);if(!a->model)ARTICLE_FAIL();a->article.x10_modelDesc=melee_item_model_descriptor(a->model);
    unsigned bones=a->article.x10_modelDesc->x4_bone_count;
    if(r[2]!=NONE&&!(a->article.x8_hurtbones=melee_item_hurtbones_decode(src,r[2],bones)))ARTICLE_FAIL();
    if(r[5]!=NONE&&!(a->article.x14_dynamics=melee_item_dynamics_decode(src,r[5],bones)))ARTICLE_FAIL();
    u32 joint;if(!ref(src,r[4],&joint))ARTICLE_FAIL();
    if(kind==It_Kind_Ness_Yoyo){a->yoyo=melee_item_yoyo_decode(src,r[1]);if(!a->yoyo)ARTICLE_FAIL();a->article.x4_specialAttributes=melee_item_yoyo_attributes(a->yoyo);}
    else if(kind==It_Kind_Unk4){
        it_2E5A_Attrs* attrs=calloc(1,sizeof(*attrs));if(!attrs)ARTICLE_FAIL();a->special=attrs;
        for(unsigned i=0;i<15;i++){
            float v;if(!melee_archive_f32(src,r[1]+4*i,&v)||!isfinite(v))ARTICLE_FAIL();
            memcpy((u8*)attrs+4*i,&v,4);
        }
        for(unsigned i=0;i<3;i++){
            u32 at=r[1]+60+44*i,model,bits;
            it_2E5A_TierEntry* t=&attrs->tiers[i];
            if(!ref(src,at,&model)||model==NONE)ARTICLE_FAIL();
            a->variants[i]=melee_item_animation_decode(src,model,at+4);if(!a->variants[i])ARTICLE_FAIL();
            t->joint=melee_scene_joint_descriptor(a->variants[i]);
            t->anim_joint=melee_scene_animation_descriptor(a->variants[i]);
            t->matanim_joint=melee_scene_material_descriptor(a->variants[i]);
            t->shape_anim_joint=melee_scene_shape_descriptor(a->variants[i]);
            if(!melee_archive_u32(src,at+16,&bits))ARTICLE_FAIL();memcpy(&t->xD84_value,&bits,4);
            if(!melee_archive_u32(src,at+20,&bits))ARTICLE_FAIL();memcpy(&t->threshold,&bits,4);
            if(t->threshold<=0||!melee_archive_f32(src,at+24,&t->scale)||!isfinite(t->scale)||t->scale<=0)ARTICLE_FAIL();
            for(unsigned j=0;j<4;j++){float v;if(!melee_archive_f32(src,at+28+4*j,&v)||!isfinite(v))ARTICLE_FAIL();memcpy((u8*)&t->ecb+4*j,&v,4);}
            a->match_coin_states[i]=(ItemStateDesc){t->anim_joint,t->matanim_joint,t->shape_anim_joint,NULL};
            t->native_state=&a->match_coin_states[i];
            melee_scene_release_objects(a->variants[i]);
        }
        a->article.x4_specialAttributes=attrs;
    }
    else if(kind==18){a->foods=melee_item_foods_decode(src,r[1]);if(!a->foods)ARTICLE_FAIL();a->article.x4_specialAttributes=melee_item_foods_attributes(a->foods);}
    else if(kind==29){a->wstar=melee_item_wstar_decode(src,r[1],joint);if(!a->wstar)ARTICLE_FAIL();a->article.x4_specialAttributes=melee_item_wstar_attributes(a->wstar);}
    else if(kind==26||kind==27){a->mushroom=melee_item_mushroom_decode(src,r[1],joint);if(!a->mushroom)ARTICLE_FAIL();a->article.x4_specialAttributes=melee_item_mushroom_attributes(a->mushroom);}
    else if(kind==It_Kind_Mato){
        /* Target's special record is a common-data pointer followed by zero
         * reserved words. Own both records instead of retaining DAT offsets. */
        u32 common,bits;void** attrs=calloc(1,sizeof(void*)+20);
        if(!attrs)ARTICLE_FAIL();a->special=attrs;
        if(!ref(src,r[1],&common)||common==NONE)ARTICLE_FAIL();
        for(unsigned i=0;i<5;i++){
            if(!melee_archive_u32(src,common+4*i,&bits))ARTICLE_FAIL();
            memcpy((u8*)&a->whitebear_common+4*i,&bits,4);
            if(!melee_archive_u32(src,r[1]+4+4*i,&bits)||bits)ARTICLE_FAIL();
        }
        *attrs=&a->whitebear_common;a->article.x4_specialAttributes=attrs;
    }
    else if(kind==It_Kind_Whitebea){
        u32 common,bits;itWhiteBeaAttributes* attrs=calloc(1,sizeof(*attrs));
        if(!attrs)ARTICLE_FAIL();a->special=attrs;
        if((uint64_t)r[1]+24>src->data_size||!ref(src,r[1],&common)||common==NONE||
           !melee_archive_u32(src,common,&bits))ARTICLE_FAIL();
        memcpy(&a->whitebear_common.hp,&bits,4);
        for(unsigned i=0;i<3;i++){
            float f;if(!melee_archive_f32(src,common+4+4*i,&f)||!isfinite(f))ARTICLE_FAIL();
            memcpy((u8*)&a->whitebear_common.speed+4*i,&f,4);
        }
        if(!melee_archive_u32(src,common+16,&bits))ARTICLE_FAIL();
        memcpy(&a->whitebear_common.value,&bits,4);
        attrs->x0=(void*)&a->whitebear_common;
        if(!melee_archive_f32(src,r[1]+4,&attrs->x4)||!isfinite(attrs->x4)||
           !melee_archive_f32(src,r[1]+16,&attrs->x10)||!isfinite(attrs->x10))ARTICLE_FAIL();
        s16* fields[]={&attrs->x8,&attrs->xA,&attrs->xC,&attrs->xE,&attrs->x14};
        for(unsigned i=0;i<5;i++){
            unsigned offset=i<4?8+2*i:20;const u8* p=src->bytes+32+r[1]+offset;
            u16 v=((u16)p[0]<<8)|p[1];memcpy(fields[i],&v,2);
        }
        a->article.x4_specialAttributes=attrs;
    }
    else if(gamewatch){
        u32 parts;if(!ref(src,r[1],&parts)||parts==NONE)ARTICLE_FAIL();
        for(unsigned i=0;i<2;i++){
            u32 packed,indices;if(!melee_archive_u32(src,parts+8*i,&packed)||(packed&65535)||!ref(src,parts+8*i+4,&indices)||indices==NONE)ARTICLE_FAIL();
            unsigned n=packed>>16;if(!n||n>256||indices>src->data_size||n>src->data_size-indices)ARTICLE_FAIL();
            a->gamewatch_indices[i]=malloc(n);if(!a->gamewatch_indices[i])ARTICLE_FAIL();
            memcpy(a->gamewatch_indices[i],src->bytes+32+indices,n);
            if(i){a->gamewatch_parts.x8=n;a->gamewatch_parts.xC=a->gamewatch_indices[i];}
            else{a->gamewatch_parts.x0=n;a->gamewatch_parts.x4=a->gamewatch_indices[i];}
        }
        if(kind==It_Kind_GameWatch_Chef||kind==It_Kind_Kirby_GameWatchChef){
            itGamewatchchefAttributes* attrs=calloc(1,sizeof(*attrs));if(!attrs)ARTICLE_FAIL();a->special=attrs;
            attrs->x0=&a->gamewatch_parts;
            for(unsigned i=0;i<28;i++){float value;if(!melee_archive_f32(src,r[1]+4+4*i,&value)||!isfinite(value))ARTICLE_FAIL();memcpy((u8*)&attrs->x4+4*i,&value,4);}
        }else{
            void** attrs=malloc(sizeof(*attrs));if(!attrs)ARTICLE_FAIL();a->special=attrs;*attrs=&a->gamewatch_parts;
        }
        a->article.x4_specialAttributes=a->special;
    }
    else if(kind==It_Kind_Kirby_LinkArrow||kind==It_Kind_Kirby_CLinkArrow||kind==It_Kind_Link_Arrow||kind==It_Kind_CLink_Arrow||kind==It_Kind_Link_HShot||kind==It_Kind_CLink_HShot){
        const int arrow=kind==It_Kind_Kirby_LinkArrow||kind==It_Kind_Kirby_CLinkArrow||kind==It_Kind_Link_Arrow||kind==It_Kind_CLink_Arrow;
        unsigned prefix_size=arrow?36:84,models=arrow?2:3;
        if((uint64_t)r[1]+prefix_size+models*4>src->data_size)ARTICLE_FAIL();
        void* prefix=melee_item_special_decode(src,kind,r[1]);if(!prefix)ARTICLE_FAIL();
        a->special=calloc(1,arrow?sizeof(itLinkArrowAttributes):sizeof(itLinkHookshotAttributes));
        if(!a->special){free(prefix);ARTICLE_FAIL();}
        memcpy(a->special,prefix,prefix_size);free(prefix);
        for(unsigned i=0;i<models;i++){
            u32 model;if(!ref(src,r[1]+prefix_size+4*i,&model)||model==NONE)ARTICLE_FAIL();
            a->variants[i]=melee_scene_decode(src,model);if(!a->variants[i])ARTICLE_FAIL();
            HSD_Joint* descriptor=melee_scene_joint_descriptor(a->variants[i]);
            if(arrow){itLinkArrowAttributes* attrs=a->special;if(i)attrs->x28=descriptor;else attrs->x24=descriptor;}
            else {itLinkHookshotAttributes* attrs=a->special;if(i==0)attrs->x54=descriptor;else if(i==1)attrs->x58=descriptor;else attrs->x5C=descriptor;}
            melee_scene_release_objects(a->variants[i]);
        }
        a->article.x4_specialAttributes=a->special;
    }
    else if(kind==It_Kind_Samus_GBeam){
        if(count||(uint64_t)r[1]+176>src->data_size)ARTICLE_FAIL();
        void* prefix=melee_item_special_decode(src,kind,r[1]);if(!prefix)ARTICLE_FAIL();
        itSamusGrappleAttributes* attrs=calloc(1,sizeof(*attrs));
        if(!attrs){free(prefix);ARTICLE_FAIL();}memcpy(attrs,prefix,100);free(prefix);a->special=attrs;
        HSD_Joint** models[]={&attrs->x64,&attrs->x68,&attrs->x6C,&attrs->x70};
        HSD_AnimJoint*** anims[]={&attrs->x74,&attrs->x80,&attrs->x8C,&attrs->x98,&attrs->xA4};
        HSD_MatAnimJoint*** mats[]={&attrs->x78,&attrs->x84,&attrs->x90,&attrs->x9C,&attrs->xA8};
        HSD_ShapeAnimJoint*** shapes[]={&attrs->x7C,&attrs->x88,&attrs->x94,&attrs->xA0,&attrs->xAC};
        for(unsigned i=0;i<5;i++){
            u32 model=joint;
            if(i<4){
                if(!ref(src,r[1]+100+4*i,&model)||model==NONE)ARTICLE_FAIL();
                a->variants[i]=melee_scene_decode(src,model);if(!a->variants[i])ARTICLE_FAIL();
                *models[i]=melee_scene_joint_descriptor(a->variants[i]);melee_scene_release_objects(a->variants[i]);
            }
            u32 child;if(model==NONE||!ref(src,model+8,&child)||child==NONE)ARTICLE_FAIL();
            MeleeScene* scene=melee_scene_decode(src,child);a->animations[i]=scene;if(!scene)ARTICLE_FAIL();
            for(unsigned j=0;j<3;j++){
                u32 table,anim;if(!ref(src,r[1]+116+12*i+4*j,&table))ARTICLE_FAIL();
                if(table==NONE)continue;
                if(!ref(src,table,&anim))ARTICLE_FAIL();
                if(anim!=NONE&&!(j==0?melee_scene_bind_joints(scene,anim):j==1?melee_scene_bind_materials(scene,anim):melee_scene_bind_shapes(scene,anim)))ARTICLE_FAIL();
                if(j==0){a->grapple_anim[i]=melee_scene_animation_descriptor(scene);*anims[i]=&a->grapple_anim[i];}
                else if(j==1){a->grapple_mat[i]=melee_scene_material_descriptor(scene);*mats[i]=&a->grapple_mat[i];}
                else{a->grapple_shape[i]=melee_scene_shape_descriptor(scene);*shapes[i]=&a->grapple_shape[i];}
            }
            melee_scene_release_objects(scene);
        }
        a->article.x4_specialAttributes=attrs;
    }
    else if(kind==It_Kind_Link_Boomerang||kind==It_Kind_CLink_Boomerang){
        if((uint64_t)r[1]+100>src->data_size)ARTICLE_FAIL();
        void* prefix=melee_item_special_decode(src,kind,r[1]);if(!prefix)ARTICLE_FAIL();
        itLinkBoomerangAttributes* attrs=calloc(1,sizeof(*attrs));
        if(!attrs){free(prefix);ARTICLE_FAIL();}
        memcpy(attrs,prefix,68);free(prefix);a->special=attrs;
        for(unsigned i=0;i<2;i++){
            u32 model;if(!ref(src,r[1]+68+4*i,&model)||model==NONE)ARTICLE_FAIL();
            MeleeScene* scene=melee_scene_decode(src,model);a->variants[i]=scene;if(!scene)ARTICLE_FAIL();
            for(unsigned j=0;j<3;j++){
                u32 anim;if(!ref(src,r[1]+76+12*i+4*j,&anim))ARTICLE_FAIL();
                if(anim!=NONE&&!(j==0?melee_scene_bind_joints(scene,anim):j==1?melee_scene_bind_materials(scene,anim):melee_scene_bind_shapes(scene,anim)))ARTICLE_FAIL();
            }
            if(i)attrs->x48=melee_scene_joint_descriptor(scene);else attrs->x44=melee_scene_joint_descriptor(scene);
            AnimBundle* bundle=i?&attrs->x58_anim:&attrs->x4C_anim;
            bundle->anim=melee_scene_animation_descriptor(scene);bundle->matanim=melee_scene_material_descriptor(scene);bundle->shapeanim=melee_scene_shape_descriptor(scene);
            melee_scene_release_objects(scene);
        }
        a->article.x4_specialAttributes=attrs;
    }
    else if(kind==It_Kind_IceClimber_GumStrings){
        if((uint64_t)r[1]+44>src->data_size)ARTICLE_FAIL();
        void* prefix=melee_item_special_decode(src,kind,r[1]);if(!prefix)ARTICLE_FAIL();
        itClimbersStringAttributes* attrs=calloc(1,sizeof(*attrs));
        if(!attrs){free(prefix);ARTICLE_FAIL();}
        memcpy(attrs,prefix,36);free(prefix);a->special=attrs;
        for(unsigned i=0;i<2;i++){
            u32 model;if(!ref(src,r[1]+36+4*i,&model)||model==NONE)ARTICLE_FAIL();
            a->variants[i]=melee_scene_decode(src,model);if(!a->variants[i])ARTICLE_FAIL();
            HSD_Joint* joint_desc=melee_scene_joint_descriptor(a->variants[i]);
            if(i)attrs->x28_joint=joint_desc;else attrs->x24_joint=joint_desc;
            melee_scene_release_objects(a->variants[i]);
        }
        a->article.x4_specialAttributes=attrs;
    }
    else if(kind==It_Kind_Seak_Chain){
        if((uint64_t)r[1]+108>src->data_size)ARTICLE_FAIL();
        void* prefix=melee_item_special_decode(src,kind,r[1]);if(!prefix)ARTICLE_FAIL();
        itSeakChain_Attrs* attrs=calloc(1,sizeof(*attrs));
        if(!attrs){free(prefix);ARTICLE_FAIL();}
        memcpy(attrs,prefix,100);free(prefix);a->special=attrs;
        for(unsigned i=0;i<2;i++){
            u32 model;if(!ref(src,r[1]+100+4*i,&model)||model==NONE)ARTICLE_FAIL();
            a->variants[i]=melee_scene_decode(src,model);if(!a->variants[i])ARTICLE_FAIL();
            HSD_Joint* joint_desc=melee_scene_joint_descriptor(a->variants[i]);
            if(i)attrs->x68_joint=joint_desc;else attrs->x64_joint=joint_desc;
            melee_scene_release_objects(a->variants[i]);
        }
        a->article.x4_specialAttributes=attrs;
    }
    else if(kind==It_PKind_Unknown||kind==It_Kind_Unknown_Swarm){
        if((uint64_t)r[1]+36+26*4>src->data_size)ARTICLE_FAIL();
        void* prefix=melee_item_special_decode(src,kind,r[1]);if(!prefix)ARTICLE_FAIL();
        itUnknownAttributes* attrs=calloc(1,sizeof(*attrs));
        if(!attrs){free(prefix);ARTICLE_FAIL();}
        memcpy(attrs,prefix,36);free(prefix);a->special=attrs;
        for(unsigned i=0;i<26;i++){
            u32 variant;if(!ref(src,r[1]+36+4*i,&variant)||variant==NONE)ARTICLE_FAIL();
            a->variants[i]=melee_scene_decode(src,variant);if(!a->variants[i])ARTICLE_FAIL();
            attrs->x24[i]=melee_scene_joint_descriptor(a->variants[i]);
            melee_scene_release_objects(a->variants[i]);
        }
        a->article.x4_specialAttributes=attrs;
    }
    else if(kind==It_Kind_Arwing_Laser){
        u32 common;if(!ref(src,r[1],&common)||common==NONE)ARTICLE_FAIL();
        ArwingLaserAttr* attrs=calloc(1,sizeof(*attrs));if(!attrs)ARTICLE_FAIL();a->special=attrs;
        /* The stage article supplies the five-word common attribute prefix. */
        for(unsigned i=0;i<5;i++){u32 bits;if(!melee_archive_u32(src,common+4*i,&bits))ARTICLE_FAIL();memcpy((u8*)&a->laser_attr+4*i,&bits,4);}
        attrs->x0=&a->laser_attr;
        if(!melee_archive_f32(src,r[1]+4,&attrs->x4)||!isfinite(attrs->x4)||
           !melee_archive_f32(src,r[1]+8,&attrs->x8)||!isfinite(attrs->x8))ARTICLE_FAIL();
        a->article.x4_specialAttributes=attrs;
    }
    else if(kind==It_Kind_Leadead||kind==It_Kind_Octarock){
        u32 common,bits;if(!ref(src,r[1],&common)||common==NONE)ARTICLE_FAIL();
        for(unsigned i=0;i<5;i++){
            if(!melee_archive_u32(src,common+4*i,&bits))ARTICLE_FAIL();
            if(i>0&&i<4){float v;memcpy(&v,&bits,4);if(!isfinite(v))ARTICLE_FAIL();}
            a->likelike_common.words[i]=bits;
        }
        if((uint64_t)r[1]+32>src->data_size)ARTICLE_FAIL();
        const u8* raw=src->bytes+32+r[1];
        if(kind==It_Kind_Leadead){
            itLeadeadAttributes* d=calloc(1,sizeof(*d));if(!d)ARTICLE_FAIL();a->special=d;d->x0=&a->likelike_common.leadead;
            for(unsigned i=1;i<6;i++){
                if(!melee_archive_u32(src,r[1]+4*i,&bits))ARTICLE_FAIL();
                if(i>1){float v;memcpy(&v,&bits,4);if(!isfinite(v))ARTICLE_FAIL();}
                memcpy((u8*)&d->x4+4*(i-1),&bits,4);
            }
            for(unsigned i=0;i<3;i++){u16 h=((u16)raw[24+2*i]<<8)|raw[25+2*i];memcpy((u8*)&d->x18+2*i,&h,2);}
            memcpy(&d->x1E,raw+30,2);
        }else{
            itOctarockAttributes* d=calloc(1,sizeof(*d));if(!d)ARTICLE_FAIL();a->special=d;d->x0=&a->likelike_common.integers.x;
            for(unsigned i=1;i<7;i++){float v;if(!melee_archive_f32(src,r[1]+4*i,&v)||!isfinite(v))ARTICLE_FAIL();memcpy((u8*)&d->x4+4*(i-1),&v,4);}
            u16 h=((u16)raw[28]<<8)|raw[29];memcpy(&d->x1C,&h,2);memcpy((u8*)&d->x1C+2,raw+30,2);
        }
        a->article.x4_specialAttributes=a->special;
    }
    else if(kind==It_Kind_Likelike){
        itLikelikeAttributes* d=calloc(1,sizeof(*d));if(!d)ARTICLE_FAIL();a->special=d;
        u32 common,bits;if(!ref(src,r[1],&common)||common==NONE)ARTICLE_FAIL();
        for(unsigned i=0;i<5;i++){
            if(!melee_archive_u32(src,common+4*i,&bits))ARTICLE_FAIL();
            if(i>0&&i<4){float value;memcpy(&value,&bits,4);if(!isfinite(value))ARTICLE_FAIL();}
            a->likelike_common.words[i]=bits;
        }
        d->x0.x0_f32=&a->likelike_common.vector;
        for(unsigned i=1;i<15;i++){
            if(!melee_archive_u32(src,r[1]+4*i,&bits))ARTICLE_FAIL();
            if(i>=7){float value;memcpy(&value,&bits,4);if(!isfinite(value))ARTICLE_FAIL();}
            memcpy((u8*)&d->x4+4*(i-1),&bits,4);
        }
        if((uint64_t)r[1]+136>src->data_size)ARTICLE_FAIL();
        memcpy(&d->x3C,src->bytes+32+r[1]+0x3C,4);
        for(unsigned i=0;i<18;i++){
            if(!melee_archive_u32(src,r[1]+0x40+4*i,&bits))ARTICLE_FAIL();
            memcpy((u8*)d->x40+4*i,&bits,4);
        }
        a->article.x4_specialAttributes=d;
    }
    else if(kind==It_Kind_Kyasarin||kind==It_Kind_Kyasarin_Egg){
        u32 common,bits;if(!ref(src,r[1],&common)||common==NONE)ARTICLE_FAIL();
        for(unsigned i=0;i<5;i++)if(!melee_archive_u32(src,common+4*i,&a->likelike_common.words[i]))ARTICLE_FAIL();
        size_t size=kind==It_Kind_Kyasarin?sizeof(itKyasarinAttributes):sizeof(itKyasarinEggAttributes);
        unsigned words=kind==It_Kind_Kyasarin?18:4,floats=kind==It_Kind_Kyasarin?15:2;
        void* d=calloc(1,size);if(!d)ARTICLE_FAIL();a->special=d;
        u8* tail;
        if(kind==It_Kind_Kyasarin){itKyasarinAttributes* v=d;v->x0=(s32*)a->likelike_common.words;tail=(u8*)&v->x4;}
        else{itKyasarinEggAttributes* v=d;v->x0=(s32*)a->likelike_common.words;tail=(u8*)&v->x4;}
        for(unsigned i=0;i<words;i++){
            if(!melee_archive_u32(src,r[1]+4+4*i,&bits))ARTICLE_FAIL();memcpy(tail+4*i,&bits,4);
            if(i<floats){float f;memcpy(&f,&bits,4);if(!isfinite(f))ARTICLE_FAIL();}
        }
        a->article.x4_specialAttributes=d;
    }
    else if(kind==It_Kind_Ottosea){
        itOldottoseaAttributes* d=calloc(1,sizeof(*d));if(!d)ARTICLE_FAIL();a->special=d;
        u32 common,bits;if(!ref(src,r[1],&common)||common==NONE||!melee_archive_u32(src,common,&bits))ARTICLE_FAIL();
        memcpy(&a->noko_common.x0,&bits,4);
        if(!melee_archive_f32(src,common+4,&a->noko_common.x4)||!isfinite(a->noko_common.x4))ARTICLE_FAIL();
        d->x0=(void*)&a->noko_common;
        float* fields[]={&d->x4,&d->x8,&d->xC,&d->x14,&d->x18,&d->x1C,&d->x20,&d->x24};
        const unsigned offsets[]={4,8,12,20,24,28,32,36};
        for(unsigned i=0;i<8;i++)if(!melee_archive_f32(src,r[1]+offsets[i],fields[i])||!isfinite(*fields[i]))ARTICLE_FAIL();
        if((uint64_t)r[1]+0x29>src->data_size)ARTICLE_FAIL();
        memcpy(&d->x10,src->bytes+32+r[1]+0x10,1);memcpy(&d->x28,src->bytes+32+r[1]+0x28,1);
        a->article.x4_specialAttributes=d;
    }
    else if(kind==It_Kind_Kuriboh){
        struct {s32* common;float x4,x8,xC;}* d=calloc(1,sizeof(*d));
        if(!d)ARTICLE_FAIL();a->special=d;d->common=&a->noko_common.x0;
        u32 common,bits;if(!ref(src,r[1],&common)||common==NONE||!melee_archive_u32(src,common,&bits))ARTICLE_FAIL();
        memcpy(&a->noko_common.x0,&bits,4);
        if(!melee_archive_f32(src,common+4,&a->noko_common.x4)||!isfinite(a->noko_common.x4))ARTICLE_FAIL();
        for(unsigned i=0;i<3;i++){
            float value;if(!melee_archive_f32(src,r[1]+4+4*i,&value)||!isfinite(value))ARTICLE_FAIL();
            memcpy((u8*)&d->x4+4*i,&value,4);
        }
        a->article.x4_specialAttributes=d;
    }
    else if(kind==It_Kind_Nokonoko||kind==It_Kind_Patapata){
        u32 common,bits;if(!ref(src,r[1],&common)||common==NONE)ARTICLE_FAIL();
        if(kind==It_Kind_Nokonoko){
            itNokoNoko_DatAttrs* d=calloc(1,sizeof(*d));if(!d)ARTICLE_FAIL();a->special=d;
            d->x0=&a->noko_common;
            if(!melee_archive_u32(src,common,&bits))ARTICLE_FAIL();memcpy(&d->x0->x0,&bits,4);
            if(!melee_archive_f32(src,common+4,&d->x0->x4)||!isfinite(d->x0->x4)||
               !melee_archive_f32(src,r[1]+4,&d->x4)||!isfinite(d->x4)||
               !melee_archive_f32(src,r[1]+8,&d->x8)||!isfinite(d->x8))ARTICLE_FAIL();
        }else{
            itPatapataAttributes* d=calloc(1,sizeof(*d));if(!d)ARTICLE_FAIL();a->special=d;d->x0=&a->pata_common;
            if(!melee_archive_u32(src,common,&bits))ARTICLE_FAIL();memcpy(d->x0->pad,&bits,4);
            if(!melee_archive_f32(src,common+4,&d->x0->x4)||!isfinite(d->x0->x4))ARTICLE_FAIL();
            for(unsigned i=1;i<16;i++){
                if(!melee_archive_u32(src,r[1]+4*i,&bits))ARTICLE_FAIL();
                if(i!=4&&i!=7&&i!=9&&i!=11){float v;memcpy(&v,&bits,4);if(!isfinite(v))ARTICLE_FAIL();}
                memcpy((u8*)&d->x4+4*(i-1),&bits,4);
            }
        }
        a->article.x4_specialAttributes=a->special;
    }
    else if(kind==It_Kind_Klap){
        u32 common;if(!ref(src,r[1],&common)||common==NONE)ARTICLE_FAIL();
        itKlapAttributes* attrs=calloc(1,sizeof(*attrs));if(!attrs)ARTICLE_FAIL();a->special=attrs;
        attrs->common=attrs->owned_common;
        for(unsigned i=0;i<5;i++)if(!melee_archive_u32(src,common+4*i,&attrs->owned_common[i]))ARTICLE_FAIL();
        a->article.x4_specialAttributes=attrs;
    }
    else if(kind==It_Kind_WhispyApple||kind==It_Kind_WhispyHealApple){
        u32 common;if(!ref(src,r[1],&common)||common==NONE)ARTICLE_FAIL();
        itWhispyAppleAttributes* attrs=calloc(1,sizeof(*attrs));if(!attrs)ARTICLE_FAIL();a->special=attrs;
        attrs->common=attrs->owned_common;
        for(unsigned i=0;i<5;i++)if(!melee_archive_u32(src,common+4*i,&attrs->owned_common[i]))ARTICLE_FAIL();
        for(unsigned i=0;i<6;i++){u32 bits;if(!melee_archive_u32(src,r[1]+4+4*i,&bits))ARTICLE_FAIL();memcpy((u8*)&attrs->x4+4*i,&bits,4);}
        a->article.x4_specialAttributes=attrs;
    }
    else if(kind==It_Kind_Heiho){
        u32 threshold,bits;
        if(!ref(src,r[1],&threshold)||threshold==NONE||!melee_archive_u32(src,threshold,&bits))ARTICLE_FAIL();
        itHeihoAttributes* attrs=calloc(1,sizeof(*attrs));if(!attrs)ARTICLE_FAIL();a->special=attrs;
        memcpy(&attrs->owned_damage_threshold,&bits,4);attrs->damage_threshold=&attrs->owned_damage_threshold;
        for(unsigned i=0;i<6;i++)if(!melee_archive_f32(src,r[1]+4+4*i,&attrs->values[i])||!isfinite(attrs->values[i]))ARTICLE_FAIL();
        a->article.x4_specialAttributes=attrs;
    }
    else if(!no_special){a->special=melee_item_special_decode(src,kind,r[1]);if(!a->special)ARTICLE_FAIL();a->article.x4_specialAttributes=a->special;}
    if(count){
        if((uint64_t)r[3]+16*count>src->data_size)ARTICLE_FAIL();
        a->states.x0_itemStateDesc=calloc(count,sizeof(ItemStateDesc));if(!a->states.x0_itemStateDesc)ARTICLE_FAIL();a->article.xC_itemStates=&a->states;
        for(unsigned i=0;i<count;i++){
            u32 state=r[3]+16*i,fields[4];for(unsigned k=0;k<4;k++)if(!ref(src,state+4*k,fields+k))ARTICLE_FAIL();
            ItemStateDesc* d=&a->states.x0_itemStateDesc[i];
            if(fields[0]!=NONE||fields[1]!=NONE||fields[2]!=NONE){
                a->animations[i]=melee_item_animation_decode(src,joint,state);MeleeScene* s=a->animations[i];if(!s)ARTICLE_FAIL();
                d->x0_anim_joint=melee_scene_animation_descriptor(s);d->x4_matanim_joint=melee_scene_material_descriptor(s);d->x8_parameters=melee_scene_shape_descriptor(s);melee_scene_release_objects(s);
            }
            if(fields[3]!=NONE){a->scripts[i]=melee_item_script_decode(src,fields[3]);if(!a->scripts[i])ARTICLE_FAIL();d->xC_script=melee_item_script_root(a->scripts[i]);}
        }
    }
    melee_archive_release(&archive);return a;
fail:melee_archive_release(&archive);melee_item_article_free(a);return NULL;
#undef ARTICLE_FAIL
}
