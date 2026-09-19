#include "melee_items_data.h"
#include "melee_item_article.h"
#include "melee_item_common.h"
#include "melee_item_colors.h"
#include "melee_item_hurtbones.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
struct MeleeItemsData {
    it_804D6D20_t header;ItemCommonData common;it_804D6D40_t enemy;
    Article* items[43];Article* character[It_PKind_Start-It_Kind_Kuriboh];
    Article* pokemon[It_Kind_Old_Kuri-It_PKind_Start];
    u32 character_offsets[It_PKind_Start-It_Kind_Kuriboh],pokemon_offsets[It_Kind_Old_Kuri-It_PKind_Start];
    MeleeItemArticle* pokemon_owners[It_Kind_Old_Kuri-It_PKind_Start];MeleeItemArticle* owners[43];MeleeItemColors* colors;
    MeleeItemArticle* coin_owner;MeleeItemArticle* match_coin_owner;MeleeItemArticle* goomba_owner;MeleeItemArticle* maze_enemies[4];
    Article hazard;ItemModelDesc hazard_model;ItemStateArray hazard_table;ItemStateDesc hazard_states[20];
};
/* US1.02 common article state schema, including ten Scope Beam states. */
static const u8 state_counts[43]={2,3,4,2,5,4,7,4,2,2,1,1,2,8,1,1,2,2,0,3,3,1,1,2,5,5,1,1,2,3,1,1,1,1,2,1,1,2,10,1,1,1,1};
void melee_items_data_free(MeleeItemsData* d){if(d){for(unsigned i=0;i<4;i++)melee_item_article_free(d->maze_enemies[i]);melee_item_article_free(d->goomba_owner);melee_item_article_free(d->coin_owner);melee_item_article_free(d->match_coin_owner);for(unsigned i=0;i<It_Kind_Old_Kuri-It_PKind_Start;i++)melee_item_article_free(d->pokemon_owners[i]);for(unsigned i=0;i<43;i++)melee_item_article_free(d->owners[i]);melee_item_colors_free(d->colors);melee_item_hurtbones_free(d->hazard.x8_hurtbones);free(d);}}
it_804D6D20_t* melee_items_data_header(MeleeItemsData* d){return d?&d->header:NULL;}
Article* melee_items_data_article(MeleeItemsData* d,unsigned kind){
    if(!d)return NULL;if(kind<43)return d->items[kind];
    if(kind<It_PKind_Start)return d->character[kind-43];
    if(kind<It_Kind_Old_Kuri)return d->pokemon[kind-It_PKind_Start];return NULL;
}
static int ref(const MeleeArchive* a,u32 at,u32* v){MeleeHostBool present;if(!melee_archive_pointer(a,at,v,&present))return 0;if(!present)*v=UINT32_MAX;return 1;}
MeleeItemsData* melee_items_data_decode(const MeleeArchive* a){
#define ITEMS_FAIL() do { fprintf(stderr,"Item table conversion failed at line %d\n",__LINE__); goto fail; } while(0)
    u32 root,r[6];if(!a||!melee_archive_find(a,"itPublicData",&root)||(uint64_t)root+24>a->data_size)return NULL;
    for(unsigned i=0;i<6;i++)if(!ref(a,root+4*i,r+i)||r[i]==UINT32_MAX)return NULL;
    MeleeItemsData* d=calloc(1,sizeof(*d));if(!d)return NULL;
    if(!melee_item_common_decode(a,&d->common))ITEMS_FAIL();
    for(unsigned i=0;i<7;i++){u32 bits;if(!melee_archive_u32(a,r[4]+4*i,&bits))ITEMS_FAIL();if(i){float v;memcpy(&v,&bits,4);if(!isfinite(v))ITEMS_FAIL();}memcpy((u8*)&d->enemy+4*i,&bits,4);}
    d->colors=melee_item_colors_decode(a,r[5],7);if(!d->colors)ITEMS_FAIL();
    for(unsigned i=0;i<43;i++){
        u32 at;if(!ref(a,r[1]+4*i,&at)||at==UINT32_MAX)ITEMS_FAIL();
        d->owners[i]=melee_item_article_decode(a,i,at,state_counts[i]);if(!d->owners[i]){fprintf(stderr,"Item kind %u failed\n",i);ITEMS_FAIL();}d->items[i]=melee_item_article_descriptor(d->owners[i]);
    }
    /* These writable slots are populated by fighter/stage registration. Retail
     * built-in noncommon articles are recorded as deferred, never cast in place. */
    for(unsigned i=0;i<sizeof(d->character_offsets)/sizeof(u32);i++)if(!ref(a,r[2]+4*i,d->character_offsets+i))ITEMS_FAIL();
    for(unsigned i=0;i<sizeof(d->pokemon_offsets)/sizeof(u32);i++)if(!ref(a,r[3]+4*i,d->pokemon_offsets+i))ITEMS_FAIL();
    static const struct {unsigned kind,count;} pokemon[] = {
        {It_PKind_Tosakinto,3},{It_PKind_Chicorita,2},
        {It_PKind_Kabigon,2},{It_Kind_Chicorita_Leaf,1},{It_PKind_Hassam,3},
        {It_PKind_Kamex,3},{It_Kind_Kamex_HydroPump,1},
        {It_PKind_Pippi,6},{It_PKind_Togepy,7},
        {It_PKind_Porygon2,2},
        {It_PKind_Lucky,4},{It_Kind_Lucky_Egg,0},
        {It_PKind_Hitodeman,2},{It_Kind_Hitodeman_Star,1},
        {It_PKind_Maril,1},{It_PKind_Fushigibana,2},
        {It_PKind_Hinoarashi,2},{It_Kind_Hinoarashi_Flame,1},
        {It_PKind_Mew,3},{It_PKind_Cerebi,3},
        {It_PKind_Houou,6},{It_Kind_Houou_SacredFire,1},
        {It_PKind_Lugia,6},{It_Kind_Lugia_Aeroblast,1},{It_Kind_Lugia_Aeroblast2,1},{It_Kind_Lugia_Aeroblast3,1},
        {It_PKind_Unknown,2},{It_Kind_Unknown_Swarm,1},
        {It_PKind_Marumine,7},
        {It_PKind_Entei,1},{It_PKind_Raikou,1},{It_PKind_Suikun,1},
        {It_PKind_Sonans,2},{It_PKind_Kireihana,5},
        {It_PKind_Fire,3},{It_PKind_Thunder,3},{It_PKind_Freezer,3},
        {It_PKind_Lizardon,4},{It_Kind_Lizardon_Flame1,1},{It_Kind_Lizardon_Flame2,1},{It_Kind_Lizardon_Flame3,1},{It_Kind_Lizardon_Flame4,1},
        {It_PKind_Matadogas,2},{It_Kind_Matadogas_Gas1,1},{It_Kind_Matadogas_Gas2,1},
    };
    for(unsigned i=0;i<sizeof(pokemon)/sizeof(*pokemon);i++){
        unsigned index=pokemon[i].kind-It_PKind_Start;
        d->pokemon_owners[index]=melee_item_article_decode(a,pokemon[i].kind,d->pokemon_offsets[index],pokemon[i].count);
        if(!d->pokemon_owners[index]){fprintf(stderr,"Pokemon item kind %u failed\n",pokemon[i].kind);ITEMS_FAIL();}
        d->pokemon[index]=melee_item_article_descriptor(d->pokemon_owners[index]);
    }
    const unsigned maze_kinds[]={It_Kind_Leadead,It_Kind_Octarock,It_Kind_Octarock_Stone,It_Kind_Ottosea},maze_counts[]={10,5,1,7};
    for(unsigned i=0;i<4;i++){
        unsigned index=maze_kinds[i]-It_Kind_Kuriboh;
        d->maze_enemies[i]=melee_item_article_decode(a,maze_kinds[i],d->character_offsets[index],maze_counts[i]);if(!d->maze_enemies[i])ITEMS_FAIL();
        d->character[index]=melee_item_article_descriptor(d->maze_enemies[i]);
    }
    d->goomba_owner=melee_item_article_decode(a,It_Kind_Kuriboh,d->character_offsets[0],5);
    if(!d->goomba_owner)ITEMS_FAIL();
    d->character[0]=melee_item_article_descriptor(d->goomba_owner);
    d->match_coin_owner=melee_item_article_decode(a,It_Kind_Unk4,
        d->character_offsets[It_Kind_Unk4-It_Kind_Kuriboh],1);
    if(!d->match_coin_owner)ITEMS_FAIL();
    d->character[It_Kind_Unk4-It_Kind_Kuriboh]=melee_item_article_descriptor(d->match_coin_owner);
    /* Coin's six code states use dynamically supplied trophy models and no
     * serialized animation states. */
    d->coin_owner=melee_item_article_decode(a,It_Kind_Coin,
        d->character_offsets[It_Kind_Coin-It_Kind_Kuriboh],0);
    if(!d->coin_owner)ITEMS_FAIL();
    d->character[It_Kind_Coin-It_Kind_Kuriboh]=melee_item_article_descriptor(d->coin_owner);
    /* The stage hazard (Yaku) article receives its attributes and scripts from
     * stage code. Its retail schema is a null model, one root hurtbone and 20
     * initially empty states, matching it_803F8C8C. */
    {
        u32 at=d->character_offsets[It_PKind_Random-It_Kind_Kuriboh],parts[6],joint,bones,attach;
        if(at==UINT32_MAX)ITEMS_FAIL();for(unsigned i=0;i<6;i++)if(!ref(a,at+4*i,parts+i))ITEMS_FAIL();
        if(parts[0]!=UINT32_MAX||parts[1]!=UINT32_MAX||parts[5]!=UINT32_MAX||parts[2]==UINT32_MAX||parts[3]==UINT32_MAX||parts[4]==UINT32_MAX)ITEMS_FAIL();
        if(!ref(a,parts[4],&joint)||joint!=UINT32_MAX||!melee_archive_u32(a,parts[4]+4,&bones)||bones!=1||!melee_archive_u32(a,parts[4]+8,&attach)||attach!=0||(uint64_t)parts[4]+16>a->data_size)ITEMS_FAIL();
        for(unsigned i=0;i<80;i++){u32 target;if(!ref(a,parts[3]+4*i,&target)||target!=UINT32_MAX)ITEMS_FAIL();}
        d->hazard_model.x4_bone_count=bones;d->hazard_model.xC_bit_field=a->bytes[32+parts[4]+12];
        d->hazard.x8_hurtbones=melee_item_hurtbones_decode(a,parts[2],1);if(!d->hazard.x8_hurtbones)ITEMS_FAIL();
        d->hazard_table.x0_itemStateDesc=d->hazard_states;d->hazard.xC_itemStates=&d->hazard_table;d->hazard.x10_modelDesc=&d->hazard_model;
        d->character[It_PKind_Random-It_Kind_Kuriboh]=&d->hazard;
    }
    d->header=(it_804D6D20_t){&d->common,d->items,d->character,d->pokemon,&d->enemy,melee_item_colors_entries(d->colors)};
    return d;
fail:melee_items_data_free(d);return NULL;
#undef ITEMS_FAIL
}
