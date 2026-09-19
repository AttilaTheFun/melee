#include <melee/gm/gm_1A45.h>
#include <melee/gm/gmregclear.h>
#include <melee/gm/gm_1884.h>
#include <sysdolphin/baselib/random.h>
#include <stdlib.h>
#include <melee/mp/mpcoll.h>
#include <sysdolphin/baselib/tobj.h>
#include <melee/cm/types.h>
#include <melee/lb/lb_00B0.h>
/* Scripted input for the windowless startup probe; never reads host devices. */
#include "melee_pad_backend.h"
static PADStatus ports[4];
static int initialized;
static void initialize(void){
    if(initialized)return;initialized=1;
    for(unsigned i=1;i<4;i++)ports[i].err=PAD_ERR_NO_CONTROLLER;
}
static void publish(unsigned buttons)
{
    initialize();ports[0].stickX=ports[0].stickY=0;
    ports[0].button=buttons;
    melee_pad_publish(ports,0);
}

void melee_startup_publish_confirm(int pressed){publish(pressed?PAD_BUTTON_A:0);}
void melee_startup_publish_start(int pressed){publish(pressed?PAD_BUTTON_START:0);}

void melee_startup_publish_down(int pressed){publish(pressed?PAD_BUTTON_DOWN:0);}

void melee_startup_publish_stick(int x,int y){
    initialize();ports[0].button=0;
    ports[0].stickX=x;ports[0].stickY=y;melee_pad_publish(ports,0);
}

void melee_startup_publish_opponent(int x,int y,int confirm){
    initialize();ports[1].err=PAD_ERR_NONE;ports[1].stickX=x;ports[1].stickY=y;
    ports[1].button=confirm?PAD_BUTTON_A:0;melee_pad_publish(ports,0);
}

#include <melee/pl/player.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/lb/lbanim.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
static void count_joints(HSD_JObj* joint,unsigned* total,unsigned* visible)
{
    for(;joint;joint=joint->next){
        ++*total;
        if(!(joint->flags&JOBJ_HIDDEN))++*visible;
        count_joints(joint->child,total,visible);
    }
}
void melee_startup_report_fighters(unsigned frame)
{
    static int last_motion[2]={-1,-1};
    for(int slot=0;slot<2;++slot){
        HSD_GObj* object=Player_GetEntity(slot);
        /* Player slots can retain handles during scene teardown. Compare
         * against the live fighter list before dereferencing a handle. */
        HSD_GObj* live = plinklow_gobjs ? plinklow_gobjs[8] : NULL;
        while (live && live != object) live = live->prev;
        if(!live||live->classifier!=HSD_GOBJ_CLASS_FIGHTER||
           !live->user_data||!live->hsd_obj)continue;
        Fighter* fighter=object->user_data;
        if(frame%20&&last_motion[slot]==fighter->motion_id)continue;
        last_motion[slot]=fighter->motion_id;
        HSD_JObj* joint=object->hsd_obj;
        unsigned total=0,visible=0;count_joints(joint,&total,&visible);
        fprintf(stderr,"Fighter frame=%u slot=%d kind=%d motion=%d damage=%.2f pos=(%.3f,%.3f,%.3f) scale=(%.3f,%.3f,%.3f) draw=%u invisible=%u hide=%u/%u/%u joints=%u/%u root=%08x\n",
            frame,slot,fighter->kind,fighter->motion_id,fighter->dmg.x1830_percent,fighter->cur_pos.x,fighter->cur_pos.y,fighter->cur_pos.z,
            joint->scale.x,joint->scale.y,joint->scale.z,fighter->x21FC_flag.b7,fighter->invisible,
            fighter->x221E_b5,fighter->x2226_b5,fighter->x221F_b3,visible,total,joint->flags);
    }
}

void melee_startup_publish_combat(unsigned slot,int x,int y,unsigned buttons)
{
    if(slot>=4)return;
    initialize();ports[slot].err=PAD_ERR_NONE;
    ports[slot].stickX=x;ports[slot].stickY=y;ports[slot].button=buttons;
    ports[slot].triggerLeft=(buttons&PAD_TRIGGER_L)?255:0;
    melee_pad_publish(ports,0);
    fprintf(stderr,"Combat input slot=%u stick=%d,%d buttons=%04x\n",slot,x,y,buttons);
}

#include "melee_audio_producer.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <time.h>
static MeleeAudioRing* audio_queue;
static MeleeAudioProducer* audio_producer;
static pthread_t audio_consumer;
static atomic_bool audio_stop;
static unsigned long long pcm_frames, pcm_nonzero;
static void* consume_audio(void* unused)
{
    (void)unused;
    while(!atomic_load(&audio_stop)){
        if(melee_audio_producer_status(audio_producer)==MELEE_AUDIO_FAILED){
            fputs("Combat audio producer failed\n",stderr);_Exit(6);
        }
        int16_t pcm[320];size_t count=melee_audio_ring_read(audio_queue,pcm,160);
        pcm_frames+=count;
        for(size_t i=0;i<count*2;++i)pcm_nonzero+=pcm[i]!=0;
        struct timespec delay={.tv_nsec=5000000};nanosleep(&delay,NULL);
    }
    return NULL;
}
void melee_startup_audio_start(void)
{
    audio_queue=melee_audio_ring_create();
    audio_producer=melee_audio_producer_start(audio_queue);
    if(!audio_queue||!audio_producer||pthread_create(&audio_consumer,NULL,consume_audio,NULL)){
        fputs("Combat audio initialization failed\n",stderr);_Exit(6);
    }
}
void melee_startup_audio_finish(void)
{
    int interrupts=OSEnableInterrupts();
    atomic_store(&audio_stop,true);pthread_join(audio_consumer,NULL);
    if(!melee_audio_producer_destroy(audio_producer)||!pcm_nonzero){
        fputs("Combat audio verification failed\n",stderr);_Exit(6);
    }
    melee_audio_ring_destroy(audio_queue);
    fprintf(stderr,"Combat audio: %llu PCM frames, %llu nonzero samples\n",pcm_frames,pcm_nonzero);
    OSRestoreInterrupts(interrupts);
}

#include <melee/it/kinds/itfflower.h>
#include <dolphin/os.h>
int melee_startup_spawn_flower(void){
    Vec3 position={0,20,0};
    Item_GObj* item=it_80292D48(&position);
    OSReport("Focused Fire Flower spawn: %s\n",item?"created":"failed");
    return item != NULL;
}

#include <melee/gm/gmvs.h>
#include <melee/gm/types.h>
int melee_startup_shorten_match(void){
    VsSceneController* match=gmVs_GetController_0();
    if(!match->start.timer_enabled||match->start.timer_counts_up||match->match_over)return 0;
    OSReport("Transition probe: active match timer %u -> 5 seconds\n",match->timer_seconds);
    match->timer_seconds=5;
    return 1;
}

#include <melee/gm/gm_1A3F.h>
#include <melee/gm/gmvsmode.h>
int melee_startup_rematch_progress(unsigned frame)
{
    static int last = -1, matches = 0, results = 0, returned = 0;
    int mode = gm_GetCurrentGameMode();
    int state = gm_GetCurrentSceneIndex();
    int key = mode * 256 + state;
    if (key != last) {
        OSReport("Rematch scene: frame=%u mode=%d state=%d\n", frame, mode, state);
        if (mode == GM_VS) {
            if (state == gmVsMode_State_Results) results = 1;
            if (results && state == gmVsMode_State_Css) returned = 1;
            if (state == gmVsMode_State_Vs) matches++;
        }
        last = key;
    }
    return results && returned && matches >= 2 && mode == GM_VS &&
        state == gmVsMode_State_Vs && !gmVs_GetController_0()->match_over;
}

#include <melee/it/item.h>
#include <melee/it/kinds/inlines.h>
int melee_startup_spawn_goldeen(void){
    Vec3 position={0,20,0};SpawnItem spawn={0};
    spawn.kind=It_PKind_Tosakinto;
    Item_InitSpawnOnPlane(&spawn,NULL,&position,1.0f);
    Item_GObj* item=Item_80268B18(&spawn);
    OSReport("Focused Goldeen spawn: %s\n",item?"created":"failed");
    return item!=NULL;
}

#include <melee/it/it_279C.h>
static unsigned pokemon_probe_kind(unsigned selection){
    static const unsigned kinds[]={0,It_PKind_Chicorita,It_PKind_Kabigon,
        It_PKind_Hassam,It_PKind_Kamex,It_PKind_Matadogas,It_PKind_Lizardon,
        It_PKind_Fire,It_PKind_Thunder,It_PKind_Freezer,It_PKind_Sonans,
        It_PKind_Kireihana,It_PKind_Entei,It_PKind_Raikou,It_PKind_Suikun,It_PKind_Marumine,It_PKind_Unknown,It_PKind_Lugia,It_PKind_Houou,It_PKind_Mew,It_PKind_Cerebi,It_PKind_Hinoarashi,It_PKind_Maril,It_PKind_Fushigibana,It_PKind_Hitodeman,It_PKind_Lucky,It_PKind_Porygon2,It_PKind_Pippi,It_PKind_Togepy};
    return selection<sizeof(kinds)/sizeof(*kinds)?kinds[selection]:0;
}
int melee_startup_spawn_pokemon(unsigned selection){
    if(gm_GetCurrentGameMode()!=GM_VS ||
       gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs ||
       gmVs_GetController_0()->match_over){
        OSReport("Pokemon probe: scripted input did not reach an active VS match\n");
        return 0;
    }
    unsigned kind=pokemon_probe_kind(selection);
    Vec3 position={0,20,0};SpawnItem spawn={0};
    /* Put the reactive target in front of the scripted fighter's jab. */
    if(selection==10)position.x=20;
    spawn.kind=It_Kind_M_Ball;
    Item_InitSpawnOnPlane(&spawn,NULL,&position,1.0f);
    Item_GObj* ball=Item_80268B18(&spawn);if(!ball)return 0;
    spawn.kind=kind;spawn.x4_parent_gobj2=ball;
    Item_GObj* pokemon=Item_80268B18(&spawn);if(!pokemon)return 0;
    /* Apply the same emergence parameters as the Poké Ball spawn path. */
    it_8027AAA0(ball,pokemon->user_data,kind);
    it_8027B288(pokemon,0x440060);
    it_8027B564(pokemon);
    /* This fixture has already released its selected Pokemon. Remove the
     * temporary ball so it cannot release a second, random one later. */
    ((Item*)ball->user_data)->xDD4_itemVar.mball.b1 = true;
    Item_8026A8EC(ball);
    OSReport("Focused Pokemon spawn: kind=%u created\n",kind);
    return 1;
}

int melee_startup_pokemon_progress(unsigned selection,unsigned frame){
    static unsigned states=0,leaves=0,second_attack=0,removed=0,water=0,gas=0,flames=0,reaction=0,attack_effect=0,swarm=0,aeroblast=0,sacred_fire=0,cyndaquil_flame=0,staryu_star=0,chansey_egg=0;
    static int previously_active=0;
    int active=0;
    unsigned kind=pokemon_probe_kind(selection);
    for(HSD_GObj* object=plinklow_gobjs?plinklow_gobjs[9]:NULL;object;object=object->prev){
        if(object->classifier!=HSD_GOBJ_CLASS_ITEM||!object->user_data)continue;
        Item* item=object->user_data;
        if(item->kind==It_Kind_Lucky_Egg&&!chansey_egg){chansey_egg=1;OSReport("Pokemon probe: Chansey healing egg active frame=%u\n",frame);}
        if(item->kind==It_Kind_Hitodeman_Star&&!staryu_star){staryu_star=1;OSReport("Pokemon probe: Staryu star active frame=%u\n",frame);}
        if(item->kind==It_Kind_Hinoarashi_Flame&&!cyndaquil_flame){cyndaquil_flame=1;OSReport("Pokemon probe: Cyndaquil flame active frame=%u\n",frame);}
        if(item->kind==It_Kind_Houou_SacredFire&&!sacred_fire){sacred_fire=1;OSReport("Pokemon probe: Sacred Fire active frame=%u\n",frame);}
        if(item->kind>=It_Kind_Lugia_Aeroblast&&item->kind<=It_Kind_Lugia_Aeroblast3){unsigned bit=1u<<(item->kind-It_Kind_Lugia_Aeroblast);if(!(aeroblast&bit)){aeroblast|=bit;OSReport("Pokemon probe: Aeroblast kind=%u frame=%u\n",item->kind,frame);}}
        if(item->kind==It_Kind_Unknown_Swarm&&!swarm){swarm=1;OSReport("Pokemon probe: Unown swarm active at frame %u\n",frame);}
        if(item->kind>=It_Kind_Lizardon_Flame1&&item->kind<=It_Kind_Lizardon_Flame4){unsigned bit=1u<<(item->kind-It_Kind_Lizardon_Flame1);if(!(flames&bit)){flames|=bit;OSReport("Pokemon probe: flame projectile kind=%u frame=%u\n",item->kind,frame);}}
        if(item->kind==It_Kind_Matadogas_Gas1||item->kind==It_Kind_Matadogas_Gas2){unsigned bit=1u<<(item->kind-It_Kind_Matadogas_Gas1);if(!(gas&bit)){gas|=bit;OSReport("Pokemon probe: gas projectile kind=%u frame=%u\n",item->kind,frame);}}
        if(item->kind==It_Kind_Kamex_HydroPump&&!water){water=1;OSReport("Pokemon probe: water projectile active at frame %u\n",frame);}
        if(item->kind==It_Kind_Chicorita_Leaf&&!leaves){leaves=1;OSReport("Pokemon probe: leaf projectile active at frame %u\n",frame);}
        if(item->kind==kind&&item->msid>=0&&item->msid<32){
            active=1;
            if(selection>=12&&selection<=14&&!attack_effect&&item->xDB4_itcmd_var2){attack_effect=1;OSReport("Pokemon probe: attack effect active kind=%u frame=%u\n",kind,frame);}
            if(selection==10&&!reaction&&item->xDD4_itemVar.sonans.x68>0){reaction=1;OSReport("Pokemon probe: Wobbuffet damage reaction at frame %u\n",frame);}
            if(selection==3&&item->msid==1&&item->xDAC_itcmd_var0)
                second_attack=1;
            if(selection==3&&frame%100==0)
                OSReport("Scizor lifetime frame=%u state=%d timer=%.1f phase=%u pos=(%.2f,%.2f,%.2f)\n",frame,item->msid,item->xD44_lifeTimer,item->xDAC_itcmd_var0,item->pos.x,item->pos.y,item->pos.z);
            unsigned bit=1u<<item->msid;
            if(!(states&bit)){states|=bit;OSReport("Pokemon probe: kind=%u state=%d frame=%u\n",kind,item->msid,frame);}
        }
    }
    if(previously_active&&!active){removed=1;OSReport("Pokemon probe: kind=%u removed at frame %u\n",kind,frame);}
    previously_active=active;
    return selection>=27?(states&0x7c)&&removed:selection==26?(states&3)==3&&removed:selection==25?chansey_egg!=0:selection==24?staryu_star!=0:selection>=22?(states&2)&&removed:selection==21?cyndaquil_flame!=0:selection>=19?(states&6)==6&&removed:selection==18?sacred_fire!=0:selection==17?aeroblast==7:selection==16?swarm!=0:selection==15?(states&(1u<<6))&&removed:selection>=12?attack_effect!=0:selection==10?reaction!=0:selection==11?(states&2)!=0:selection>=7?(states&6)==6:selection==6?flames==15:selection==5?gas==3:selection==4?water!=0:selection==1?leaves!=0:selection==3?
        (states&3)==3&&second_attack&&removed:(states&3)==3;
}

/* Steer through real PAD input so the reactive-item probe actually lands a hit. */
void melee_startup_target_wobbuffet(unsigned frame){
    HSD_GObj* fighter=Player_GetEntity(0);
    HSD_GObj* live=plinklow_gobjs?plinklow_gobjs[8]:NULL;
    while(live&&live!=fighter)live=live->prev;
    if(!live||!live->user_data)return;
    Fighter* fp=live->user_data;
    for(HSD_GObj* object=plinklow_gobjs[9];object;object=object->prev){
        if(object->classifier!=HSD_GOBJ_CLASS_ITEM||!object->user_data)continue;
        Item* item=object->user_data;
        if(item->kind!=It_PKind_Sonans)continue;
        float dx=item->pos.x-fp->cur_pos.x;
        int dir=dx<0?-1:1;
        int approach=dx>14||dx< -14||fp->facing_dir*dir<0;
        melee_startup_publish_combat(0,approach?dir*80:0,0,
            !approach&&frame%20<6?PAD_BUTTON_A:0);
        return;
    }
}

/* Observe an unshortened match; use PAD movement to resolve a tied sudden death. */
int melee_startup_full_match_progress(unsigned frame)
{
    static unsigned seen[16], kinds, common_kinds, results, sudden_death;
    if(gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_SuddenDeath){
        if(!sudden_death){sudden_death=frame;OSReport("Full match probe: sudden death reached frame=%u\n",frame);}
        if(frame==sudden_death+120)melee_startup_publish_combat(1,80,0,0);
        if(frame==sudden_death+420)melee_startup_publish_combat(1,0,0,0);
    }
    if(gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Results&&!results){
        results=1;OSReport("Full match probe: results reached frame=%u item-kinds=%u\n",frame,kinds);
    }
    for(HSD_GObj* object=plinklow_gobjs?plinklow_gobjs[9]:NULL;object;object=object->prev){
        if(object->classifier!=HSD_GOBJ_CLASS_ITEM||!object->user_data)continue;
        Item* item=object->user_data;
        unsigned kind=item->kind;
        if(kind>=512)continue;
        unsigned bit=1u<<(kind%32);
        if(!(seen[kind/32]&bit)){
            seen[kind/32]|=bit;kinds++;if(kind<43)common_kinds++;
            OSReport("Full match probe: item kind=%u state=%d frame=%u\n",kind,item->msid,frame);
        }
    }
    return results&&common_kinds>=3;
}

#include <melee/ft/kinds/ftMario/forward.h>
int melee_startup_mario_progress(unsigned frame,int require_specials)
{
    int mario_seen=0;
    static int fireball_seen, cape_seen, up_seen, down_seen, reflector_seen;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data&&
           ((Fighter*)g->user_data)->kind==Ft_Kind_Mario){
            Fighter* fighter=g->user_data;mario_seen=1;
            if(fighter->reflecting&&!reflector_seen){reflector_seen=1;OSReport("Mario probe: cape reflector active frame=%u\n",frame);}
            if((fighter->motion_id==ftMr_MS_SpecialHi||fighter->motion_id==ftMr_MS_SpecialAirHi)&&!up_seen){up_seen=1;OSReport("Mario probe: Super Jump Punch frame=%u\n",frame);}
            if((fighter->motion_id==ftMr_MS_SpecialLw||fighter->motion_id==ftMr_MS_SpecialAirLw)&&!down_seen){down_seen=1;OSReport("Mario probe: Mario Tornado frame=%u\n",frame);}
        }
    }
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[9]:NULL;g;g=g->prev){
        if(g->classifier==HSD_GOBJ_CLASS_ITEM&&g->user_data&&
           ((Item*)g->user_data)->kind==It_Kind_Mario_Fire&&!fireball_seen){
            fireball_seen=1;OSReport("Mario probe: original fireball spawned frame=%u\n",frame);
        }
    }
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[9]:NULL;g;g=g->prev){
        if(g->classifier==HSD_GOBJ_CLASS_ITEM&&g->user_data&&
           ((Item*)g->user_data)->kind==It_Kind_Mario_Cape&&!cape_seen){
            cape_seen=1;OSReport("Mario probe: original cape spawned frame=%u\n",frame);
        }
    }
    return mario_seen&&(!require_specials||(fireball_seen&&cape_seen&&up_seen&&down_seen&&reflector_seen));
}

#include <melee/ft/kinds/ftCaptain/forward.h>
int melee_startup_captain_progress(unsigned frame,int require_specials)
{
    int live=0;static int punch,boost,dive,kick;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fighter=g->user_data;if(fighter->kind!=Ft_Kind_Captain)continue;
        live=1;
        if(!boost&&(fighter->motion_id==ftCa_MS_SpecialSStart||fighter->motion_id==ftCa_MS_SpecialAirSStart)){boost=1;OSReport("Captain probe: Raptor Boost frame=%u\n",frame);}
        if(!dive&&(fighter->motion_id==ftCa_MS_SpecialHi||fighter->motion_id==ftCa_MS_SpecialAirHi)){dive=1;OSReport("Captain probe: Falcon Dive frame=%u\n",frame);}
        if(!kick&&(fighter->motion_id==ftCa_MS_SpecialLw||fighter->motion_id==ftCa_MS_SpecialAirLw)){kick=1;OSReport("Captain probe: Falcon Kick frame=%u\n",frame);}
        if(!punch&&(fighter->motion_id==ftCa_MS_SpecialN||fighter->motion_id==ftCa_MS_SpecialAirN)){
            punch=1;OSReport("Captain probe: Falcon Punch frame=%u\n",frame);
        }
    }
    return live&&(!require_specials||(punch&&boost&&dive&&kick));
}

#include <melee/ft/kinds/ftDonkey/forward.h>
int melee_startup_donkey_progress(unsigned frame,int require_specials)
{
    int live=0;static int charge,punch,headbutt,spin,slap;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fighter=g->user_data;if(fighter->kind!=Ft_Kind_Donkey)continue;
        live=1;
        if(!punch&&(fighter->motion_id==ftDk_MS_SpecialN||fighter->motion_id==ftDk_MS_SpecialNFull||fighter->motion_id==ftDk_MS_SpecialAirN||fighter->motion_id==ftDk_MS_SpecialAirNFull)){punch=1;OSReport("Donkey probe: Giant Punch released frame=%u\n",frame);}
        if(!headbutt&&(fighter->motion_id==ftDk_MS_SpecialS||fighter->motion_id==ftDk_MS_SpecialAirS)){headbutt=1;OSReport("Donkey probe: Headbutt frame=%u\n",frame);}
        if(!spin&&(fighter->motion_id==ftDk_MS_SpecialHi||fighter->motion_id==ftDk_MS_SpecialAirHi)){spin=1;OSReport("Donkey probe: Spinning Kong frame=%u\n",frame);}
        if(!slap&&fighter->motion_id==ftDk_MS_SpecialLwLoop){slap=1;OSReport("Donkey probe: Hand Slap loop frame=%u\n",frame);}
        if(!charge&&fighter->motion_id==ftDk_MS_SpecialNLoop){
            charge=1;OSReport("Donkey probe: Giant Punch charging frame=%u\n",frame);
        }
    }
    return live&&(!require_specials||(charge&&punch&&headbutt&&spin&&slap));
}

#include <melee/ft/kinds/ftKoopa/forward.h>
int melee_startup_koopa_progress(unsigned frame,int require_specials)
{
    int live=0;static int flame,side,spin,bomb,article,effect;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fighter=g->user_data;if(fighter->kind!=Ft_Kind_Koopa)continue;
        live=1;
        if(!flame&&(fighter->motion_id==ftKp_MS_SpecialN||fighter->motion_id==ftKp_MS_SpecialAirN)){flame=1;OSReport("Bowser probe: Fire Breath frame=%u\n",frame);}
        if(!side&&(fighter->motion_id==ftKp_MS_SpecialSStart||fighter->motion_id==ftKp_MS_SpecialAirSStart)){side=1;OSReport("Bowser probe: Koopa Klaw frame=%u\n",frame);}
        if(!spin&&(fighter->motion_id==ftKp_MS_SpecialHi||fighter->motion_id==ftKp_MS_SpecialAirHi)){spin=1;OSReport("Bowser probe: Whirling Fortress frame=%u\n",frame);}
        if(!bomb&&(fighter->motion_id==ftKp_MS_SpecialLw||fighter->motion_id==ftKp_MS_SpecialAirLw)){bomb=1;OSReport("Bowser probe: Bowser Bomb frame=%u\n",frame);}
    }
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[9]:NULL;g;g=g->prev){
        if(g->classifier==HSD_GOBJ_CLASS_ITEM&&g->user_data&&
           ((Item*)g->user_data)->kind==It_Kind_Koopa_Flame){
            if(!article){article=1;OSReport("Bowser probe: original flame article active frame=%u\n",frame);}
            if(!effect&&((Item*)g->user_data)->xDD4_itemVar.koopaflame.x44_spawned){
                effect=1;OSReport("Bowser probe: flame effect spawn callback completed frame=%u\n",frame);
            }
        }
    }
    return live&&(!require_specials||(flame&&side&&spin&&bomb&&article&&effect));
}

#include <melee/ft/kinds/ftLuigi/forward.h>
#include <melee/gm/gm_1601.h>
#include <melee/gm/gm_16F1.h>
#include <melee/gm/gmmain_lib.h>
void melee_startup_unlock_luigi(void)
{
    /* Test-only in-memory save state; no persistent save or app defaults change. */
    unsigned bit=gm_CKindToUnlockIndex(CKind_Luigi);
    if(bit>=16)_Exit(6);
    *gmMainLib_GetUnlockedCharactersBitmaskPtr()|=(u16)(1u<<bit);
    OSReport("Luigi probe: unlocked in test memory before CSS\n");
}
int melee_startup_luigi_progress(unsigned frame,int require_specials)
{
    int live=0;static int fire,missile,up,down;static unsigned up_input;
    if(up_input&&frame==up_input+6)melee_startup_publish_combat(0,0,0,0);
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fighter=g->user_data;if(fighter->kind!=Ft_Kind_Luigi)continue;live=1;
        if(require_specials&&!up&&frame>=3150&&frame<3350&&frame>up_input+30&&fighter->motion_id==ftCo_MS_Wait){
            up_input=frame;melee_startup_publish_combat(0,0,80,0x200);
            OSReport("Luigi probe: Super Jump Punch input from Wait frame=%u\n",frame);
        }
        if(!missile&&(fighter->motion_id==ftLg_MS_SpecialS||fighter->motion_id==ftLg_MS_SpecialAirS||fighter->motion_id==ftLg_MS_SpecialSMisfire||fighter->motion_id==ftLg_MS_SpecialAirSMisfire)){missile=1;OSReport("Luigi probe: Green Missile released frame=%u\n",frame);}
        if(!up&&(fighter->motion_id==ftLg_MS_SpecialHi||fighter->motion_id==ftLg_MS_SpecialAirHi)){up=1;OSReport("Luigi probe: Super Jump Punch frame=%u\n",frame);}
        if(!down&&(fighter->motion_id==ftLg_MS_SpecialLw||fighter->motion_id==ftLg_MS_SpecialAirLw)){down=1;OSReport("Luigi probe: Cyclone frame=%u\n",frame);}
    }
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[9]:NULL;g;g=g->prev){
        if(g->classifier==HSD_GOBJ_CLASS_ITEM&&g->user_data&&((Item*)g->user_data)->kind==It_Kind_Luigi_Fire&&!fire){fire=1;OSReport("Luigi probe: original fireball active frame=%u\n",frame);}
    }
    return live&&(!require_specials||(fire&&missile&&up&&down));
}

#include <melee/gr/ground.h>
#include <melee/gr/types.h>
void melee_startup_unlock_dreamland(void)
{
    /* Fixture-only save state makes Dream Land selectable. No save is written. */
    for(unsigned bit=0;bit<16;bit++){
        if(fn_801607A8(bit)==Gr_Kind_OldPupupu){
            *gmMainLib_8015EDA4()|=(u16)(1u<<bit);
            return;
        }
    }
    _Exit(6);
}
int melee_startup_fountain_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Izumi&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_story_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Story&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_stadium_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_PStadium&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_greatbay_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_GreatBay&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_kongo_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Kongo&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_japes_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Garden&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
void melee_startup_results_rematch(unsigned frame)
{
    static int last=-1;static unsigned entered;static int saw_results;
    if(gm_GetCurrentGameMode()!=GM_VS)return;
    int state=gm_GetCurrentSceneIndex();
    if(state!=last){last=state;entered=frame;}
    unsigned elapsed=frame-entered;
    if(state==gmVsMode_State_Results){
        saw_results=1;
        if(elapsed>=60){unsigned phase=(elapsed-60)%120;
            if(phase==0||phase==6){melee_startup_publish_combat(0,0,0,phase==0?0x1000:0);melee_startup_publish_combat(1,0,0,phase==0?0x1000:0);}}
    }else if(saw_results&&state==gmVsMode_State_Css){
        if(elapsed==60||elapsed==66)melee_startup_publish_start(elapsed==60);
    }
}
int melee_startup_icemt_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Icemt&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
#include <melee/it/itzako.h>
int melee_startup_icemt_bear(unsigned frame)
{
    static unsigned entered,active,motions;static int spawned,last=-1,moved;static Vec3 first;
    if(!melee_startup_icemt_progress())return 0;
    if(!entered)entered=frame;
    if(!spawned&&frame-entered>=200){
        HSD_GObj* fighter=plinklow_gobjs?plinklow_gobjs[8]:NULL;
        while(fighter&&(fighter->classifier!=HSD_GOBJ_CLASS_FIGHTER||!fighter->user_data))fighter=fighter->prev;
        if(!fighter)return 0;
        Vec3 pos=((Fighter*)fighter->user_data)->cur_pos;pos.x+=20;pos.y+=25;
        HSD_GObj* bear=it_8027B5B0(It_Kind_Whitebea,&pos,NULL,NULL,1);
        if(!bear){OSReport("Controlled Polar Bear spawn failed\n");_Exit(6);}
        spawned=1;first=pos;OSReport("Controlled Polar Bear spawned frame=%u pos=%f,%f\n",frame,pos.x,pos.y);
    }
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[9]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_ITEM||!g->user_data)continue;
        Item* item=g->user_data;if(item->kind!=It_Kind_Whitebea)continue;
        active++;if(item->msid>=0&&item->msid<32)motions|=1u<<item->msid;
        if(item->msid!=last){OSReport("Polar Bear motion=%d frame=%u pos=%f,%f\n",item->msid,frame,item->pos.x,item->pos.y);last=item->msid;}
        if(fabsf(item->pos.x-first.x)+fabsf(item->pos.y-first.y)>10)moved=1;
    }
    if(frame==5000)OSReport("Polar Bear coverage active_frames=%u motions=%x moved=%d\n",active,motions,moved);
    return spawned&&active>=60&&moved&&(motions&(motions-1));
}
int melee_startup_icemt_scroll(unsigned frame)
{
    static int previous=-1,changes,moved,have,bear;static float first;
    if(!melee_startup_icemt_progress())return 0;
    HSD_GObj* g=Ground_GetMapGObj(9);if(!g||!g->user_data)return 0;
    Ground* p=g->user_data;int lower=p->u.icemt9.x0.ids.under,upper=p->u.icemt9.x0.ids.upper;
    int pair=lower*32+upper;
    if(pair!=previous){if(previous!=-1)changes++;previous=pair;OSReport("Icicle segments lower=%d upper=%d frame=%u\n",lower,upper,frame);}
    HSD_GObj* segment=Ground_GetMapGObj(lower);HSD_JObj* j=segment?segment->hsd_obj:NULL;
    if(j){float y=HSD_JObjGetTranslationY(j);if(!have){have=1;first=y;}if(fabsf(y-first)>20)moved=1;}
    for(HSD_GObj* it=plinklow_gobjs?plinklow_gobjs[9]:NULL;it;it=it->prev){
        if(it->classifier==HSD_GOBJ_CLASS_ITEM&&it->user_data&&((Item*)it->user_data)->kind==It_Kind_Whitebea&&!bear){bear=1;OSReport("Icicle Polar Bear active frame=%u\n",frame);}
    }
    if(frame%600==0)OSReport("Icicle scroll frame=%u changes=%d moved=%d bear=%d speed=%f\n",frame,changes,moved,bear,p->u.icemt9.x0.state.cur);
    return changes>=2&&moved;
}
int melee_startup_fourside_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Fourside&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_fourside_hazards(unsigned frame)
{
    static unsigned masks[3];static int last[3]={-1,-1,-1};
    static int have,moved;static float first;
    if(!melee_startup_fourside_progress())return 0;
    unsigned ids[3]={1,5,3};
    for(unsigned i=0;i<3;i++){
        HSD_GObj* g=Ground_GetMapGObj(ids[i]);if(!g||!g->user_data)return 0;
        Ground* p=g->user_data;
        unsigned st=i==0?p->u.foursideCrane.x0:i==1?p->u.foursideUfo.x0:p->u.fourside2.x0;
        if(st<32)masks[i]|=1u<<st;
        if((int)st!=last[i]){OSReport("Fourside hazard=%u state=%u frame=%u\n",ids[i],st,frame);last[i]=st;}
        if(i==0){float y=p->u.foursideCrane.xC;if(!have){first=y;have=1;}if(fabsf(y-first)>1)moved=1;}
    }
    if(frame==7800)OSReport("Fourside hazards crane=%x ufo=%x helicopter=%x crane_moved=%d\n",masks[0],masks[1],masks[2],moved);
    return moved&&(masks[1]&2)&&(masks[2]&2);
}
int melee_startup_bigblue_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_BigBlue&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_flatzone_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Flatzone&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_kraid_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Kraid&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_pura_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Pura&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_rcruise_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_RCruise&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_pura_platforms(unsigned frame)
{
    static unsigned seen,moved;static int have[25],last[25],changes;static Vec3 first[25];
    if(!melee_startup_pura_progress())return 0;
    HSD_GObj* g=Ground_GetMapGObj(4);if(!g||!g->user_data)return 0;Ground* p=g->user_data;
    for(unsigned i=0;i<25;i++){
        CmSubject* c=p->u.pura3.x128[i];if(!c||!p->u.pura3.xC4[i])continue;
        if(!isfinite(c->pos.x)||!isfinite(c->pos.y)||!isfinite(c->pos.z)){OSReport("Poke Floats nonfinite subject %u\n",i);_Exit(6);}
        int active=c->state==CmSubjectState_Active;
        if(!have[i]){have[i]=1;first[i]=c->pos;last[i]=active;}
        if(active)seen|=1u<<i;
        if(active!=last[i]){changes++;last[i]=active;OSReport("Poke Floats subject=%u active=%d frame=%u\n",i,active,frame);}
        if(fabsf(c->pos.x-first[i].x)+fabsf(c->pos.y-first[i].y)>20)moved|=1u<<i;
    }
    if(frame%600==0)OSReport("Poke Floats coverage frame=%u active_seen=%x moved=%x transitions=%d\n",frame,seen,moved,changes);
    return __builtin_popcount(seen)>=8&&__builtin_popcount(moved)>=8&&changes>=8;
}
int melee_startup_rcruise_traverse(unsigned frame)
{
    static unsigned states;static int previous[8],have,changes,moved;static Vec3 first;
    if(!melee_startup_rcruise_progress())return 0;
    HSD_GObj* camera=Ground_GetMapGObj(3);HSD_GObj* ground=Ground_GetMapGObj(1);
    if(!camera||!camera->user_data||!ground||!ground->user_data)return 0;
    Ground* c=camera->user_data;Ground* p=ground->user_data;
    if(!p->u.rcruise.vanish)return 0;
    if(!have){first=c->u.scroll.x10;for(unsigned i=0;i<8;i++)previous[i]=p->u.rcruise.vanish[i].x0;have=1;}
    if(fabsf(c->u.scroll.x10.x-first.x)+fabsf(c->u.scroll.x10.y-first.y)>100)moved=1;
    for(unsigned i=0;i<8;i++){
        int state=p->u.rcruise.vanish[i].x0;
        if(state>=0&&state<4)states|=1u<<state;
        if(state!=previous[i]){changes++;previous[i]=state;OSReport("Rainbow platform=%u state=%d frame=%u\n",i,state,frame);}
    }
    if(frame%600==0)OSReport("Rainbow route frame=%u camera=%f,%f states=%x changes=%d moved=%d\n",frame,c->u.scroll.x10.x,c->u.scroll.x10.y,states,changes,moved);
    return moved&&changes>=8;
}
int melee_startup_kraid_hazards(unsigned frame)
{
    static unsigned states,attacks;static int last=-1,cycles,moved,have;static float first;
    if(!melee_startup_kraid_progress())return 0;
    HSD_GObj* g=Ground_GetMapGObj(4);if(!g||!g->user_data)return 0;
    Ground* p=g->user_data;int state=p->u.kraid2.x0;
    if(state>=0&&state<5)states|=1u<<state;
    if(state!=last){if(last==4&&state==0)cycles++;OSReport("Kraid state=%d animation=%d frame=%u cycles=%d\n",state,p->u.kraid2.x1,frame,cycles);last=state;}
    if(state==2&&p->u.kraid2.x1>=0&&p->u.kraid2.x1<3)attacks|=1u<<p->u.kraid2.x1;
    g=Ground_GetMapGObj(3);HSD_JObj* j=g?Ground_801C3FA4(g,0):NULL;
    if(j){float z=HSD_JObjGetRotationZ(j);if(!have){first=z;have=1;}if(fabsf(z-first)>0.25f)moved=1;}
    if(frame==7800)OSReport("Kraid coverage states=%x attacks=%x cycles=%d terrain_rotated=%d\n",states,attacks,cycles,moved);
    return states==31&&cycles>=2&&moved;
}
int melee_startup_flatzone_hazards(unsigned frame)
{
    static unsigned shapes, motions, oil;static int last=-2, spill;
    if(!melee_startup_flatzone_progress())return 0;
    HSD_GObj* g=Ground_GetMapGObj(3);
    if(g&&g->user_data){
        Ground* p=g->user_data;int state=p->u.flatzone2.xD0;
        if(state>=0&&state<32)oil|=1u<<state;
        if(state!=last){OSReport("Flat Zone oil state=%d frame=%u\n",state,frame);last=state;}
        if(state==3&&p->u.flatzone2.xCC)spill=1;
    }
    for(g=plinklow_gobjs?plinklow_gobjs[9]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_ITEM||!g->user_data)continue;
        Item* p=g->user_data;if(p->kind!=It_Kind_Tools)continue;
        int shape=p->xDD4_itemVar.tools.x0;
        if(shape>=0&&shape<5&&!(shapes&(1u<<shape))){shapes|=1u<<shape;OSReport("Flat Zone tool shape=%d frame=%u\n",shape,frame);}
        if(p->msid>=0&&p->msid<10)motions|=1u<<p->msid;
    }
    if(frame==7800)OSReport("Flat Zone hazards shapes=%x motions=%x oil=%x spill=%d\n",shapes,motions,oil,spill);
    return shapes&&motions&&(oil&8)&&spill;
}
int melee_startup_bigblue_track(unsigned frame)
{
    static unsigned car_states,flyer_states,platform_states;
    static int have,road_moved,car_moved,last_flyer=-1;
    static Vec3 first_road,first_car;
    if(!melee_startup_bigblue_progress())return 0;
    HSD_GObj* road=Ground_GetMapGObj(34);
    HSD_GObj* cars=Ground_GetMapGObj(33);
    HSD_GObj* flyer=Ground_GetMapGObj(35);
    HSD_GObj* manager=Ground_GetMapGObj(32);
    if(!road||!cars||!flyer||!manager)return 0;
    Ground* r=road->user_data;Ground* c=cars->user_data;
    Ground* f=flyer->user_data;Ground* m=manager->user_data;
    if(!r||!c||!f||!m)return 0;
    for(unsigned i=0;i<4;i++){
        unsigned st=c->u.bigblue.car.lanes[i].state;
        if(st<32)car_states|=1u<<st;
    }
    for(unsigned i=0;i<3;i++){
        unsigned st=m->u.bigblue.manager.data[i].x1;
        if(st<32)platform_states|=1u<<st;
    }
    unsigned st=f->u.bigblue.flyer.state;
    if(st<32)flyer_states|=1u<<st;
    if((int)st!=last_flyer){OSReport("Big Blue flyer state=%u frame=%u\n",st,frame);last_flyer=st;}
    Vec3 rp=r->u.bigblue.road.position,cp=c->u.bigblue.car.lanes[0].pos;
    if(!have){first_road=rp;first_car=cp;have=1;}
    if(fabsf(rp.x-first_road.x)+fabsf(rp.y-first_road.y)>10)road_moved=1;
    if(fabsf(cp.x-first_car.x)+fabsf(cp.y-first_car.y)>10)car_moved=1;
    if(frame%600==0||frame==7800)OSReport("Big Blue track frame=%u car_states=%x flyer_states=%x platform_states=%x road_moved=%d car_moved=%d\n",frame,car_states,flyer_states,platform_states,road_moved,car_moved);
    return road_moved&&car_moved&&(flyer_states&9)==9&&(platform_states&8)&&
        (car_states&(1u<<10));
}
int melee_startup_mutecity_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_MuteCity&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_mutecity_track(unsigned frame)
{
    static unsigned modes,moving;static int last=-1,have_car,car_moved;static Vec3 first;
    if(!melee_startup_mutecity_progress())return 0;
    HSD_GObj* controller=Ground_GetMapGObj(30);if(!controller||!controller->user_data)return 0;
    Ground* gp=controller->user_data;
    unsigned mode=gp->u.mutecity.xD0_flags.b23,active=gp->u.mutecity.xD0_flags.b0;
    modes|=1u<<mode;moving|=1u<<active;
    int state=(int)(mode+4*active);
    if(state!=last){OSReport("Mute City track mode=%u moving=%u command=%d frame=%u\n",mode,active,gp->u.mutecity.xC4,frame);last=state;}
    HSD_GObj* cars=gp->u.mutecity.xCC;
    HSD_JObj* root=cars?cars->hsd_obj:NULL;
    HSD_JObj* car=root?HSD_JObjGetChild(root):NULL;
    if(car){
        Vec3 pos;HSD_JObjGetTranslation(car,&pos);
        if(!have_car){first=pos;have_car=1;}
        else if(!car_moved&&fabsf(pos.x-first.x)+fabsf(pos.y-first.y)+fabsf(pos.z-first.z)>10){
            car_moved=1;OSReport("Mute City car movement frame=%u\n",frame);
        }
    }
    return modes==7&&moving==3&&car_moved;
}
int melee_startup_corneria_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Corneria&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_venom_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Venom&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_corneria_hazards(unsigned frame)
{
    static unsigned lasers;
    if(!melee_startup_corneria_progress())return 0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[9]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_ITEM||!g->user_data)continue;
        Item* ip=g->user_data;
        if(ip->kind==It_Kind_Arwing_Laser||ip->kind==It_Kind_GreatFox_Laser){
            unsigned bit=ip->kind==It_Kind_Arwing_Laser?1:2;
            if(!(lasers&bit))OSReport("Corneria laser kind=%d motion=%d frame=%u\n",ip->kind,ip->msid,frame);
            lasers|=bit;
        }
    }
    return lasers==3;
}
int melee_startup_venom_hazards(unsigned frame)
{
    static int arwing_seen,laser_seen,returned,last=-1;
    if(!melee_startup_venom_progress())return 0;
    HSD_GObj* arwing=Ground_GetMapGObj(2);
    int active=arwing&&arwing->user_data;
    if(active!=last){OSReport("Venom Arwing active=%d frame=%u\n",active,frame);last=active;}
    if(active)arwing_seen=1;
    else if(arwing_seen)returned=1;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[9]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_ITEM||!g->user_data)continue;
        Item* ip=g->user_data;
        if(ip->kind==It_Kind_Arwing_Laser&&!laser_seen){
            laser_seen=1;OSReport("Venom laser motion=%d frame=%u\n",ip->msid,frame);
        }
    }
    return arwing_seen&&laser_seen&&returned;
}
int melee_startup_shrine_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Shrine&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_greens_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Greens&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_greens_blocks(unsigned frame)
{
    static int hit,removed,last_count=-1;
    if(!melee_startup_greens_progress())return 0;
    HSD_GObj* controller=Ground_GetMapGObj(6);
    if(!controller||!controller->user_data)return 0;
    Ground* gp=controller->user_data;
    if(!gp->u.greens.x8_blocks)return 0;
    int count=0;
    for(unsigned row=0;row<5;row++)for(unsigned col=0;col<6;col++){
        struct grGreens_BlockVars* b=&gp->u.greens.x8_blocks[row][col];
        if(b->status)count++;
        if(b->x1C&&!hit){hit=1;OSReport("Greens block hit row=%u col=%u source=%p frame=%u\n",row,col,(void*)b->x1C,frame);}
    }
    if(hit&&last_count>count&&!removed){removed=1;OSReport("Greens block removal count=%d previous=%d frame=%u\n",count,last_count,frame);}
    last_count=count;
    if(frame>=3000&&frame<6000){
        for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
            if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
            Fighter* fp=g->user_data;if(fp->player_id!=0)continue;
            float best=1e9f;Vec3 target={0};
            for(unsigned row=0;row<5;row++)for(unsigned col=0;col<6;col++){
                struct grGreens_BlockVars* b=&gp->u.greens.x8_blocks[row][col];
                if(!b->status||!b->x14)continue;
                Vec3 pos;lb_8000B1CC(b->x14,NULL,&pos);
                float score=fabsf(pos.x-fp->cur_pos.x)+4*fabsf(pos.y-fp->cur_pos.y);
                if(score<best){best=score;target=pos;}
            }
            float dx=target.x-fp->cur_pos.x;
            unsigned phase=(frame-3000)%60;
            if(best<1e8f)melee_startup_publish_combat(0,fabsf(dx)>20?(dx>0?40:-40):phase<4?(dx>0?80:-80):0,0,fabsf(dx)<=20&&phase<4?0x100:0);
            break;
        }
    }
    if(frame==6000)melee_startup_publish_combat(0,0,0,0);
    return hit&&removed;
}
int melee_startup_greens_hazards(unsigned frame)
{
    static unsigned wind,apples;static int last=-1;
    if(!melee_startup_greens_progress())return 0;
    HSD_GObj* tree=Ground_GetMapGObj(5);
    if(tree&&tree->user_data){
        Ground* gp=tree->user_data;
        int state=gp->u.greens2.x4;
        if(state!=last){OSReport("Greens Whispy state=%d wind=%d frame=%u\n",state,gp->u.greens2.x18,frame);last=state;}
        if(gp->u.greens2.x18==1||gp->u.greens2.x18==2){
            unsigned bit=1u<<gp->u.greens2.x18;
            if(!(wind&bit))OSReport("Greens wind direction=%d frame=%u\n",gp->u.greens2.x18,frame);
            wind|=bit;
        }
    }
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[9]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_ITEM||!g->user_data)continue;
        Item* ip=g->user_data;
        if(ip->kind==It_Kind_WhispyApple||ip->kind==It_Kind_WhispyHealApple){
            unsigned bit=ip->kind==It_Kind_WhispyApple?1:2;
            if(!(apples&bit))OSReport("Greens apple kind=%d motion=%d frame=%u\n",ip->kind,ip->msid,frame);
            apples|=bit;
        }
    }
    return wind&&apples;
}
int melee_startup_yorster_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Yorster&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_yorster_blocks(unsigned frame)
{
    static unsigned states[9];static int previous[9]={-1,-1,-1,-1,-1,-1,-1,-1,-1};
    static int completed;
    if(!melee_startup_yorster_progress())return 0;
    HSD_GObj* blocks=Ground_GetMapGObj(1);if(!blocks||!blocks->user_data)return 0;
    Ground* gp=blocks->user_data;
    if(gp->u.yorster.xC4!=0)return 0;
    for(unsigned i=0;i<9;i++){
        struct grYorster_TrackElement* e=&gp->u.yorster.elements[i];unsigned state=e->x01;
        if(state>4||!e->x18||!e->x1C)_Exit(6);
        states[i]|=1u<<state;
        if(previous[i]!=(int)state){OSReport("Yoshi Island block=%u state=%u damage=%.3f frame=%u\n",i,state,e->x04,frame);previous[i]=state;}
        if(!completed&&(states[i]&14)==14&&state==1){completed=1;OSReport("Yoshi Island block break/restore complete block=%u frame=%u\n",i,frame);}
    }
    if(frame>=3000&&frame<5500){
        Vec3 target;lb_8000B1CC(gp->u.yorster.elements[0].x18,NULL,&target);
        for(unsigned i=1;i<9;i++){
            Vec3 pos;lb_8000B1CC(gp->u.yorster.elements[i].x18,NULL,&pos);
            if(pos.y<target.y-0.01f||(fabsf(pos.y-target.y)<0.01f&&fabsf(pos.x)<fabsf(target.x)))target=pos;
        }
        for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
            if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
            Fighter* fp=g->user_data;if(fp->player_id!=0)continue;
            float dx=target.x-fp->cur_pos.x;
            if(fabsf(dx)>12)melee_startup_publish_combat(0,dx>0?40:-40,0,0);
            else {unsigned phase=(frame-3000)%90;melee_startup_publish_combat(0,0,phase<4?-80:0,phase<4?0x100:0);}
            break;
        }
    }
    if(frame==5500)melee_startup_publish_combat(0,0,0,0);
    return completed;
}

int melee_startup_castle_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Castle&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_castle_hazards(unsigned frame)
{
    static int projectile_seen,explosion_seen,returned;
    if(!melee_startup_castle_progress())return 0;
    int active=0;
    for(unsigned i=8;i<=16;i++){
        HSD_GObj* g=Ground_GetMapGObj(i);
        if(g&&g->user_data){
            active++;
            Ground* gp=g->user_data;
            if(!projectile_seen){projectile_seen=1;OSReport("Castle Banzai Bill model=%u camera=%d collision=%d frame=%u\n",i,gp->u.castle11.xD8!=0,gp->u.castle11.xD0!=0,frame);}
        }
    }
    HSD_GObj* explosion=Ground_GetMapGObj(1);
    if(projectile_seen&&explosion&&explosion->user_data&&!explosion_seen){
        Ground* gp=explosion->user_data;
        if(gp->u.castle_explosion.xC4&&gp->u.castle_explosion.xC8){explosion_seen=1;OSReport("Castle Banzai Bill explosion with camera and collision frame=%u\n",frame);}
    }
    if(projectile_seen&&explosion_seen&&!active&&!explosion&&!returned){returned=1;OSReport("Castle Banzai Bill cycle complete frame=%u\n",frame);}
    return returned?1:explosion_seen?2:0;
}

int melee_startup_brinstar_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Zebes&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}
int melee_startup_brinstar_acid(unsigned frame)
{
    static int previous=-1, previous_entry=-1, completed;
    static unsigned states;
    static float low=1000, high=-1000;
    if(!melee_startup_brinstar_progress())return 0;
    HSD_GObj* control=Ground_GetMapGObj(8);
    if(!control||!control->user_data)return 0;
    Ground* gp=control->user_data;
    unsigned state=gp->u.zebes5.xC8;
    int entry=gp->u.zebes5.xC4;
    float level=gp->u.zebes5.xD8;
    if(state>4||entry<0||entry>=30||!isfinite(level))_Exit(6);
    if(level<low)low=level;
    if(level>high)high=level;
    states|=1u<<state;
    if((int)state!=previous||entry!=previous_entry){
        OSReport("Brinstar acid state=%u entry=%d level=%.3f frame=%u\n",state,entry,level,frame);
        previous=state;previous_entry=entry;
    }
    if(!completed&&(states&30)==30&&state==1&&entry>0&&high-low>20){
        completed=1;
        OSReport("Brinstar acid movement cycle complete range=%.3f..%.3f frame=%u\n",low,high,frame);
    }
    return completed;
}

int melee_startup_japes_hazards(unsigned frame)
{
    static int previous=-1,active,returned,cranky_changed;
    if(!melee_startup_japes_progress())return 0;
    HSD_GObj* klap=Ground_GetMapGObj(3);
    if(klap&&klap->user_data){
        Ground* gp=klap->user_data;int state=gp->u.garden2.xc8;
        HSD_GObj* collision=gp->u.garden2.xc4;
        if(!collision||!collision->user_data)_Exit(6);
        Item* item=collision->user_data;
        if(state!=previous){OSReport("Japes Klaptrap state=%d collision motion=%d frame=%u\n",state,item->msid,frame);previous=state;}
        if(state==1&&item->msid==2)active=1;
        if(active&&state==0&&item->msid==0&&!returned){returned=1;OSReport("Japes Klaptrap cycle complete frame=%u\n",frame);}
    }
    HSD_GObj* cranky=Ground_GetMapGObj(1);
    if(cranky&&cranky->user_data&&!cranky_changed){
        Ground* gp=cranky->user_data;
        if(gp->u.garden.xc4!=0){cranky_changed=1;OSReport("Japes Cranky animation=%d frame=%u\n",gp->u.garden.xc4,frame);}
    }
    return returned&&cranky_changed;
}

int melee_startup_greatbay_hazards(unsigned frame)
{
    static int previous=-1,returned,tingle_moved;
    static unsigned states;
    static HSD_GObj* last_tingle;
    static Vec3 last_pos;
    static float tingle_travel;
    if(!melee_startup_greatbay_progress())return 0;
    HSD_GObj* turtle=Ground_GetMapGObj(1);
    if(turtle&&turtle->user_data){
        Ground* gp=turtle->user_data;unsigned state=gp->u.greatbay.xC4;
        if(state>3)_Exit(6);
        if((int)state!=previous){OSReport("Great Bay turtle state=%u frame=%u\n",state,frame);previous=state;}
        states|=1u<<state;if(states==15&&state==0&&!returned){returned=1;OSReport("Great Bay turtle cycle complete frame=%u\n",frame);}
    }
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[9]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_ITEM||!g->user_data)continue;
        Item* ip=g->user_data;if(ip->kind!=It_Kind_Tincle)continue;
        if(!isfinite(ip->pos.x)||!isfinite(ip->pos.y))_Exit(6);
        float step=fmaxf(fabsf(ip->pos.x-last_pos.x),fabsf(ip->pos.y-last_pos.y));
        if(g!=last_tingle||step>10)tingle_travel=0;
        else tingle_travel+=step;
        if(!tingle_moved&&tingle_travel>=10){
            tingle_moved=1;OSReport("Great Bay Tingle continuous travel=%.3f frame=%u\n",tingle_travel,frame);
        }
        last_tingle=g;last_pos=ip->pos;
    }
    return returned&&tingle_moved;
}

#include <melee/gr/grpstadium.h>
int melee_startup_stadium_active_form(void)
{
    if(stage_info.grkind!=Gr_Kind_PStadium||gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs)return -1;
    HSD_GObj* control=Ground_GetMapGObj(2);
    if(!control||!control->user_data)return -1;
    Ground* gp=control->user_data;
    if(gp->u.stadium.xDC||!gp->u.stadium.xE4||!gp->u.stadium.xE4->user_data)return -1;
    return ((Ground*)gp->u.stadium.xE4->user_data)->map_id;
}

int melee_startup_stadium_form_end(unsigned frame)
{
    static int ended,last_form=-1;
    static unsigned last_live_frame;
    int mode=gm_GetCurrentGameMode(),state=gm_GetCurrentSceneIndex();
    if(mode==GM_VS&&state==gmVsMode_State_Vs){
        last_form=melee_startup_stadium_active_form();
        last_live_frame=frame;
    }else if(!ended&&mode==GM_VS&&state==gmVsMode_State_Results&&
             last_live_frame+1==frame&&last_form==6){
        /* match_over is set during scene exit, after the last live frame. */
        ended=1;OSReport("Stadium results after rock form: last live frame=%u results frame=%u\n",last_live_frame,frame);
    }
    return ended;
}

int melee_startup_stadium_transform(unsigned frame,unsigned requested)
{
    static int previous=-1,transformed,returned,configured;
    if(requested&&!configured){grStadium_NativeTestNextForm(requested);configured=1;}
    if(!melee_startup_stadium_progress())return 0;
    HSD_GObj* control=Ground_GetMapGObj(2);if(!control||!control->user_data)return 0;
    Ground* gp=control->user_data;
    int key=gp->u.stadium.xDC*256+gp->u.stadium.xDE;
    if(key!=previous){OSReport("Stadium transformation frame=%u phase=%d target=%d\n",frame,gp->u.stadium.xDC,gp->u.stadium.xDE);previous=key;}
    if(gp->u.stadium.xDC==0&&gp->u.stadium.xE4&&gp->u.stadium.xE4->user_data){
        Ground* form=gp->u.stadium.xE4->user_data;
        if(form->map_id!=5){if(requested&&form->map_id!=requested)_Exit(6);if(!transformed)OSReport("Stadium active form=%d frame=%u\n",form->map_id,frame);transformed=1;}
        else if(transformed&&!returned){returned=1;OSReport("Stadium returned to base frame=%u\n",frame);}
    }
    return transformed&&returned;
}
int melee_startup_story_hazards(unsigned frame)
{
    static unsigned cloud_started,cloud_moved,shyguy_moved;
    static Vec3 cloud_start;
    static HSD_GObj* previous;
    static Vec3 previous_pos;
    if(!melee_startup_story_progress())return 0;
    HSD_GObj* cloud=Ground_GetMapGObj(2);
    if(cloud&&cloud->user_data){
        HSD_JObj* joint=((Ground*)cloud->user_data)->u.randall.jobj;
        if(joint){
            Vec3 pos;lb_8000B1CC(joint,NULL,&pos);
            if(!isfinite(pos.x)||!isfinite(pos.y))_Exit(6);
            if(!cloud_started){cloud_start=pos;cloud_started=1;}
            if(!cloud_moved&&(fabsf(pos.x-cloud_start.x)>10||fabsf(pos.y-cloud_start.y)>10)){
                cloud_moved=1;OSReport("Story Randall moved from %.3f,%.3f to %.3f,%.3f frame=%u\n",cloud_start.x,cloud_start.y,pos.x,pos.y,frame);
            }
        }
    }
    HSD_GObj* first=NULL;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[9]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_ITEM||!g->user_data)continue;
        Item* ip=g->user_data;if(ip->kind!=It_Kind_Heiho)continue;
        if(!isfinite(ip->pos.x)||!isfinite(ip->pos.y)||!isfinite(ip->x40_vel.x))_Exit(6);
        if(!first)first=g;
        if(g==previous&&!shyguy_moved&&fabsf(ip->pos.x-previous_pos.x)>0.1f){
            shyguy_moved=1;OSReport("Story Shy Guy flight x=%.3f -> %.3f velocity=%.3f frame=%u\n",previous_pos.x,ip->pos.x,ip->x40_vel.x,frame);
        }
    }
    previous=first;if(first)previous_pos=((Item*)first->user_data)->pos;
    return cloud_moved&&shyguy_moved;
}
int melee_startup_fountain_platform_progress(unsigned frame)
{
    static unsigned initialized,moved;
    static float initial[2];
    if(!melee_startup_fountain_progress())return 0;
    HSD_GObj* model=Ground_GetMapGObj(3);if(!model||!model->user_data)return 0;
    Ground* gp=model->user_data;HSD_JObj* joints[]={gp->u.izumi.xD0,gp->u.izumi.xD4};
    for(unsigned i=0;i<2;i++){
        if(!joints[i])return 0;
        float y=HSD_JObjGetTranslationY(joints[i]);if(!isfinite(y))_Exit(6);
        if(!(initialized&(1u<<i))){initial[i]=y;initialized|=1u<<i;}
        if(!(moved&(1u<<i))&&fabsf(y-initial[i])>=1.0f){
            moved|=1u<<i;OSReport("Fountain platform %u moved %.3f -> %.3f frame=%u\n",i,initial[i],y,frame);
        }
    }
    return moved==3;
}

int melee_startup_dreamland_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_OldPupupu&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}

#include <melee/ft/ftcoll.h>
int melee_startup_dreamland_wind_progress(unsigned frame)
{
    static unsigned active, affected, stopped;
    if(!melee_startup_dreamland_progress())return 0;
    HSD_GObj* tree=Ground_GetMapGObj(7);if(!tree||!tree->user_data)return 0;
    Ground* ground=tree->user_data;
    unsigned direction=ground->u.oldpupupu.xDC;
    if(direction&&!active){active=frame;OSReport("Dream Land wind active direction=%u frame=%u\n",direction,frame);}
    if(direction&&!affected){
        for(unsigned slot=0;slot<2;slot++){
            HSD_GObj* fighter=Player_GetEntity(slot);if(!fighter)continue;
            Vec3 wind;ftColl_GetWindOffsetVec(fighter,&wind);
            if(wind.x!=0){
                if(!isfinite(wind.x)||wind.y!=0||wind.z!=0)_Exit(6);
                affected=frame;OSReport("Dream Land wind offset slot=%u x=%.4f frame=%u\n",slot,wind.x,frame);break;
            }
        }
    }
    if(active&&!direction&&!stopped){stopped=frame;OSReport("Dream Land wind stopped frame=%u\n",frame);}
    return active&&affected&&stopped;
}

void melee_startup_unlock_fourside(void)
{
    /* Fixture-only save state makes Fourside selectable. No save is written. */
    for(unsigned bit=0;bit<16;bit++){
        if(fn_801607A8(bit)==Gr_Kind_Fourside){
            *gmMainLib_8015EDA4()|=(u16)(1u<<bit);
            return;
        }
    }
    _Exit(6);
}
void melee_startup_unlock_bigblue(void)
{
    /* Fixture-only save state makes Big Blue selectable. No save is written. */
    for(unsigned bit=0;bit<16;bit++){
        if(fn_801607A8(bit)==Gr_Kind_BigBlue){
            *gmMainLib_8015EDA4()|=(u16)(1u<<bit);
            return;
        }
    }
    _Exit(6);
}
void melee_startup_unlock_flatzone(void)
{
    /* Fixture-only save state makes Flat Zone selectable. No save is written. */
    for(unsigned bit=0;bit<16;bit++){
        if(fn_801607A8(bit)==Gr_Kind_Flatzone){
            *gmMainLib_8015EDA4()|=(u16)(1u<<bit);
            return;
        }
    }
    _Exit(6);
}
void melee_startup_unlock_kraid(void)
{
    /* Fixture-only save state makes Brinstar Depths selectable. No save is written. */
    for(unsigned bit=0;bit<16;bit++){
        if(fn_801607A8(bit)==Gr_Kind_Kraid){
            *gmMainLib_8015EDA4()|=(u16)(1u<<bit);
            return;
        }
    }
    _Exit(6);
}
void melee_startup_unlock_pura(void)
{
    /* Fixture-only save state makes Poke Floats selectable. No save is written. */
    for(unsigned bit=0;bit<16;bit++){
        if(fn_801607A8(bit)==Gr_Kind_Pura){
            *gmMainLib_8015EDA4()|=(u16)(1u<<bit);
            return;
        }
    }
    _Exit(6);
}
void melee_startup_unlock_stages(void)
{
    /* Fixture-only save state makes Battlefield selectable. No save is written. */
    for(unsigned bit=0;bit<16;bit++){
        if(fn_801607A8(bit)==Gr_Kind_Battle){
            *gmMainLib_8015EDA4()|=(u16)(1u<<bit);
            return;
        }
    }
    _Exit(6);
}
int melee_startup_battle_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Battle&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}

void melee_startup_unlock_final(void)
{
    /* Fixture-only save state makes Final Destination selectable. No save is written. */
    for(unsigned bit=0;bit<16;bit++){
        if(fn_801607A8(bit)==Gr_Kind_Last){
            *gmMainLib_8015EDA4()|=(u16)(1u<<bit);
            return;
        }
    }
    _Exit(6);
}
int melee_startup_final_progress(void)
{
    int fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    return stage_info.grkind==Gr_Kind_Last&&fighters>=2&&gm_GetCurrentGameMode()==GM_VS&&gm_GetCurrentSceneIndex()==gmVsMode_State_Vs&&!gmVs_GetController_0()->match_over;
}

int melee_startup_battle_background_progress(unsigned frame)
{
    static int transitioning, fading, completed, previous=-1;
    if(!melee_startup_battle_progress())return 0;
    HSD_GObj* g=Ground_GetMapGObj(3);if(!g||!g->user_data)return 0;
    Ground* ground=g->user_data;
    if(ground->u.battle_bg.state==1&&!transitioning){transitioning=1;OSReport("Battlefield background: transition animation frame=%u\n",frame);}
    if(ground->u.battle_bg.state==2&&!fading){
        fading=1;previous=ground->u.battle_bg.prev;
        OSReport("Battlefield background: fading %d -> %d frame=%u\n",previous,ground->u.battle_bg.curr,frame);
    }
    if(fading&&ground->u.battle_bg.state==0&&!completed){
        if(ground->u.battle_bg.curr==previous||Ground_GetMapGObj(previous)||!Ground_GetMapGObj(ground->u.battle_bg.curr))return 0;
        completed=1;OSReport("Battlefield background: old model removed, new model active frame=%u\n",frame);
    }
    return transitioning&&fading&&completed;
}

int melee_startup_final_background_progress(unsigned frame)
{
    static unsigned seen;static int previous=-1;
    if(!melee_startup_final_progress())return 0;
    HSD_GObj* g=Ground_GetMapGObj(3);if(!g||!g->user_data)return 0;
    Ground* ground=g->user_data;unsigned state=ground->u.map.xC4_b2_25;
    if(state<1||state>17)return 0;
    seen|=1u<<state;
    if(previous!=(int)state){OSReport("Final Destination background: state=%u frame=%u timer=%.0f\n",state,frame,ground->u.map.xC8);previous=state;}
    return (seen&14u)==14u;
}

void melee_startup_unlock_pichu(void)
{
    unsigned bit=gm_CKindToUnlockIndex(CKind_Pichu);if(bit>=16)_Exit(6);
    *gmMainLib_GetUnlockedCharactersBitmaskPtr()|=(u16)(1u<<bit);
}
int melee_startup_pichu_progress(void)
{
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;if(fp->player_id==0&&fp->kind==Ft_Kind_Pichu)return 1;
    }
    return 0;
}

int melee_startup_pikachu_progress(void)
{
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;if(fp->player_id==0&&fp->kind==Ft_Kind_Pikachu)return 1;
    }
    return 0;
}

void melee_startup_unlock_marth(void)
{
    unsigned bit=gm_CKindToUnlockIndex(CKind_Mars);if(bit>=16)_Exit(6);
    *gmMainLib_GetUnlockedCharactersBitmaskPtr()|=(u16)(1u<<bit);
}
int melee_startup_marth_progress(void)
{
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;if(fp->player_id==0&&fp->kind==Ft_Kind_Mars)return 1;
    }
    return 0;
}

void melee_startup_unlock_roy(void)
{
    unsigned bit=gm_CKindToUnlockIndex(CKind_Emblem);if(bit>=16)_Exit(6);
    *gmMainLib_GetUnlockedCharactersBitmaskPtr()|=(u16)(1u<<bit);
}
int melee_startup_roy_progress(void)
{
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;if(fp->player_id==0&&fp->kind==Ft_Kind_Emblem)return 1;
    }
    return 0;
}

#include <melee/ft/kinds/ftMars/forward.h>
int melee_startup_marth_specials(unsigned frame)
{
    static unsigned seen,last_input,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(!melee_startup_marth_progress())return 0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;if(fp->player_id!=0||fp->kind!=Ft_Kind_Mars)continue;
        unsigned bit=0;int state=fp->motion_id;
        if(state==ftMs_MS_SpecialNEnd0||state==ftMs_MS_SpecialNEnd1||state==ftMs_MS_SpecialAirNEnd0||state==ftMs_MS_SpecialAirNEnd1)bit=1;
        if(state==ftMs_MS_SpecialS1||state==ftMs_MS_SpecialAirS1)bit=2;
        if(state==ftMs_MS_SpecialLw||state==ftMs_MS_SpecialAirLw)bit=4;
        if(state==ftMs_MS_SpecialHi||state==ftMs_MS_SpecialAirHi)bit=8;
        if(bit&&!(seen&bit)){seen|=bit;OSReport("Marth special: mask=%u state=%d frame=%u\n",seen,state,frame);}
        if(frame>=3000&&frame<4800&&frame>last_input+90&&state==ftCo_MS_Wait&&seen!=15){
            unsigned move=!(seen&1)?1:!(seen&2)?2:!(seen&4)?4:8;
            melee_startup_publish_combat(0,move==2?80:0,move==4?-80:move==8?80:0,PAD_BUTTON_B);
            last_input=frame;release=frame+(move==1?30:6);
            OSReport("Marth special input: move=%u frame=%u\n",move,frame);
        }
    }
    return seen==15;
}

int melee_startup_roy_specials(unsigned frame)
{
    static unsigned seen,last_input,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(!melee_startup_roy_progress())return 0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;if(fp->player_id!=0||fp->kind!=Ft_Kind_Emblem)continue;
        unsigned bit=0;int state=fp->motion_id;
        if(state==ftMs_MS_SpecialNEnd0||state==ftMs_MS_SpecialNEnd1||state==ftMs_MS_SpecialAirNEnd0||state==ftMs_MS_SpecialAirNEnd1)bit=1;
        if(state==ftMs_MS_SpecialS1||state==ftMs_MS_SpecialAirS1)bit=2;
        if(state==ftMs_MS_SpecialLw||state==ftMs_MS_SpecialAirLw)bit=4;
        if(state==ftMs_MS_SpecialHi||state==ftMs_MS_SpecialAirHi)bit=8;
        if(bit&&!(seen&bit)){seen|=bit;OSReport("Roy special: mask=%u state=%d frame=%u\n",seen,state,frame);}
        if(frame>=3000&&frame<4800&&frame>last_input+90&&state==ftCo_MS_Wait&&seen!=15){
            unsigned move=!(seen&1)?1:!(seen&2)?2:!(seen&4)?4:8;
            melee_startup_publish_combat(0,move==2?80:0,move==4?-80:move==8?80:0,PAD_BUTTON_B);
            last_input=frame;release=frame+(move==1?30:6);
            OSReport("Roy special input: move=%u frame=%u\n",move,frame);
        }
    }
    return seen==15;
}

#include <melee/ft/kinds/ftPikachu/forward.h>
int melee_startup_pikachu_specials(unsigned frame, int pichu)
{
    static unsigned seen,last_input,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(!(pichu?melee_startup_pichu_progress():melee_startup_pikachu_progress()))return 0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;if(fp->player_id!=0||fp->kind!=(pichu?Ft_Kind_Pichu:Ft_Kind_Pikachu))continue;
        unsigned bit=0;int state=fp->motion_id;
        if(state==ftPk_MS_SpecialN||state==ftPk_MS_SpecialAirN)bit=1;
        if(state==ftPk_MS_SpecialS1||state==ftPk_MS_SpecialAirS1)bit=2;
        if(state==ftPk_MS_SpecialLwEnd||state==ftPk_MS_SpecialAirLwEnd)bit=4;
        if(state==ftPk_MS_SpecialHiStart1||state==ftPk_MS_SpecialAirHiStart1)bit=8;
        if(bit&&!(seen&bit)){seen|=bit;OSReport("Pikachu/Pichu special: mask=%u state=%d frame=%u\n",seen,state,frame);}
        if(frame>=3000&&frame<4800&&frame>last_input+150&&state==ftCo_MS_Wait&&seen!=15){
            unsigned move=!(seen&1)?1:!(seen&2)?2:!(seen&4)?4:8;
            melee_startup_publish_combat(0,move==2?80:0,move==4?-80:move==8?80:0,PAD_BUTTON_B);
            last_input=frame;release=frame+(move==2?30:6);
            OSReport("Pikachu/Pichu special input: move=%u frame=%u\n",move,frame);
        }
    }
    return seen==15;
}

#include <melee/mn/mnmain.h>
#include <melee/mn/mnevent.h>
#include <melee/db/db.h>
int melee_startup_event_preview(unsigned frame)
{
    static u64 seen;
    static int previous=-1;
    if(frame<720)return 0;
    if(frame==720)DbLevel=DbLKind_DebugDevelop+1;
    if(gm_GetCurrentGameMode()!=GM_MENU)return 0;
    int selected=mnEvent_NativeSelected();
    if(selected>=0&&selected<51){
        seen|=1ULL<<selected;
        if(selected!=previous){OSReport("Event preview selected=%d frame=%u\n",selected,frame);previous=selected;}
    }
    if(frame%60==6)publish(0);
    if(frame%60==0){
        unsigned menu=mn_804A04F0.cur_menu,hover=mn_804A04F0.hovered_selection;
        if(menu==MENU_KIND_MAIN)publish(hover==SEL_MAIN_1P?PAD_BUTTON_A:PAD_BUTTON_UP);
        else if(menu==MENU_KIND_1P)publish(hover==SEL_1P_EVENT?PAD_BUTTON_A:PAD_BUTTON_DOWN);
        else if(menu==MENU_KIND_EVENT&&selected>=0&&selected<50)publish(PAD_BUTTON_DOWN);
    }
    return seen==((1ULL<<51)-1);
}

void melee_startup_unlock_purin(void)
{
    unsigned bit=gm_CKindToUnlockIndex(CKind_Purin);if(bit>=16)_Exit(6);
    *gmMainLib_GetUnlockedCharactersBitmaskPtr()|=(u16)(1u<<bit);
}
int melee_startup_purin_progress(void)
{
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;if(fp->player_id==0&&fp->kind==Ft_Kind_Purin)return 1;
    }
    return 0;
}


int melee_startup_purin_hat_progress(void)
{
    if(!melee_startup_purin_progress())return 0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;
        if(fp->player_id==0&&fp->kind==Ft_Kind_Purin)
            {
                int costume=getenv("MELEE_TEST_PURIN_COSTUME")?atoi(getenv("MELEE_TEST_PURIN_COSTUME")):1;
                if(fp->x619_costume_id!=costume||!fp->u.pr.x223C||!fp->u.pr.x2240.count||!fp->u.pr.x2240.data)return 0;
                if(costume>=2){
                    if(fp->dynamics_num!=3)return 0;
                    for(unsigned i=1;i<3;i++)if(fp->dynamic_bone_sets[i].dyn_desc.count!=(costume==2?3u:5u)||!fp->dynamic_bone_sets[i].dyn_desc.data)return 0;
                }
                fprintf(stderr,"Jigglypuff costume %d: hat and %d live physics chains passed\n",costume,fp->dynamics_num);return 1;
            }
    }
    return 0;
}

#include <melee/ft/kinds/ftPurin/forward.h>
int melee_startup_purin_specials(unsigned frame)
{
    static unsigned seen,jumps,last_input,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(!melee_startup_purin_progress())return 0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;if(fp->player_id!=0||fp->kind!=Ft_Kind_Purin)continue;
        unsigned bit=0;int state=fp->motion_id;
        if(state==ftPr_MS_SpecialNRelease||state==ftPr_MS_SpecialAirNChargeRelease)bit=1;
        if(state==ftPr_MS_SpecialS||state==ftPr_MS_SpecialAirS)bit=2;
        if(state==ftPr_MS_SpecialHiL||state==ftPr_MS_SpecialHiR||state==ftPr_MS_SpecialAirHiL||state==ftPr_MS_SpecialAirHiR)bit=4;
        if(state==ftPr_MS_SpecialLwL||state==ftPr_MS_SpecialLwR||state==ftPr_MS_SpecialAirLwL||state==ftPr_MS_SpecialAirLwR)bit=8;
        if(bit&&!(seen&bit)){seen|=bit;OSReport("Jigglypuff special: mask=%u state=%d frame=%u\n",seen,state,frame);}
        if(state>=ftPr_MS_JumpAerialF1&&state<=ftPr_MS_JumpAerialF5){
            unsigned j=1u<<(state-ftPr_MS_JumpAerialF1);
            if(!(jumps&j)){jumps|=j;OSReport("Jigglypuff jumps: mask=%u used=%u state=%d frame=%u\n",jumps,fp->x1968_jumpsUsed,state,frame);}
        }
        if(frame>=3000&&frame<6800&&seen!=15&&frame>last_input+150&&state==ftCo_MS_Wait){
            unsigned move=!(seen&1)?1:!(seen&2)?2:!(seen&4)?4:8;
            melee_startup_publish_combat(0,move==2?80:0,move==4?80:move==8?-80:0,PAD_BUTTON_B);
            last_input=frame;release=frame+(move==1?90:6);
            OSReport("Jigglypuff special input: move=%u frame=%u\n",move,frame);
        }
        if(frame>=3000&&frame<6800&&seen==15&&jumps!=31&&frame>last_input+30){
            melee_startup_publish_combat(0,0,0,PAD_BUTTON_X);last_input=frame;release=frame+6;
        }
    }
    return seen==15&&jumps==31;
}

int melee_startup_yoshi_progress(void)
{
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;if(fp->player_id==0&&fp->kind==Ft_Kind_Yoshi)return 1;
    }
    return 0;
}

#include <melee/mn/mncharsel.h>
void melee_startup_select_yoshi(unsigned frame)
{
    static unsigned settled,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(release||frame<1020||frame>=1190||gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=0)return;
    float dx,dy;if(!mnCharSel_NativeTargetDelta(0,CKind_Yoshi,&dx,&dy))return;
    int x=dx>0.7f?45:dx< -0.7f?-45:0,y=dy>0.7f?45:dy< -0.7f?-45:0;
    if(x||y){settled=0;melee_startup_publish_combat(0,x,y,0);}
    else if(++settled>=6){melee_startup_publish_combat(0,0,0,PAD_BUTTON_A);release=frame+6;OSReport("Yoshi selection confirmed at frame %u\n",frame);}
    else melee_startup_publish_combat(0,0,0,0);
}

#include <melee/ft/kinds/ftYoshi/forward.h>
int melee_startup_yoshi_specials(unsigned frame)
{
    static unsigned seen,last_input,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(!melee_startup_yoshi_progress())return 0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;if(fp->player_id!=0||fp->kind!=Ft_Kind_Yoshi)continue;
        unsigned bit=0;int state=fp->motion_id;
        if(state>=ftYs_MS_SpecialN1&&state<=ftYs_MS_SpecialAirN2_1)bit=1;
        if(state==ftYs_MS_SpecialAirSLoop_0||state==ftYs_MS_SpecialAirSLoop_1||state==ftYs_MS_SpecialAirSLoop_2||state==ftYs_MS_SpecialAirSLoop_3)bit=2;
        if(state==ftYs_MS_SpecialHi||state==ftYs_MS_SpecialAirHi)bit=4;
        if(state==ftYs_MS_SpecialLwLanding)bit=8;
        if(state==ftYs_MS_GuardHold)bit=16;
        if(bit&&!(seen&bit)){seen|=bit;OSReport("Yoshi special: mask=%u state=%d frame=%u\n",seen,state,frame);}
        if(frame>=3000&&frame<6800&&seen!=31&&frame>last_input+180&&state==ftCo_MS_Wait){
            unsigned move=!(seen&1)?1:!(seen&2)?2:!(seen&4)?4:!(seen&8)?8:16;
            melee_startup_publish_combat(0,move==2?80:0,move==4?80:move==8?-80:0,move==16?PAD_TRIGGER_R:PAD_BUTTON_B);
            last_input=frame;release=frame+(move==16?45:move==4?30:6);
            OSReport("Yoshi special input: move=%u frame=%u\n",move,frame);
        }
    }
    return seen==31;
}

int melee_startup_yoshi_capture(unsigned frame)
{
    static unsigned captured,escaped,release,last_attempt;
    static float captured_timer;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(!melee_startup_yoshi_progress())return 0;
    Fighter* yoshi=NULL;Fighter* victim=NULL;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;
        if(fp->player_id==0&&fp->kind==Ft_Kind_Yoshi)yoshi=fp;
        if(fp->player_id==1&&fp->kind==Ft_Kind_Ness)victim=fp;
    }
    if(!yoshi||!victim)return 0;
    if(victim->motion_id==ftCo_MS_YoshiEgg){
        if(!victim->x20A0_accessory||!victim->invisible||victim->x597_bits!=33){
            OSReport("Egg Lay capture missing accessory, hidden fighter or shared skeleton\n");_Exit(6);
        }
        if(!captured){captured=frame;captured_timer=victim->grab_timer;OSReport("Egg Lay captured Ness: frame=%u timer=%f skeleton=%u\n",frame,captured_timer,victim->x597_bits);}
    }else if(captured&&!escaped&&frame>captured&& !victim->invisible&&victim->motion_id==ftCo_MS_Wait){
        escaped=frame;OSReport("Egg Lay escaped and returned to Wait: frame=%u\n",frame);
    }
    if(frame>=3000&&frame<6800&&!captured&&frame>last_attempt+90){
        float dx=victim->cur_pos.x-yoshi->cur_pos.x,dy=victim->cur_pos.y-yoshi->cur_pos.y;
        if(dx>14||dx< -14||dy>15){
            melee_startup_publish_combat(0,dx>3?60:dx< -3?-60:0,0,dy>15?PAD_BUTTON_X:0);
            release=frame+12;
        }else if(yoshi->motion_id==ftCo_MS_Wait){
            melee_startup_publish_combat(0,0,0,PAD_BUTTON_B);release=frame+6;
        }
        last_attempt=frame;
    }
    return captured&&escaped;
}
int melee_startup_doctor_progress(unsigned frame,int require_specials)
{
    int mario_seen=0;
    static int fireball_seen, cape_seen, up_seen, down_seen, reflector_seen;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data&&
           ((Fighter*)g->user_data)->kind==Ft_Kind_DrMario){
            Fighter* fighter=g->user_data;mario_seen=1;
            if(fighter->reflecting&&!reflector_seen){reflector_seen=1;OSReport("Dr. Mario probe: cape reflector active frame=%u\n",frame);}
            if((fighter->motion_id==ftMr_MS_SpecialHi||fighter->motion_id==ftMr_MS_SpecialAirHi)&&!up_seen){up_seen=1;OSReport("Dr. Mario probe: Super Jump Punch frame=%u\n",frame);}
            if((fighter->motion_id==ftMr_MS_SpecialLw||fighter->motion_id==ftMr_MS_SpecialAirLw)&&!down_seen){down_seen=1;OSReport("Dr. Mario probe: Mario Tornado frame=%u\n",frame);}
        }
    }
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[9]:NULL;g;g=g->prev){
        if(g->classifier==HSD_GOBJ_CLASS_ITEM&&g->user_data&&
           ((Item*)g->user_data)->kind==It_Kind_DrMario_Vitamin&&!fireball_seen){
            fireball_seen=1;OSReport("Dr. Mario probe: original pill spawned frame=%u\n",frame);
        }
    }
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[9]:NULL;g;g=g->prev){
        if(g->classifier==HSD_GOBJ_CLASS_ITEM&&g->user_data&&
           ((Item*)g->user_data)->kind==It_Kind_DrMario_Sheet&&!cape_seen){
            cape_seen=1;OSReport("Dr. Mario probe: original sheet spawned frame=%u\n",frame);
        }
    }
    return mario_seen&&(!require_specials||(fireball_seen&&cape_seen&&up_seen&&down_seen&&reflector_seen));
}

void melee_startup_unlock_doctor(void)
{
    unsigned bit=gm_CKindToUnlockIndex(CKind_DrMario);if(bit>=16)_Exit(6);
    *gmMainLib_GetUnlockedCharactersBitmaskPtr()|=(u16)(1u<<bit);
}
void melee_startup_select_doctor(unsigned frame)
{
    static unsigned settled,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(release||frame<1020||frame>=1190||gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=0)return;
    float dx,dy;if(!mnCharSel_NativeTargetDelta(0,CKind_DrMario,&dx,&dy))return;
    int x=dx>0.7f?45:dx< -0.7f?-45:0,y=dy>0.7f?45:dy< -0.7f?-45:0;
    if(x||y){settled=0;melee_startup_publish_combat(0,x,y,0);}
    else if(++settled>=6){melee_startup_publish_combat(0,0,0,PAD_BUTTON_A);release=frame+6;OSReport("Dr. Mario selection confirmed at frame %u\n",frame);}
    else melee_startup_publish_combat(0,0,0,0);
}
int melee_startup_ganon_progress(unsigned frame,int require_specials)
{
    int live=0;static int punch,boost,dive,kick;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fighter=g->user_data;if(fighter->kind!=Ft_Kind_Ganon)continue;
        live=1;
        if(!boost&&(fighter->motion_id==ftCa_MS_SpecialSStart||fighter->motion_id==ftCa_MS_SpecialAirSStart)){boost=1;OSReport("Ganondorf probe: Gerudo Dragon frame=%u\n",frame);}
        if(!dive&&(fighter->motion_id==ftCa_MS_SpecialHi||fighter->motion_id==ftCa_MS_SpecialAirHi)){dive=1;OSReport("Ganondorf probe: Dark Dive frame=%u\n",frame);}
        if(!kick&&(fighter->motion_id==ftCa_MS_SpecialLw||fighter->motion_id==ftCa_MS_SpecialAirLw)){kick=1;OSReport("Ganondorf probe: Wizard Foot frame=%u\n",frame);}
        if(!punch&&(fighter->motion_id==ftCa_MS_SpecialN||fighter->motion_id==ftCa_MS_SpecialAirN)){
            punch=1;OSReport("Ganondorf probe: Warlock Punch frame=%u\n",frame);
        }
    }
    return live&&(!require_specials||(punch&&boost&&dive&&kick));
}
void melee_startup_unlock_ganon(void)
{
    unsigned bit=gm_CKindToUnlockIndex(CKind_Ganon);if(bit>=16)_Exit(6);
    *gmMainLib_GetUnlockedCharactersBitmaskPtr()|=(u16)(1u<<bit);
}
void melee_startup_select_ganon(unsigned frame)
{
    static unsigned settled,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(release||frame<1020||frame>=1190||gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=0)return;
    float dx,dy;if(!mnCharSel_NativeTargetDelta(0,CKind_Ganon,&dx,&dy))return;
    int x=dx>0.7f?45:dx< -0.7f?-45:0,y=dy>0.7f?45:dy< -0.7f?-45:0;
    if(x||y){settled=0;melee_startup_publish_combat(0,x,y,0);}
    else if(++settled>=6){melee_startup_publish_combat(0,0,0,PAD_BUTTON_A);release=frame+6;OSReport("Ganondorf selection confirmed at frame %u\n",frame);}
    else melee_startup_publish_combat(0,0,0,0);
}
void melee_startup_select_zelda(unsigned frame)
{
    static unsigned settled,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(release||frame<1020||frame>=1190||gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=0)return;
    float dx,dy;if(!mnCharSel_NativeTargetDelta(0,CKind_Zelda,&dx,&dy))return;
    int x=dx>0.7f?45:dx< -0.7f?-45:0,y=dy>0.7f?45:dy< -0.7f?-45:0;
    if(x||y){settled=0;melee_startup_publish_combat(0,x,y,0);}
    else if(++settled>=6){melee_startup_publish_combat(0,0,0,PAD_BUTTON_A);release=frame+6;OSReport("Zelda selection confirmed at frame %u\n",frame);}
    else melee_startup_publish_combat(0,0,0,0);
}
int melee_startup_zelda_progress(unsigned frame,int roundtrip)
{
    static unsigned phase,release,next_input;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    HSD_GObj* g=Player_GetEntity(0);if(!g||!g->user_data)return 0;
    Fighter* fp=g->user_data;
    if((phase==0&&fp->kind==Ft_Kind_Zelda)||(phase==1&&fp->kind==Ft_Kind_Seak)||(phase==2&&fp->kind==Ft_Kind_Zelda)){
        phase++;OSReport("Zelda/Sheik transformation phase %u active kind %d frame %u\n",phase,fp->kind,frame);
    }
    /* Use ordinary PAD input only when the active fighter can accept a move. */
    if(roundtrip&&phase>0&&phase<3&&frame>=3000&&frame>=next_input&&fp->motion_id==ftCo_MS_Wait){
        melee_startup_publish_combat(0,0,-80,PAD_BUTTON_B);release=frame+6;next_input=frame+180;
        OSReport("Zelda/Sheik down-special input frame %u kind %d\n",frame,fp->kind);
    }
    return roundtrip?phase==3:(fp->kind==Ft_Kind_Zelda||fp->kind==Ft_Kind_Seak);
}

#include <melee/ft/kinds/ftZelda/forward.h>
#include <melee/ft/kinds/ftSeak/forward.h>
int melee_startup_zelda_specials(unsigned frame)
{
    static unsigned moves[2],items,phase,release,next_input;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    HSD_GObj* g=Player_GetEntity(0);if(!g||!g->user_data)return 0;
    Fighter* fp=g->user_data;int sk=fp->kind==Ft_Kind_Seak;
    if(!sk&&fp->kind!=Ft_Kind_Zelda)return 0;
    int state=fp->motion_id;unsigned bit=0;
    if(!sk){
        if(state==ftZd_MS_SpecialN||state==ftZd_MS_SpecialAirN)bit=1;
        if(state==ftZd_MS_SpecialSLoop||state==ftZd_MS_SpecialAirSLoop)bit=2;
        if(state==ftZd_MS_SpecialHi||state==ftZd_MS_SpecialAirHi)bit=4;
    }else{
        if(state==ftSk_MS_SpecialNEnd||state==ftSk_MS_SpecialAirNEnd)bit=1;
        if(state==ftSk_MS_SpecialS||state==ftSk_MS_SpecialAirS)bit=2;
        if(state==ftSk_MS_SpecialHi||state==ftSk_MS_SpecialAirHi)bit=4;
    }
    if(bit&&!(moves[sk]&bit)){moves[sk]|=bit;OSReport("Zelda/Sheik special kind=%d mask=%u state=%d frame=%u\n",fp->kind,moves[sk],state,frame);}
    const ItemKind kinds[]={It_Kind_Zelda_DinFire,It_Kind_Zelda_DinFire_Explode,It_Kind_Seak_NeedleThrow,It_Kind_Seak_NeedleHeld,It_Kind_Seak_Chain,It_Kind_Seak_Vanish};
    for(HSD_GObj* it=plinklow_gobjs?plinklow_gobjs[9]:NULL;it;it=it->prev){
        if(it->classifier!=HSD_GOBJ_CLASS_ITEM||!it->user_data)continue;
        Item* ip=it->user_data;
        for(unsigned i=0;i<6;i++)if(ip->kind==kinds[i]&&!(items&(1u<<i))){items|=1u<<i;OSReport("Zelda/Sheik article kind=%d mask=%u frame=%u\n",ip->kind,items,frame);}
    }
    if(phase==0&&sk){phase=1;OSReport("Special probe transformed to Sheik frame=%u\n",frame);}
    if(phase==1&&!sk){phase=2;OSReport("Special probe transformed back to Zelda frame=%u\n",frame);}
    if(frame>=1800&&frame<7800&&frame>=next_input&&phase<2){
        int loop=sk&&(state==ftSk_MS_SpecialNLoop||state==ftSk_MS_SpecialAirNLoop);
        if(state==ftCo_MS_Wait||loop){
            unsigned move=!(moves[sk]&1)||(sk&&!(items&4))?1:!(moves[sk]&2)||(!sk&&(items&3)!=3)||(sk&&!(items&16))?2:!(moves[sk]&4)||(sk&&!(items&32))?4:8;
            melee_startup_publish_combat(0,move==2?80:0,move==4?80:move==8?-80:0,PAD_BUTTON_B);
            release=frame+(move==2?60:move==4?30:6);next_input=frame+180;
            OSReport("Zelda/Sheik special input kind=%d move=%u frame=%u\n",fp->kind,move,frame);
        }
    }
    return phase==2&&moves[0]==7&&moves[1]==7&&items==63;
}

void melee_startup_select_link(unsigned frame,int young)
{
    static unsigned settled,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(release||frame<1020||frame>=1190||gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=0)return;
    float dx,dy;if(!mnCharSel_NativeTargetDelta(0,young?CKind_CLink:CKind_Link,&dx,&dy))return;
    int x=dx>0.7f?45:dx< -0.7f?-45:0,y=dy>0.7f?45:dy< -0.7f?-45:0;
    if(x||y){settled=0;melee_startup_publish_combat(0,x,y,0);}
    else if(++settled>=6){melee_startup_publish_combat(0,0,0,PAD_BUTTON_A);release=frame+6;OSReport("Link selection confirmed at frame %u\n",frame);}
    else melee_startup_publish_combat(0,0,0,0);
}

#include <melee/ft/kinds/ftLink/forward.h>
int melee_startup_link_specials(unsigned frame,int young,int extras)
{
    static unsigned moves,items,release,next_input;
    static unsigned air_input,air_seen,air_done,milk_seen,milk_done;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    HSD_GObj* g=Player_GetEntity(0);if(!g||!g->user_data)return 0;
    Fighter* fp=g->user_data;
    if(fp->kind!=(young?Ft_Kind_CLink:Ft_Kind_Link))return 0;
    int state=fp->motion_id;unsigned bit=0;
    if(state==ftLk_MS_SpecialNEnd||state==ftLk_MS_SpecialAirNEnd)bit=1;
    if(state==ftLk_MS_SpecialS1||state==ftLk_MS_SpecialAirS1)bit=2;
    if(state==ftLk_MS_SpecialHi||state==ftLk_MS_SpecialAirHi)bit=4;
    if(state==ftLk_MS_SpecialLw||state==ftLk_MS_SpecialAirLw)bit=8;
    if(state==ftCo_MS_Catch||state==ftLk_MS_AirCatch)bit=16;
    if(bit&&!(moves&bit)){moves|=bit;OSReport("Link special young=%d mask=%u state=%d frame=%u\n",young,moves,state,frame);}
    const ItemKind kinds[]={young?It_Kind_CLink_Arrow:It_Kind_Link_Arrow,young?It_Kind_CLink_Bow:It_Kind_Link_Bow,young?It_Kind_CLink_Boomerang:It_Kind_Link_Boomerang,young?It_Kind_CLink_Bomb:It_Kind_Link_Bomb,young?It_Kind_CLink_HShot:It_Kind_Link_HShot};
    for(HSD_GObj* it=plinklow_gobjs?plinklow_gobjs[9]:NULL;it;it=it->prev){
        if(it->classifier!=HSD_GOBJ_CLASS_ITEM||!it->user_data)continue;
        Item* ip=it->user_data;
        for(unsigned i=0;i<5;i++)if(ip->kind==kinds[i]&&!(items&(1u<<i))){items|=1u<<i;OSReport("Link article young=%d kind=%d mask=%u frame=%u\n",young,ip->kind,items,frame);}
        if(extras&&state==ftLk_MS_AirCatch&&ip->kind==kinds[4]&&!air_seen){air_seen=1;OSReport("Link airborne hookshot young=%d frame=%u\n",young,frame);}
        if(extras&&young&&ip->kind==It_Kind_CLink_Milk&&!milk_seen){milk_seen=1;OSReport("Young Link milk created frame=%u\n",frame);}
    }
    if(extras&&air_seen&&!air_done&&state==ftCo_MS_Wait){air_done=1;OSReport("Link returned to idle after airborne hookshot young=%d frame=%u\n",young,frame);}
    if(extras&&milk_seen&&!milk_done&&state==ftCo_MS_Wait&&fp->u.lk.x18==NULL){milk_done=1;OSReport("Young Link milk detached after taunt frame=%u\n",frame);}
    if(extras&&frame==air_input){
        melee_startup_publish_combat(0,0,0,PAD_TRIGGER_Z);release=frame+6;
    }
    if(extras&&moves==31&&items==31&&frame>=next_input&&frame<6600&&state==ftCo_MS_Wait){
        if(fp->item_gobj){melee_startup_publish_combat(0,0,0,PAD_BUTTON_A);release=frame+6;next_input=frame+180;}
        else if(!air_done){melee_startup_publish_combat(0,0,0,PAD_BUTTON_X);release=frame+6;air_input=frame+20;next_input=frame+180;}
        else if(young&&!milk_done){melee_startup_publish_combat(0,0,0,PAD_BUTTON_UP);release=frame+6;next_input=frame+300;}
    }
    if(frame>=1800&&frame<6800&&frame>=next_input&&state==ftCo_MS_Wait&&(moves!=31||items!=31)){
        unsigned move=!(moves&1)||(items&3)!=3?1:!(moves&2)||!(items&4)?2:!(moves&4)?4:!(moves&16)||!(items&16)?16:8;
        melee_startup_publish_combat(0,move==2?80:0,move==4?80:move==8?-80:0,move==16?PAD_TRIGGER_Z:PAD_BUTTON_B);
        release=frame+(move==1?45:6);next_input=frame+180;
        OSReport("Link special input young=%d move=%u frame=%u\n",young,move,frame);
    }
    return moves==31&&items==31&&(!extras||(air_done&&(!young||milk_done)));
}

int melee_startup_link_progress(int young)
{
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    HSD_GObj* g=Player_GetEntity(0);if(!g||!g->user_data)return 0;
    Fighter* fp=g->user_data;return fp->kind==(young?Ft_Kind_CLink:Ft_Kind_Link);
}

void melee_startup_unlock_young_link(void)
{
    unsigned bit=gm_CKindToUnlockIndex(CKind_CLink);if(bit>=16)_Exit(6);
    *gmMainLib_GetUnlockedCharactersBitmaskPtr()|=(u16)(1u<<bit);
}

void melee_startup_unlock_gamewatch(void)
{
    unsigned bit=gm_CKindToUnlockIndex(CKind_GameWatch);if(bit>=16)_Exit(6);
    *gmMainLib_GetUnlockedCharactersBitmaskPtr()|=(u16)(1u<<bit);
}

void melee_startup_select_gamewatch(unsigned frame)
{
    static unsigned settled,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(release||frame<1020||frame>=1190||gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=0)return;
    float dx,dy;if(!mnCharSel_NativeTargetDelta(0,CKind_GameWatch,&dx,&dy))return;
    int x=dx>0.7f?45:dx< -0.7f?-45:0,y=dy>0.7f?45:dy< -0.7f?-45:0;
    if(x||y){settled=0;melee_startup_publish_combat(0,x,y,0);}
    else if(++settled>=6){melee_startup_publish_combat(0,0,0,PAD_BUTTON_A);release=frame+6;OSReport("GameWatch selection confirmed at frame %u\n",frame);}
    else melee_startup_publish_combat(0,0,0,0);
}

int melee_startup_gamewatch_progress(void)
{
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    HSD_GObj* g=Player_GetEntity(0);return g&&g->user_data&&((Fighter*)g->user_data)->kind==Ft_Kind_GameWatch;
}

void melee_startup_unlock_mewtwo(void)
{
    unsigned bit=gm_CKindToUnlockIndex(CKind_Mewtwo);if(bit>=16)_Exit(6);
    *gmMainLib_GetUnlockedCharactersBitmaskPtr()|=(u16)(1u<<bit);
}

void melee_startup_select_iceclimbers(unsigned frame)
{
    static unsigned settled,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(release||frame<1020||frame>=1190||gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=0)return;
    float dx,dy;if(!mnCharSel_NativeTargetDelta(0,CKind_PopoNana,&dx,&dy))return;
    int x=dx>0.7f?45:dx< -0.7f?-45:0,y=dy>0.7f?45:dy< -0.7f?-45:0;
    if(x||y){settled=0;melee_startup_publish_combat(0,x,y,0);}
    else if(++settled>=6){melee_startup_publish_combat(0,0,0,PAD_BUTTON_A);release=frame+6;OSReport("Popo selection confirmed at frame %u\n",frame);}
    else melee_startup_publish_combat(0,0,0,0);
}

int melee_startup_iceclimbers_progress(void)
{
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    HSD_GObj* g=Player_GetEntityAtIndex(0,0);HSD_GObj* partner=Player_GetEntityAtIndex(0,1);return g&&g->user_data&&((Fighter*)g->user_data)->kind==Ft_Kind_Popo&&partner&&partner->user_data&&((Fighter*)partner->user_data)->kind==Ft_Kind_Nana;
}

void melee_startup_select_mewtwo(unsigned frame)
{
    static unsigned settled,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(release||frame<1020||frame>=1190||gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=0)return;
    float dx,dy;if(!mnCharSel_NativeTargetDelta(0,CKind_Mewtwo,&dx,&dy))return;
    int x=dx>0.7f?45:dx< -0.7f?-45:0,y=dy>0.7f?45:dy< -0.7f?-45:0;
    if(x||y){settled=0;melee_startup_publish_combat(0,x,y,0);}
    else if(++settled>=6){melee_startup_publish_combat(0,0,0,PAD_BUTTON_A);release=frame+6;OSReport("Mewtwo selection confirmed at frame %u\n",frame);}
    else melee_startup_publish_combat(0,0,0,0);
}

int melee_startup_mewtwo_progress(void)
{
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    HSD_GObj* g=Player_GetEntity(0);return g&&g->user_data&&((Fighter*)g->user_data)->kind==Ft_Kind_Mewtwo;
}

void melee_startup_select_peach(unsigned frame)
{
    static unsigned settled,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(release||frame<1020||frame>=1190||gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=0)return;
    float dx,dy;if(!mnCharSel_NativeTargetDelta(0,CKind_Peach,&dx,&dy))return;
    int x=dx>0.7f?45:dx< -0.7f?-45:0,y=dy>0.7f?45:dy< -0.7f?-45:0;
    if(x||y){settled=0;melee_startup_publish_combat(0,x,y,0);}
    else if(++settled>=6){melee_startup_publish_combat(0,0,0,PAD_BUTTON_A);release=frame+6;OSReport("Peach selection confirmed at frame %u\n",frame);}
    else melee_startup_publish_combat(0,0,0,0);
}

int melee_startup_peach_progress(void)
{
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    HSD_GObj* g=Player_GetEntity(0);return g&&g->user_data&&((Fighter*)g->user_data)->kind==Ft_Kind_Peach;
}

void melee_startup_select_samus(unsigned frame)
{
    static unsigned settled,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(release||frame<1020||frame>=1190||gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=0)return;
    float dx,dy;if(!mnCharSel_NativeTargetDelta(0,CKind_Samus,&dx,&dy))return;
    int x=dx>0.7f?45:dx< -0.7f?-45:0,y=dy>0.7f?45:dy< -0.7f?-45:0;
    if(x||y){settled=0;melee_startup_publish_combat(0,x,y,0);}
    else if(++settled>=6){melee_startup_publish_combat(0,0,0,PAD_BUTTON_A);release=frame+6;OSReport("Samus selection confirmed at frame %u\n",frame);}
    else melee_startup_publish_combat(0,0,0,0);
}

int melee_startup_samus_progress(void)
{
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    HSD_GObj* g=Player_GetEntity(0);return g&&g->user_data&&((Fighter*)g->user_data)->kind==Ft_Kind_Samus;
}

#include <melee/ft/kinds/ftSamus/forward.h>
int melee_startup_samus_specials(unsigned frame)
{
    static unsigned moves,items,release,next_input,air_input,side_input,charged,air_done;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(!melee_startup_samus_progress())return 0;
    Fighter* fp=Player_GetEntity(0)->user_data;int state=fp->motion_id;unsigned bit=0;
    if(state==ftSs_MS_SpecialNHold&&!charged){charged=1;OSReport("Samus charging frame=%u\n",frame);}
    if(state==ftSs_MS_SpecialN||state==ftSs_MS_SpecialAirN)bit=1;
    if(state==ftSs_MS_SpecialS||state==ftSs_MS_SpecialAirS)bit=2;
    if(state==ftSs_MS_SpecialSSmash||state==ftSs_MS_SpecialAirSSmash)bit=4;
    if(state==ftSs_MS_SpecialHi||state==ftSs_MS_SpecialAirHi)bit=8;
    if(state==ftSs_MS_SpecialLwBomb||state==ftSs_MS_SpecialAirLwBomb)bit=16;
    if(state==ftCo_MS_Catch)bit=32;
    if(state==ftSs_MS_AirCatch)bit=64;
    if(bit&&!(moves&bit)){moves|=bit;OSReport("Samus special mask=%u state=%d frame=%u\n",moves,state,frame);}
    const ItemKind kinds[]={It_Kind_Samus_Charge,It_Kind_Samus_Missile,It_Kind_Samus_Bomb,It_Kind_Samus_GBeam};
    for(HSD_GObj* it=plinklow_gobjs?plinklow_gobjs[9]:NULL;it;it=it->prev){
        if(it->classifier!=HSD_GOBJ_CLASS_ITEM||!it->user_data)continue;Item* ip=it->user_data;
        for(unsigned i=0;i<4;i++)if(ip->kind==kinds[i]&&!(items&(1u<<i))){items|=1u<<i;OSReport("Samus article kind=%d mask=%u frame=%u\n",ip->kind,items,frame);}
    }
    if((moves&64)&&!air_done&&state==ftCo_MS_Wait){air_done=1;OSReport("Samus returned from airborne grapple frame=%u\n",frame);}
    if(frame==side_input){melee_startup_publish_combat(0,80,0,PAD_BUTTON_B);release=frame+6;}
    if(frame==air_input){melee_startup_publish_combat(0,0,0,PAD_TRIGGER_Z);release=frame+6;}
    if(frame>=1800&&frame<6800&&frame>=next_input&&(state==ftCo_MS_Wait||state==ftSs_MS_SpecialNHold)&&!(moves==127&&items==15&&air_done)){
        unsigned move=!(moves&1)||!(items&1)?1:!(moves&2)?2:!(moves&4)?4:!(moves&8)?8:!(moves&16)||!(items&4)?16:!(moves&32)||!(items&8)?32:64;
        if(move==2){melee_startup_publish_combat(0,80,0,0);side_input=frame+8;}
        else if(move==64){melee_startup_publish_combat(0,0,0,PAD_BUTTON_X);air_input=frame+20;}
        else melee_startup_publish_combat(0,move==2?40:move==4?80:0,move==8?80:move==16?-80:0,move==32?PAD_TRIGGER_Z:PAD_BUTTON_B);
        release=frame+(move==2?14:6);next_input=frame+180;OSReport("Samus special input move=%u frame=%u\n",move,frame);
    }
    return moves==127&&items==15&&charged&&air_done;
}

#include <melee/ft/kinds/ftPeach/forward.h>
int melee_startup_peach_specials(unsigned frame)
{
    static unsigned moves,items,release,next_input,returned;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(!melee_startup_peach_progress())return 0;
    Fighter* fp=Player_GetEntity(0)->user_data;int state=fp->motion_id;unsigned bit=0;
    if(state==ftPe_MS_SpecialN||state==ftPe_MS_SpecialAirN)bit=1;
    if(state==ftPe_MS_SpecialSStart||state==ftPe_MS_SpecialAirSStart)bit=2;
    if(state==ftPe_MS_SpecialHiStart||state==ftPe_MS_SpecialAirHiStart)bit=4;
    if(state==ftPe_MS_SpecialLw||state==ftPe_MS_SpecialAirLw)bit=8;
    if(bit&&!(moves&bit)){moves|=bit;OSReport("Peach special mask=%u state=%d frame=%u\n",moves,state,frame);}
    const ItemKind kinds[]={It_Kind_Peach_Toad,It_Kind_Peach_Parasol,It_Kind_Peach_Turnip,It_Kind_Peach_Explode,It_Kind_Peach_ToadSpore};
    for(HSD_GObj* it=plinklow_gobjs?plinklow_gobjs[9]:NULL;it;it=it->prev){
        if(it->classifier!=HSD_GOBJ_CLASS_ITEM||!it->user_data)continue;Item* ip=it->user_data;
        for(unsigned i=0;i<5;i++)if(ip->kind==kinds[i]&&!(items&(1u<<i))){items|=1u<<i;OSReport("Peach article kind=%d mask=%u frame=%u\n",ip->kind,items,frame);}
    }
    if(moves==15&&(items&7)==7&&state==ftCo_MS_Wait&&!returned){returned=1;OSReport("Peach returned to idle after specials frame=%u\n",frame);}
    if(frame>=1800&&frame<6800&&frame>=next_input&&state==ftCo_MS_Wait&&!returned){
        unsigned move=!(moves&1)||!(items&1)?1:!(moves&2)?2:!(moves&4)||!(items&2)?4:8;
        Fighter* opponent=Player_GetEntity(1)?Player_GetEntity(1)->user_data:NULL;
        int direction=opponent&&opponent->cur_pos.x<fp->cur_pos.x?-80:80;
        melee_startup_publish_combat(0,move==2?direction:0,move==4?80:move==8?-80:0,PAD_BUTTON_B);
        release=frame+6;next_input=frame+180;OSReport("Peach special input move=%u frame=%u\n",move,frame);
    }
    return returned;
}

int melee_startup_peach_float(unsigned frame)
{
    static unsigned phase,release,next_input,floated,attacked,landed;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(!melee_startup_peach_progress())return 0;
    Fighter* fp=Player_GetEntity(0)->user_data;int state=fp->motion_id;
    if(state==ftPe_MS_Float&&!floated){floated=1;OSReport("Peach float entered frame=%u\n",frame);}
    if(state==ftPe_MS_FloatAttackAirN&&!attacked){attacked=1;OSReport("Peach float neutral attack frame=%u\n",frame);}
    if(attacked&&state==ftCo_MS_Wait&&!landed){landed=1;OSReport("Peach landed after float attack frame=%u\n",frame);}
    if(phase==0&&state==ftCo_MS_Wait){
        melee_startup_publish_combat(0,0,0,PAD_TRIGGER_Z);release=frame+6;next_input=frame+90;phase=1;
    }else if(phase==1&&frame>=next_input&&state==ftCo_MS_Wait){
        melee_startup_publish_combat(0,0,-80,PAD_BUTTON_X);release=frame+150;phase=2;
    }else if(phase==2&&floated&&state==ftPe_MS_Float){
        melee_startup_publish_combat(0,0,0,PAD_BUTTON_X|PAD_BUTTON_A);release=frame+40;phase=3;
    }
    return landed;
}

#include <melee/ft/kinds/ftMewtwo/forward.h>
int melee_startup_mewtwo_specials(unsigned frame)
{
    static unsigned moves,items,release,next_input,charged,returned,teleported;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(!melee_startup_mewtwo_progress())return 0;
    Fighter* fp=Player_GetEntity(0)->user_data;int state=fp->motion_id;unsigned bit=0;
    if((state==ftMt_MS_SpecialNLoop||state==ftMt_MS_SpecialAirNLoop)&&!charged){charged=1;OSReport("Mewtwo charging frame=%u\n",frame);}
    if(state==ftMt_MS_SpecialNEnd||state==ftMt_MS_SpecialAirNEnd)bit=1;
    if(state==ftMt_MS_SpecialS||state==ftMt_MS_SpecialAirS)bit=2;
    if(state==ftMt_MS_SpecialHiStart||state==ftMt_MS_SpecialAirHiStart)bit=4;
    if(state==ftMt_MS_SpecialLw||state==ftMt_MS_SpecialAirLw)bit=8;
    if((state==ftMt_MS_SpecialHi||state==ftMt_MS_SpecialAirHi)&&!teleported){teleported=1;OSReport("Mewtwo teleport exit frame=%u\n",frame);}
    if(bit&&!(moves&bit)){moves|=bit;OSReport("Mewtwo special mask=%u state=%d frame=%u\n",moves,state,frame);}
    const ItemKind kinds[]={It_Kind_Mewtwo_ShadowBall,It_Kind_Mewtwo_Disable};
    for(HSD_GObj* it=plinklow_gobjs?plinklow_gobjs[9]:NULL;it;it=it->prev){
        if(it->classifier!=HSD_GOBJ_CLASS_ITEM||!it->user_data)continue;Item* ip=it->user_data;
        for(unsigned i=0;i<2;i++)if(ip->kind==kinds[i]&&!(items&(1u<<i))){items|=1u<<i;OSReport("Mewtwo article kind=%d mask=%u frame=%u\n",ip->kind,items,frame);}
    }
    if(moves==15&&items==3&&charged&&teleported&&state==ftCo_MS_Wait&&!returned){returned=1;OSReport("Mewtwo returned to idle after specials frame=%u\n",frame);}
    if(frame>=1800&&frame<6800&&frame>=next_input&&(state==ftCo_MS_Wait||state==ftMt_MS_SpecialNLoop||state==ftMt_MS_SpecialNLoopFull)&&!returned){
        unsigned move=!(moves&1)?1:!(moves&2)?2:!(moves&4)?4:8;
        melee_startup_publish_combat(0,move==2?80:0,move==4?80:move==8?-80:0,PAD_BUTTON_B);
        release=frame+6;next_input=frame+180;OSReport("Mewtwo special input move=%u frame=%u\n",move,frame);
    }
    return returned;
}

#include <melee/ft/kinds/ftPopo/forward.h>
int melee_startup_iceclimbers_specials(unsigned frame)
{
    static unsigned moves,items,release,next_input,paired,returned;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(!melee_startup_iceclimbers_progress())return 0;
    Fighter* fp=Player_GetEntityAtIndex(0,0)->user_data;Fighter* nana=Player_GetEntityAtIndex(0,1)->user_data;
    int state=fp->motion_id;unsigned bit=0;
    if(state==ftPp_MS_SpecialN||state==ftPp_MS_SpecialAirN)bit=1;
    if(state==ftPp_MS_SpecialS1||state==ftPp_MS_SpecialS2||state==ftPp_MS_SpecialAirS1||state==ftPp_MS_SpecialAirS2)bit=2;
    if(state>=ftPp_MS_SpecialHiStart_0&&state<=ftPp_MS_SpecialAirHiThrow_1)bit=4;
    if(state==ftPp_MS_SpecialLw||state==ftPp_MS_SpecialAirLw)bit=8;
    if((nana->motion_id==ftPp_MS_SpecialS_0||nana->motion_id==ftPp_MS_SpecialS_1)&&!paired){paired=1;OSReport("Ice Climbers paired side special frame=%u\n",frame);}
    if(bit&&!(moves&bit)){moves|=bit;OSReport("Ice Climbers special mask=%u popo=%d nana=%d frame=%u\n",moves,state,nana->motion_id,frame);}
    const ItemKind kinds[]={It_Kind_IceClimber_Ice,It_Kind_IceClimber_Blizzard,It_Kind_IceClimber_GumStrings};
    for(HSD_GObj* it=plinklow_gobjs?plinklow_gobjs[9]:NULL;it;it=it->prev){
        if(it->classifier!=HSD_GOBJ_CLASS_ITEM||!it->user_data)continue;Item* ip=it->user_data;
        for(unsigned i=0;i<3;i++)if(ip->kind==kinds[i]&&!(items&(1u<<i))){items|=1u<<i;OSReport("Ice Climbers article kind=%d mask=%u frame=%u\n",ip->kind,items,frame);}
    }
    if(moves==15&&items==7&&paired&&state==ftCo_MS_Wait&&nana->motion_id==ftCo_MS_Wait&&!returned){returned=1;OSReport("Ice Climbers both idle after specials frame=%u\n",frame);}
    if(frame>=1800&&frame<6800&&frame>=next_input&&state==ftCo_MS_Wait&&!returned){
        unsigned move=!(moves&1)||!(items&1)?1:!(moves&8)||!(items&2)?8:!(moves&2)||!paired?2:4;
        melee_startup_publish_combat(0,move==2?80:0,move==4?80:move==8?-80:0,PAD_BUTTON_B);
        release=frame+(move==8?45:6);next_input=frame+240;OSReport("Ice Climbers input move=%u frame=%u\n",move,frame);
    }
    return returned;
}

#include <melee/ft/kinds/ftGameWatch/forward.h>
int melee_startup_gamewatch_specials(unsigned frame)
{
    static unsigned moves,items,release,next_input,returned;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(!melee_startup_gamewatch_progress())return 0;
    Fighter* fp=Player_GetEntity(0)->user_data;int state=fp->motion_id;unsigned bit=0;
    if(state==ftGw_MS_SpecialN||state==ftGw_MS_SpecialAirN)bit=1;
    if(state>=ftGw_MS_SpecialS1&&state<=ftGw_MS_SpecialAirS9)bit=2;
    if(state==ftGw_MS_SpecialHi||state==ftGw_MS_SpecialAirHi)bit=4;
    if(state==ftGw_MS_SpecialLw||state==ftGw_MS_SpecialAirLw)bit=8;
    if(bit&&!(moves&bit)){moves|=bit;OSReport("Game Watch special mask=%u state=%d frame=%u\n",moves,state,frame);}
    const ItemKind kinds[]={It_Kind_GameWatch_Chef,It_Kind_GameWatch_Judge,It_Kind_GameWatch_Rescue,It_Kind_GameWatch_Panic};
    for(HSD_GObj* it=plinklow_gobjs?plinklow_gobjs[9]:NULL;it;it=it->prev){
        if(it->classifier!=HSD_GOBJ_CLASS_ITEM||!it->user_data)continue;Item* ip=it->user_data;
        for(unsigned i=0;i<4;i++)if(ip->kind==kinds[i]&&!(items&(1u<<i))){items|=1u<<i;OSReport("Game Watch article kind=%d mask=%u frame=%u\n",ip->kind,items,frame);}
    }
    if(moves==15&&(items&7)==7&&state==ftCo_MS_Wait&&!returned){returned=1;OSReport("Game Watch idle after specials frame=%u\n",frame);}
    if(frame>=1800&&frame<6800&&frame>=next_input&&state==ftCo_MS_Wait&&!returned){
        unsigned move=!(moves&1)||!(items&1)?1:!(moves&2)||!(items&2)?2:!(moves&8)?8:4;
        melee_startup_publish_combat(0,move==2?80:0,move==4?80:move==8?-80:0,PAD_BUTTON_B);
        release=frame+6;next_input=frame+200;OSReport("Game Watch input move=%u frame=%u\n",move,frame);
    }
    return returned;
}

void melee_startup_select_kirby(unsigned frame)
{
    static unsigned settled,release;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(release||frame<1020||frame>=1190||gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=0)return;
    float dx,dy;if(!mnCharSel_NativeTargetDelta(0,CKind_Kirby,&dx,&dy))return;
    int x=dx>0.7f?45:dx< -0.7f?-45:0,y=dy>0.7f?45:dy< -0.7f?-45:0;
    if(x||y){settled=0;melee_startup_publish_combat(0,x,y,0);}
    else if(++settled>=6){melee_startup_publish_combat(0,0,0,PAD_BUTTON_A);release=frame+6;OSReport("Kirby selection confirmed at frame %u\n",frame);}
    else melee_startup_publish_combat(0,0,0,0);
}

int melee_startup_kirby_progress(void)
{
    if(gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=gmVsMode_State_Vs||gmVs_GetController_0()->match_over)return 0;
    HSD_GObj* g=Player_GetEntity(0);return g&&g->user_data&&((Fighter*)g->user_data)->kind==Ft_Kind_Kirby;
}


#include <melee/ft/kinds/ftKirby/forward.h>
int melee_startup_kirby_specials(unsigned frame)
{
    static unsigned moves,items,release,next_input,returned;
    if(frame==release)melee_startup_publish_combat(0,0,0,0);
    if(!melee_startup_kirby_progress())return 0;
    Fighter* fp=Player_GetEntity(0)->user_data;int state=fp->motion_id;unsigned bit=0;
    if(state==ftKb_MS_SpecialN||state==ftKb_MS_SpecialAirN)bit=1;
    if(state==ftKb_MS_SpecialS||state==ftKb_MS_SpecialAirS)bit=2;
    if(state>=ftKb_MS_SpecialHi1&&state<=ftKb_MS_SpecialAirHi3)bit=4;
    if(state==ftKb_MS_SpecialLw||state==ftKb_MS_SpecialAirLw)bit=8;
    if(bit&&!(moves&bit)){moves|=bit;OSReport("Kirby special mask=%u state=%d frame=%u\n",moves,state,frame);}
    const ItemKind kinds[]={It_Kind_Kirby_CBeam,It_Kind_Kirby_Hammer};
    for(HSD_GObj* it=plinklow_gobjs?plinklow_gobjs[9]:NULL;it;it=it->prev){
        if(it->classifier!=HSD_GOBJ_CLASS_ITEM||!it->user_data)continue;Item* ip=it->user_data;
        for(unsigned i=0;i<2;i++)if(ip->kind==kinds[i]&&!(items&(1u<<i))){items|=1u<<i;OSReport("Kirby article kind=%d mask=%u frame=%u\n",ip->kind,items,frame);}
    }
    if(moves==15&&items==3&&state==ftCo_MS_Wait&&!returned){returned=1;OSReport("Kirby idle after specials frame=%u\n",frame);}
    if(frame>=1800&&frame<6800&&frame>=next_input&&state==ftCo_MS_Wait&&!returned){
        unsigned move=!(moves&1)?1:!(moves&2)||!(items&2)?2:!(moves&4)||!(items&1)?4:8;
        melee_startup_publish_combat(0,move==2?80:0,move==4?80:move==8?-80:0,PAD_BUTTON_B);
        release=frame+6;next_input=frame+240;OSReport("Kirby input move=%u frame=%u\n",move,frame);
    }
    return returned;
}

int melee_startup_kirby_copy(unsigned frame,unsigned variant)
{
 static float punch_damage_before,breath_low;
 static unsigned release,next_input,captured,copied,items,returned,discard_requested,discarded,transform_release,transform_retry,counter_release,counter_retry;
 const FighterKind targets[]={Ft_Kind_Ness,Ft_Kind_Mario,Ft_Kind_DrMario,Ft_Kind_Luigi,Ft_Kind_Fox,Ft_Kind_Falco,Ft_Kind_Donkey,Ft_Kind_Captain,Ft_Kind_Ganon,Ft_Kind_Mars,Ft_Kind_Emblem,Ft_Kind_Zelda,Ft_Kind_Seak,Ft_Kind_Peach,Ft_Kind_Popo,Ft_Kind_Samus,Ft_Kind_Pikachu,Ft_Kind_Pichu,Ft_Kind_Koopa,Ft_Kind_Mewtwo,Ft_Kind_Link,Ft_Kind_CLink,Ft_Kind_Purin,Ft_Kind_Yoshi,Ft_Kind_GameWatch};if(variant>24)return 0;FighterKind target=targets[variant];
 if(frame==release)melee_startup_publish_combat(0,0,0,0);
 if(!melee_startup_kirby_progress())return 0;
 Fighter*fp=Player_GetEntity(0)->user_data;HSD_GObj*other=Player_GetEntity(1);if(!other||!other->user_data)return 0;Fighter*victim=other->user_data;
 if(variant==12){
  if(frame==transform_release)melee_startup_publish_combat(1,0,0,0);
  if(victim->kind!=Ft_Kind_Seak){
   if(frame>=1800&&frame>=transform_retry&&victim->motion_id==ftCo_MS_Wait){
    melee_startup_publish_combat(1,0,-80,PAD_BUTTON_B);transform_release=frame+6;transform_retry=frame+180;
    OSReport("Kirby copy opponent transform input frame=%u\n",frame);
   }
   return 0;
  }
 }
 if(fp->victim_gobj&&!captured){captured=frame;OSReport("Kirby copy captured victim frame=%u state=%d\n",frame,fp->motion_id);}
 if(fp->u.kb.hat.kind==target&&!copied){
  if((variant==5||variant==6||variant==19||variant==22||variant==24)?!fp->u.kb.hat.x14.count:!fp->u.kb.hat.jobj){OSReport("Kirby copied ability without hat model\n");_Exit(6);}
  copied=frame;OSReport("Kirby acquired hat kind=%d frame=%u\n",target,frame);
 }
 const ItemKind projectiles[]={It_Kind_Kirby_NessPKFlush,It_Kind_Kirby_MarioFire,It_Kind_Kirby_DrMarioVitamin,It_Kind_Kirby_LuigiFire,It_Kind_Kirby_FoxLaser,It_Kind_Kirby_FalcoLaser,0,0,0,0,0,0,It_Kind_Kirby_SeakNeedleThrow,It_Kind_Kirby_PeachToad,It_Kind_Kirby_IceClimberIce,It_Kind_Kirby_SamusCharge,It_Kind_Kirby_PikachuTJolt_Air,It_Kind_Kirby_PichuTJolt_Air,It_Kind_Kirby_KoopaFlame,It_Kind_Kirby_MewtwoShadowBall,It_Kind_Kirby_LinkArrow,It_Kind_Kirby_CLinkArrow,0,It_Kind_Kirby_YoshiEggLay,It_Kind_Kirby_GameWatchChef};
 const ItemKind kinds[]={projectiles[variant],variant==24?It_Kind_Kirby_GameWatchChefPan:variant==21?It_Kind_Kirby_CLinkBow:variant==20?It_Kind_Kirby_LinkBow:variant==17?It_Kind_Kirby_PichuTJolt_Ground:variant==16?It_Kind_Kirby_PikachuTJolt_Ground:variant==13?It_Kind_Kirby_PeachToadSpore:variant==12?It_Kind_Kirby_SeakNeedleHeld:variant==5?It_Kind_Kirby_FalcoBlaster:variant==4?It_Kind_Kirby_FoxBlaster:It_Kind_Kirby_NessPKFlush_Explode};
 unsigned count=variant==24?2:variant==23?0:variant==22?0:variant>=20?2:variant>=18?1:variant>=16?2:variant>=14?1:variant>=12?2:variant>=6?0:variant&&variant<4?1:2,required=variant==24?3:variant==23?3:variant==22?7:variant>=20?15:variant==19?7:variant==18?31:variant>=16?3:variant==15?63:variant==13?15:variant>=9?7:variant>=7?3:variant==6?31:count==1?1:3;
 if(variant==6&&copied){
  if(fp->motion_id==ftKb_MS_DkSpecialNLoop&&!(items&1)){items|=1;OSReport("Kirby copied punch charging frame=%u\n",frame);}
  if(fp->motion_id==ftKb_MS_DkSpecialN&&!(items&2)){items|=2;OSReport("Kirby copied partial punch released frame=%u\n",frame);}
  if(fp->motion_id==ftKb_MS_DkSpecialNCancel&&!(items&4)){
   if(fp->u.kb.xBC<=0){OSReport("Kirby charge cancellation lost stored charge\n");_Exit(6);}
   items|=4;OSReport("Kirby copied punch cancelled with charge=%d frame=%u\n",fp->u.kb.xBC,frame);
  }
  if((items&4)&&fp->motion_id==ftCo_MS_Wait&&fp->u.kb.xBC>=((ftKb_DatAttrs*)fp->dat_attrs)->specialn_dk_swings_to_full_charge&&!(items&8)){
   items|=8;OSReport("Kirby copied punch full charge stored frame=%u\n",frame);
  }
  if(fp->motion_id==ftKb_MS_DkSpecialNFull&&!(items&16)){items|=16;OSReport("Kirby copied full punch released frame=%u\n",frame);}
 }
 if(variant==7&&copied&&fp->motion_id==ftKb_MS_CaSpecialN&&!items){items=1;OSReport("Kirby copied Falcon Punch activated frame=%u\n",frame);}
 if(variant==8&&copied&&fp->motion_id==ftKb_MS_GnSpecialN&&!items){items=1;OSReport("Kirby copied Warlock Punch activated frame=%u\n",frame);}
 if((variant==7||variant==8)&&(items&1)&&!(items&2)&&
    fp->motion_id==(variant==7?ftKb_MS_CaSpecialN:ftKb_MS_GnSpecialN)&&victim->dmg.x1830_percent>punch_damage_before){
  items|=2;OSReport("Kirby copied punch hit frame=%u damage=%.1f->%.1f\n",frame,punch_damage_before,victim->dmg.x1830_percent);
 }
 if((variant==9||variant==10)&&copied){
  int loop=variant==9?ftKb_MS_MsSpecialNLoop:ftKb_MS_FeSpecialNLoop;
  int end=variant==9?ftKb_MS_MsSpecialNEnd0:ftKb_MS_FeSpecialNEnd0;
  if(fp->x20A0_accessory&&!(items&1)){items|=1;OSReport("Kirby copied sword attached frame=%u dynamics=%d\n",frame,fp->dynamics_num);}
  if(fp->motion_id==loop&&!(items&2)){items|=2;OSReport("Kirby copied sword charging frame=%u\n",frame);}
  if((fp->motion_id==end||fp->motion_id==end+1)&&!(items&4)){items|=4;OSReport("Kirby copied sword released frame=%u\n",frame);}
 }
 if(variant==11&&copied){
  if(fp->motion_id==ftKb_MS_ZdSpecialN&&!(items&1)){items|=1;OSReport("Kirby copied Nayrus Love activated frame=%u dynamics=%d\n",frame,fp->dynamics_num);}
  if(fp->reflecting&&!(items&2)){items|=2;OSReport("Kirby copied reflector active frame=%u\n",frame);}
  if((items&2)&&!fp->reflecting&&!(items&4)){items|=4;OSReport("Kirby copied reflector ended frame=%u\n",frame);}
 }
 if(variant==12&&copied&&fp->motion_id==ftKb_MS_SkSpecialNLoop&&!(items&4)){
  items|=4;OSReport("Kirby copied needles charging frame=%u dynamics=%d\n",frame,fp->dynamics_num);
 }
 if(variant==13&&copied){
  if(frame==counter_release)melee_startup_publish_combat(1,0,0,0);
  if(fp->motion_id==ftKb_MS_PeSpecialLw&&!(items&4)){items|=4;OSReport("Kirby copied Toad activated frame=%u\n",frame);}
  if(fp->motion_id==ftKb_MS_PeSpecialLwHit&&!(items&8)){items|=8;OSReport("Kirby copied Toad counter triggered frame=%u\n",frame);}
  if(fp->motion_id==ftKb_MS_PeSpecialLw&&fp->cmd_vars[1]==2&&frame>=counter_retry&&victim->motion_id==ftCo_MS_Wait){
   melee_startup_publish_combat(1,0,0,PAD_BUTTON_A);counter_release=frame+6;counter_retry=frame+120;
   OSReport("Kirby copy opponent jab input frame=%u\n",frame);
  }
 }
 if((variant==20||variant==21)&&copied){
  int loop=variant==20?ftKb_MS_LkSpecialNLoop:ftKb_MS_ClSpecialNLoop;
  int end=variant==20?ftKb_MS_LkSpecialNEnd:ftKb_MS_ClSpecialNEnd;
  if(fp->motion_id==loop&&!(items&4)){items|=4;OSReport("Kirby copied bow charging frame=%u\n",frame);}
  if(fp->motion_id==end&&!(items&8)){items|=8;OSReport("Kirby copied bow released frame=%u\n",frame);}
 }
 if(variant==23&&copied){
  if(victim->motion_id==ftCo_MS_KirbyYoshiEgg&&!(items&1)){
   if(!victim->x20A0_accessory||!victim->invisible){OSReport("Copied Egg Lay missing egg or hidden victim\n");_Exit(6);}
   items|=1;OSReport("Kirby copied Egg Lay captured opponent frame=%u\n",frame);
  }
  if((items&1)&&victim->motion_id==ftCo_MS_Wait&&!victim->invisible&&!(items&2)){items|=2;OSReport("Kirby copied Egg Lay opponent escaped frame=%u\n",frame);}
 }
 if(variant==22&&copied){
  if(fp->motion_id==ftKb_MS_PrSpecialNLoop&&!(items&1)){items|=1;OSReport("Kirby copied Rollout charging frame=%u\n",frame);}
  if(fp->motion_id==ftKb_MS_PrSpecialN1&&!(items&2)){items|=2;OSReport("Kirby copied Rollout released frame=%u\n",frame);}
  if(fp->motion_id==ftKb_MS_PrSpecialNHit&&!(items&4)){/* Hit callback only accepts released rolling states. */items|=6;OSReport("Kirby copied Rollout released and hit frame=%u\n",frame);}
 }
 if(variant==19&&copied){
  if(fp->motion_id==ftKb_MS_MtSpecialNLoop&&!(items&2)){items|=2;OSReport("Kirby copied Shadow Ball charging frame=%u\n",frame);}
  if(fp->motion_id==ftKb_MS_MtSpecialNEnd&&!(items&4)){items|=4;OSReport("Kirby copied Shadow Ball released frame=%u\n",frame);}
 }
 if(variant==18&&copied){
  if(fp->motion_id==ftKb_MS_KpSpecialN){
   if(!(items&2)){items|=2;OSReport("Kirby copied Fire Breath active frame=%u\n",frame);}
   if(fp->u.kb.x84<((ftKb_DatAttrs*)fp->dat_attrs)->specialn_kp_max_fuel){items|=4;breath_low=fp->u.kb.x84;}
   if(victim->dmg.x1830_percent>punch_damage_before&&!(items&16)){items|=16;OSReport("Kirby copied Fire Breath hit frame=%u\n",frame);}
  }
  if((items&4)&&fp->motion_id==ftCo_MS_Wait&&fp->u.kb.x84>breath_low&&!(items&8)){
   items|=8;OSReport("Kirby copied Fire Breath fuel recovering frame=%u low=%.2f now=%.2f\n",frame,breath_low,fp->u.kb.x84);
  }
 }
 if(variant==15&&copied){
  if(fp->motion_id==ftKb_MS_SsSpecialNHold&&!(items&2)){items|=2;OSReport("Kirby copied Charge Shot charging frame=%u\n",frame);}
  if(fp->motion_id==ftKb_MS_SsSpecialN&&!(items&4)){
   if(fp->u.kb.xA8>=((ftKb_DatAttrs*)fp->dat_attrs)->specialn_ss_charge_time){OSReport("Expected partial Charge Shot\n");_Exit(6);}
   items|=4;OSReport("Kirby copied partial Charge Shot fired frame=%u charge=%d\n",frame,fp->u.kb.xA8);
  }
  if((items&4)&&fp->motion_id==ftKb_MS_SsSpecialNCancel&&!(items&8)){
   if(fp->u.kb.xA8<=0){OSReport("Cancelled Charge Shot lost charge\n");_Exit(6);}
   items|=8;OSReport("Kirby copied Charge Shot cancelled frame=%u charge=%d\n",frame,fp->u.kb.xA8);
  }
  if((items&8)&&fp->motion_id==ftCo_MS_Wait&&fp->u.kb.xA8>=((ftKb_DatAttrs*)fp->dat_attrs)->specialn_ss_charge_time&&!(items&16)){
   items|=16;OSReport("Kirby copied full Charge Shot stored frame=%u\n",frame);
  }
  if((items&16)&&fp->motion_id==ftKb_MS_SsSpecialN&&!(items&32)){items|=32;OSReport("Kirby copied full Charge Shot fired frame=%u\n",frame);}
 }
 if(variant==14&&copied){
  if(fp->motion_id==ftKb_MS_PpSpecialN&&!(items&2)){items|=2;OSReport("Kirby copied Ice Shot activated frame=%u\n",frame);}
  if(fp->x20A0_accessory&&!(items&4)){items|=4;OSReport("Kirby copied hammer attached frame=%u\n",frame);}
 }
 for(HSD_GObj*it=plinklow_gobjs?plinklow_gobjs[9]:NULL;it;it=it->prev){
  if(it->classifier!=HSD_GOBJ_CLASS_ITEM||!it->user_data)continue;Item*ip=it->user_data;
  for(unsigned i=0;i<count;i++)if(ip->kind==kinds[i]&&!(items&(1u<<i))){items|=1u<<i;OSReport("Kirby copied article kind=%d mask=%u frame=%u\n",ip->kind,items,frame);}
 }
 if(captured&&copied&&items==required&&fp->u.kb.hat.kind==target&&fp->motion_id==ftCo_MS_Wait&&!returned){returned=frame;OSReport("Kirby copy returned to idle frame=%u\n",frame);}
 if(variant==5&&returned&&!discard_requested&&frame>=returned+20&&fp->motion_id==ftCo_MS_Wait){
  melee_startup_publish_combat(0,0,0,PAD_BUTTON_UP);release=frame+6;discard_requested=frame;
  OSReport("Kirby copy discard input frame=%u\n",frame);
 }
 if(variant==5&&discard_requested&&!discarded&&fp->u.kb.hat.kind==Ft_Kind_Kirby){
  if(fp->u.kb.hat.x14.data){OSReport("Kirby discarded copy but retained costume objects\n");_Exit(6);}
  discarded=frame;captured=copied=items=returned=0;next_input=frame+120;
  OSReport("Kirby copy costume removed frame=%u\n",frame);
 }
 if(frame>=1800&&frame<6800&&frame>=next_input&&!returned){
  if(fp->motion_id==ftKb_MS_EatWait||fp->motion_id==ftKb_MS_EatFall){melee_startup_publish_combat(0,0,0,PAD_BUTTON_B);release=frame+6;next_input=frame+90;}
  else if(copied){
   if(variant==23&&(items&1))return returned!=0;
   if(fp->motion_id==ftCo_MS_Wait){
    float dx=victim->cur_pos.x-fp->cur_pos.x;
    if((variant==16||variant==17)&&dx<25&&dx> -25){
     melee_startup_publish_combat(0,dx>0?-60:60,0,0);release=frame+6;next_input=frame+30;
    }else if(variant>=7&&(((variant<16||variant>=18)&&(dx>14||dx< -14))||dx*fp->facing_dir< -1)){
     melee_startup_publish_combat(0,dx>0?60:-60,0,0);release=frame+((variant<16||variant>=18)&&(dx>14||dx< -14)?6:2);next_input=frame+30;
    }else{
     punch_damage_before=victim->dmg.x1830_percent;
     melee_startup_publish_combat(0,0,0,PAD_BUTTON_B);release=frame+(variant==18?180:variant>=9&&variant!=15?90:6);next_input=frame+(variant==6||variant==15?60:240);
    }
   }
   else if(variant==19&&(fp->motion_id==ftKb_MS_MtSpecialNLoop||fp->motion_id==ftKb_MS_MtSpecialNLoopFull)){
    melee_startup_publish_combat(0,0,0,PAD_BUTTON_B);release=frame+6;next_input=frame+240;
   }
   else if(variant==15&&fp->motion_id==ftKb_MS_SsSpecialNHold){
    unsigned button=!(items&4)?PAD_BUTTON_B:!(items&8)?PAD_TRIGGER_L:0;
    if(button){melee_startup_publish_combat(0,0,0,button);release=frame+6;next_input=frame+60;}
   }
   else if(variant==6&&fp->motion_id==ftKb_MS_DkSpecialNLoop){
    unsigned button=!(items&2)?PAD_BUTTON_B:!(items&4)?PAD_TRIGGER_L:0;
    if(button){melee_startup_publish_combat(0,0,0,button);release=frame+(variant==18?180:variant>=9&&variant!=15?90:6);next_input=frame+(variant==6||variant==15?60:240);}
   }
  }
  else{
   float dx=victim->cur_pos.x-fp->cur_pos.x,dy=victim->cur_pos.y-fp->cur_pos.y;
   if(dx>14||dx< -14||dy>15){melee_startup_publish_combat(0,dx>3?60:dx< -3?-60:0,0,dy>15?PAD_BUTTON_X:0);release=frame+12;}
   else if(dx*fp->facing_dir< -1&&fp->motion_id==ftCo_MS_Wait){
    melee_startup_publish_combat(0,dx>0?60:-60,0,0);release=frame+2;
    OSReport("Kirby copy turn toward victim frame=%u dx=%.2f facing=%.1f\n",frame,dx,fp->facing_dir);
   }
   else if(fp->motion_id==ftCo_MS_Wait){melee_startup_publish_combat(0,0,0,PAD_BUTTON_B);release=frame+45;}
   next_input=frame+90;
  }
 }
 return returned!=0&&(variant!=5||discarded!=0);
}

void melee_startup_kirby_copy_opponent(unsigned frame,unsigned variant)
{
 static unsigned settled,release;
 if(!variant||variant>24)return;
 const unsigned kinds[]={CKind_Ness,CKind_Mario,CKind_DrMario,CKind_Luigi,CKind_Fox,CKind_Falco,CKind_Donkey,CKind_Captain,CKind_Ganon,CKind_Mars,CKind_Emblem,CKind_Zelda,CKind_Zelda,CKind_Peach,CKind_PopoNana,CKind_Samus,CKind_Pikachu,CKind_Pichu,CKind_Koopa,CKind_Mewtwo,CKind_Link,CKind_CLink,CKind_Purin,CKind_Yoshi,CKind_GameWatch};
 if(frame==800&&(variant==24||variant==22||variant==21||variant==19||variant==17||variant==2||variant==3||variant==5||(variant>=8&&variant<=10))){unsigned bit=gm_CKindToUnlockIndex(kinds[variant]);if(bit>=16)_Exit(6);*gmMainLib_GetUnlockedCharactersBitmaskPtr()|=(u16)(1u<<bit);}
 if(frame==1100)melee_startup_publish_opponent(0,0,0);
 if(frame==release)melee_startup_publish_opponent(0,0,0);
 if(release||frame<1200||frame>=1380||gm_GetCurrentGameMode()!=GM_VS||gm_GetCurrentSceneIndex()!=0)return;
 float dx,dy;if(!mnCharSel_NativeTargetDelta(1,kinds[variant],&dx,&dy))return;
 int x=dx>0.7f?45:dx< -0.7f?-45:0,y=dy>0.7f?45:dy< -0.7f?-45:0;
 if(x||y){settled=0;melee_startup_publish_opponent(x,y,0);}
 else if(++settled>=6){melee_startup_publish_opponent(0,0,1);release=frame+6;OSReport("Kirby copy opponent selected kind=%u frame=%u\n",kinds[variant],frame);}
 else melee_startup_publish_opponent(0,0,0);
}

#include <melee/ft/kinds/ftKirby/ftkirby.h>
int melee_startup_kirby_cache_reset_test(void)
{
 static Kirby_Unk mario;
 static KirbyHatStruct hats[Ft_Kind_Max];
 ft_80459B88.x0=&mario;
 for(unsigned i=0;i<Ft_Kind_Max-1;i++)ft_80459B88.hats[i]=&hats[i];
 ftKb_Init_800EE528();
 if(ft_80459B88.x0){fprintf(stderr,"Kirby reset retained Mario pointer\n");return 6;}
 for(unsigned i=0;i<Ft_Kind_Max-1;i++)if(ft_80459B88.hats[i]){fprintf(stderr,"Kirby reset retained pointer slot %u\n",i+1);return 6;}
 puts("Kirby copy reset: every used native pointer slot cleared");return 0;
}

#include "melee_texture.h"
extern HSD_ImageDesc* grPura_NativeToonImage(void);
extern u16 grPu_803E6E20[1024];
int melee_startup_pura_toon_test(void)
{
    HSD_ImageDesc* d=grPura_NativeToonImage();u8 rgba[4096];
    if(d!=grPura_NativeToonImage()||d->width!=32||d->height!=32||d->format!=4||((uintptr_t)d->image_ptr&31))return 1;
    if(!melee_texture_decode(d->image_ptr,2048,32,32,4,NULL,0,0,rgba,sizeof(rgba)))return 2;
    for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
        unsigned tile=(y/4)*8+x/4,index=tile*16+(y%4)*4+x%4;
        u16 v=grPu_803E6E20[index];u8* out=rgba+4*(y*32+x);
        unsigned r=(v>>11)&31,g=(v>>5)&63,b=v&31;
        if(out[0]!=((r<<3)|(r>>2))||out[1]!=((g<<2)|(g>>4))||out[2]!=((b<<3)|(b>>2))||out[3]!=255)return 3;
    }
    puts("Poke Floats toon texture: all 1024 RGB565 pixels, alignment and repeated initialization passed");return 0;
}

int melee_startup_classic_entry_progress(void)
{
    if(getenv("MELEE_TEST_HOMERUN"))return gm_GetCurrentGameMode()==GM_HOME_RUN_CONTEST&&gm_GetCurrentSceneIndex()==0;
    if(getenv("MELEE_TEST_TRAINING"))return gm_GetCurrentGameMode()==GM_TRAINING&&gm_GetCurrentSceneIndex()==0;
    if(getenv("MELEE_TEST_ALLSTAR")){
        fprintf(stderr,"All-Star entry: mode=%d scene=%d\n",gm_GetCurrentGameMode(),gm_GetCurrentSceneIndex());
        return gm_GetCurrentGameMode()==GM_ALLSTAR&&gm_GetCurrentSceneIndex()==112;
    }
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    fprintf(stderr,"Classic entry: mode=%d scene=%d (expected Classic CSS 3/112)\n",mode,scene);
    if(mode!=GM_CLASSIC || scene!=112)return 0;
    extern u8 gm_804908A0[112];
    const unsigned offsets[]={0x0C,0x34,0x54,0x60};
    const unsigned counts[]={38,29,10,9};
    for(unsigned group=0;group<4;group++){
        u8 seen[39]={0};
        for(unsigned i=0;i<counts[group];i++){
            unsigned v=gm_804908A0[offsets[group]+i];
            if(v>=counts[group]||seen[v]++){
                fprintf(stderr,"Classic invalid matchup permutation group=%u index=%u value=%u\n",group,i,v);return 0;
            }
        }
    }
    puts("Classic entry: all four matchup permutations valid");
    return 1;
}

int melee_startup_classic_match(unsigned frame)
{
    static unsigned settled,release;
    if(frame==release)melee_startup_publish_confirm(0);
    if(frame>1800&&frame<2150&&!release&&(((gm_GetCurrentGameMode()==GM_CLASSIC||gm_GetCurrentGameMode()==GM_ADVENTURE||gm_GetCurrentGameMode()==GM_ALLSTAR)&&gm_GetCurrentSceneIndex()==112)||((getenv("MELEE_TEST_TRAINING")||getenv("MELEE_TEST_HOMERUN"))&&(gm_GetCurrentGameMode()==GM_TRAINING||gm_GetCurrentGameMode()==GM_HOME_RUN_CONTEST)&&gm_GetCurrentSceneIndex()==0))){
        float dx,dy;
        CharacterKind choice=getenv("MELEE_TEST_TARGET_COURSE")?atoi(getenv("MELEE_TEST_TARGET_COURSE")):getenv("MELEE_TEST_CLASSIC_CHARACTER")?atoi(getenv("MELEE_TEST_CLASSIC_CHARACTER")):CKind_Fox;
        HSD_ASSERT(__LINE__,choice>=0&&choice<CKind_Playable_Count);
        if(mnCharSel_NativeTargetDelta(0,choice,&dx,&dy)){
            int x=dx>0.7f?45:dx< -0.7f?-45:0,y=dy>0.7f?45:dy< -0.7f?-45:0;
            if(x||y){settled=0;melee_startup_publish_stick(x,y);}
            else if(++settled>=6){melee_startup_publish_confirm(1);release=frame+6;}
            else melee_startup_publish_stick(0,0);
        }
    }
    if(frame==2200||frame==2206)melee_startup_publish_start(frame==2200);
    if(frame==2600||frame==2606)melee_startup_publish_confirm(frame==2600);
    unsigned fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    if(frame==3800)fprintf(stderr,"Classic match: mode=%d scene=%d fighters=%u\n",gm_GetCurrentGameMode(),gm_GetCurrentSceneIndex(),fighters);
    return gm_GetCurrentGameMode()==GM_CLASSIC&&gm_GetCurrentSceneIndex()==1&&fighters>=2;
}

extern const void* gmIntro_NativeDecodeLayout(const void*,size_t);
int melee_startup_classic_layout_test(const char* path)
{
    FILE* f=fopen(path,"rb");if(!f)return 1;
    fseek(f,0,SEEK_END);long length=ftell(f);rewind(f);
    if(length<32||length>1048576){fclose(f);return 2;}
    u8* bytes=malloc(length);if(!bytes){fclose(f);return 3;}
    if(fread(bytes,1,length,f)!=(size_t)length){fclose(f);free(bytes);return 4;}
    fclose(f);
    const u8* decoded=gmIntro_NativeDecodeLayout(bytes,length);
    if(!decoded){free(bytes);return 5;}
    u8 expected[0x9B8];
    for(unsigned i=0;i<sizeof(expected);i+=4){
        u32 v=((u32)bytes[32+i]<<24)|((u32)bytes[33+i]<<16)|((u32)bytes[34+i]<<8)|bytes[35+i];
        memcpy(expected+i,&v,4);
    }
    if(memcmp(expected,decoded,sizeof(expected))){free(bytes);return 6;}
    bytes[32]=0x7f;bytes[33]=0x80;bytes[34]=bytes[35]=0;
    if(gmIntro_NativeDecodeLayout(bytes,length)||gmIntro_NativeDecodeLayout(bytes,31)){free(bytes);return 7;}
    memset(bytes,0,length);free(bytes);
    if(memcmp(expected,decoded,sizeof(expected)))return 8;
    puts("Classic intro layout: all 622 scalar words, source-free ownership, invalid input preserves valid data passed");
    return 0;
}

int melee_startup_classic_round(unsigned frame)
{
    static int last=-1;static unsigned defeated;
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    int key=mode*256+scene;
    if(key!=last){fprintf(stderr,"Classic round transition: frame=%u mode=%d scene=%d\n",frame,mode,scene);last=key;}
    if(frame==3900){
        if(mode!=GM_CLASSIC||scene!=1)_Exit(6);
        for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev){
            Fighter* fp=g->user_data;
            if(fp&&fp->player_id!=0){fp->cur_pos.y=-10000;defeated++;}
        }
        fprintf(stderr,"Classic fixture: placed %u opponents below blast zone\n",defeated);
        if(!defeated)_Exit(6);
    }
    if(frame>4100&&mode==GM_CLASSIC&&scene!=9&&(scene!=1||gmVs_GetController_0()->match_over)){
        if(frame%90==0){if(scene==1)melee_startup_publish_start(1);else melee_startup_publish_confirm(1);}
        if(frame%90==6){if(scene==1)melee_startup_publish_start(0);else melee_startup_publish_confirm(0);}
    }
    unsigned fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    if(frame==6500)fprintf(stderr,"Classic next round: mode=%d scene=%d fighters=%u\n",mode,scene,fighters);
    return defeated&&mode==GM_CLASSIC&&scene==9&&fighters>=2;
}

void melee_startup_classic_navigate(unsigned frame)
{
    if(frame==1020&&(getenv("MELEE_TEST_TRAINING")||getenv("MELEE_TEST_HOMERUN"))){
        HSD_ASSERT(__LINE__,gm_GetCurrentGameMode()==GM_MENU);
        ((struct MenuExitData*)gm_GetCurrentSceneExitData())->pending_mode=getenv("MELEE_TEST_HOMERUN")?GM_HOME_RUN_CONTEST:GM_TRAINING;gm_801A4B60();
    }
    if(frame==1020&&getenv("MELEE_TEST_TARGET_COURSE")){
        int choice=atoi(getenv("MELEE_TEST_TARGET_COURSE"));HSD_ASSERT(__LINE__,choice>=0&&choice<CKind_Playable_Count);
        if(!gm_IsCKindUnlocked(choice)){
            unsigned bit=gm_CKindToUnlockIndex(choice);HSD_ASSERT(__LINE__,bit<16);
            *gmMainLib_GetUnlockedCharactersBitmaskPtr()|=(u16)(1u<<bit);
        }
    }
    if(getenv("MELEE_TEST_ALLSTAR")&&frame==1020){
        HSD_ASSERT(__LINE__,gm_GetCurrentGameMode()==GM_MENU);
        *seed_ptr=getenv("MELEE_TEST_ALLSTAR_SEED")?(u32)strtoul(getenv("MELEE_TEST_ALLSTAR_SEED"),NULL,0):1;
        ((struct MenuExitData*)gm_GetCurrentSceneExitData())->pending_mode=GM_ALLSTAR;
        gm_801A4B60();
    }

    if(frame<1100||frame>1700)return;
    if(frame%120==60&&gm_GetCurrentGameMode()==GM_MENU)melee_startup_publish_confirm(1);
    if(frame%120==66)melee_startup_publish_confirm(0);
}

int melee_startup_classic_bonus(unsigned frame)
{
    static unsigned defeated;
    static int finished;
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    if(frame==6600){
        if(mode!=GM_CLASSIC||scene!=9)_Exit(6);
        if(!gmVs_GetController_0()->match_over){
            int team=Player_GetTeam(0);
            for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev){
                Fighter* fp=g->user_data;
                if(fp&&Player_GetTeam(fp->player_id)!=team){fp->cur_pos.y=-10000;defeated++;}
            }
            fprintf(stderr,"Classic team fixture: placed %u enemy fighters below blast zone\n",defeated);
            if(!defeated)_Exit(6);
        }else fprintf(stderr,"Classic team fixture: match already finished naturally\n");
        finished=1;
    }
    if(frame>6700&&mode==GM_CLASSIC&&scene==9&&gmVs_GetController_0()->match_over){
        if(frame%90==0)melee_startup_publish_start(1);
        if(frame%90==6)melee_startup_publish_start(0);
    }
    unsigned fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    if(frame==9000)fprintf(stderr,"Classic bonus: mode=%d scene=%d fighters=%u ground=%d\n",mode,scene,fighters,stage_info.grkind);
    return finished&&mode==GM_CLASSIC&&scene==17&&fighters>=1;
}

#include <melee/it/kinds/itmato.h>
int melee_startup_classic_after_bonus(unsigned frame)
{
    static unsigned broken;
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    if(frame==9100){
        if(mode!=GM_CLASSIC||scene!=17||stage_info.x6D4!=10)_Exit(6);
        for(HSD_GObj* g=plinklow_gobjs[9],*next;g;g=next){
            next=g->prev;Item* item=g->user_data;
            if(item&&item->kind==It_Kind_Mato){
                /* Exercise the normal target damage callback and removal;
                 * this fixture tests completion/transition, not hit detection. */
                if(!it_802D85F4(g))_Exit(6);
                Item_8026A8EC(g);broken++;
            }
        }
        fprintf(stderr,"Classic target fixture: broke %u targets, remaining=%d\n",broken,stage_info.x6D4);
        if(broken!=10||stage_info.x6D4)_Exit(6);
    }
    if(frame>9200&&mode==GM_CLASSIC&&scene==17&&gmVs_GetController_0()->match_over){
        if(frame%90==0)melee_startup_publish_start(1);
        if(frame%90==6)melee_startup_publish_start(0);
    }
    unsigned fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    if(frame==12000){for(int slot=0;slot<2;slot++)fprintf(stderr,"Classic HUD slot=%d stamina=%d damage=%d\n",slot,Player_GetMoreFlagsBit2(slot),Player_GetDamage(slot));}
    if(frame==12000)fprintf(stderr,"Classic after bonus: mode=%d scene=%d fighters=%u ground=%d\n",mode,scene,fighters,stage_info.grkind);
    return broken==10&&mode==GM_CLASSIC&&scene==25&&fighters>=2;
}

#include <melee/if/ifstatus.h>
#include <melee/if/types.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
int melee_startup_hud_restart(unsigned frame, int stamina)
{
    if(frame==2200||frame==2400){
        for(int slot=0;slot<2;slot++){
            if(!Player_GetEntity(slot)||Player_GetMoreFlagsBit2(slot))return 0;
            Player_SetHUDDamage(slot,frame==2200?123:42);
        }
    }
    if(stamina&&(frame==2600||frame==2900)){
        if(!Player_GetEntity(1))return 0;
        Player_SetMoreFlagsBit2(1,frame==2600);
        Player_SetOtherStamina(1,100);
        Player_SetHUDDamage(1,frame==2600?25:42);
        ifStatus_802F5EC0(&ifStatus_GetHUDInfo()->players[1],1);
    }
    if(stamina&&frame==2700)Player_SetHUDDamage(1,10);
    if(stamina&&(frame==2750||frame==3000||frame==3200)){
        HudIndex* hud=ifStatus_GetHUDInfo();IfDamageState* state=&hud->players[1];
        HSD_MatAnimJoint* root=((HSD_MatAnimJoint**)hud->janim_selection_joints)[0];
        HSD_TexAnim* mark=root->child->next->next->next->matanim->texanim;
        HSD_TObj* texture=state->jobjs[3]->u.dobj->mobj->tobj;
        int hp=Player_GetMoreFlagsBit2(1);
        int value=hp?Player_GetRemainingHP(1):Player_GetDamage(1);
        if(mark->n_imagetbl<2||texture->imagedesc!=mark->imagetbl[hp]||state->damage_percent!=value){
            fprintf(stderr,"HUD stamina mismatch frame=%u hp=%d displayed=%d actual=%d marker=%d\n",frame,hp,state->damage_percent,value,texture->imagedesc==mark->imagetbl[hp]);return 0;
        }
        fprintf(stderr,"HUD stamina frame=%u hp=%d value=%d marker verified\n",frame,hp,value);
    }
    if(frame==2500||frame==2800){
        HudIndex* hud=ifStatus_GetHUDInfo();
        HSD_MatAnimJoint* root=((HSD_MatAnimJoint**)hud->janim_selection_joints)[0];
        HSD_TexAnim* digits=root->child->matanim->texanim;
        HSD_TexAnim* mark=root->child->next->next->next->matanim->texanim;
        for(int slot=0;slot<2;slot++)for(int j=0;j<4;j++){
            HSD_JObj* joint=hud->players[slot].jobjs[j];
            if(!joint||!joint->u.dobj||!joint->u.dobj->mobj)return 0;
            HSD_TObj* texture=joint->u.dobj->mobj->tobj;
            if(!texture||texture->imagetbl!=(j==3?mark:digits)->imagetbl){
                fprintf(stderr,"HUD wrong texture table: slot=%d digit=%d\n",slot,j);return 0;
            }
        }
        fprintf(stderr,"HUD restart: frame=%u native texture tables verified for both players\n",frame);
    }
    return 1;
}

int melee_startup_classic_fifth(unsigned frame)
{
    static int finished;
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    if(frame==12100){
        if(mode!=GM_CLASSIC||scene!=25)_Exit(6);
        unsigned defeated=0;
        if(!gmVs_GetController_0()->match_over){
        for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev){
            Fighter* fp=g->user_data;
            if(fp&&fp->player_id!=0){fp->cur_pos.y=-10000;defeated++;}
        }
        fprintf(stderr,"Classic fourth fixture: placed %u opponents below blast zone\n",defeated);
        if(!defeated)_Exit(6);
        }else fprintf(stderr,"Classic fourth fixture: match already finished\n");
        finished=1;
    }
    if(frame>12200&&mode==GM_CLASSIC&&scene==25&&gmVs_GetController_0()->match_over){
        if(frame%90==0)melee_startup_publish_start(1);
        if(frame%90==6)melee_startup_publish_start(0);
    }
    unsigned fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    if(frame==15000)fprintf(stderr,"Classic fifth: mode=%d scene=%d fighters=%u ground=%d\n",mode,scene,fighters,stage_info.grkind);
    return finished&&mode==GM_CLASSIC&&scene==33&&fighters>=2;
}

int melee_startup_classic_trophy(unsigned frame)
{
    static int finished, entered_bonus;
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    if(mode==GM_CLASSIC&&scene==41)entered_bonus=1;
    if(frame==15100){
        if(mode!=GM_CLASSIC||scene!=33)_Exit(6);
        if(!gmVs_GetController_0()->match_over){
            int team=Player_GetTeam(0);unsigned defeated=0;
            for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev){
                Fighter* fp=g->user_data;
                if(fp&&Player_GetTeam(fp->player_id)!=team){fp->cur_pos.y=-10000;defeated++;}
            }
            if(!defeated)_Exit(6);
            fprintf(stderr,"Classic giant fixture: placed %u opponents below blast zone\n",defeated);
        }
        finished=1;
    }
    if(frame>15200&&mode==GM_CLASSIC&&scene==33&&gmVs_GetController_0()->match_over){
        if(frame%90==0)melee_startup_publish_start(1);
        if(frame%90==6)melee_startup_publish_start(0);
    }
    unsigned fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    if(frame==18000)fprintf(stderr,"Classic trophy: mode=%d scene=%d fighters=%u ground=%d\n",mode,scene,fighters,stage_info.grkind);
    return finished&&entered_bonus&&mode==GM_CLASSIC&&
        ((scene==41&&fighters>=1)||(scene==49&&fighters>=2));
}

int melee_startup_classic_team(unsigned frame)
{
    static int finished;
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    if(frame==18100){
        if(mode!=GM_CLASSIC||scene!=49)_Exit(6);
        if(!gmVs_GetController_0()->match_over){
            unsigned defeated=0;
            for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev){
                Fighter* fp=g->user_data;
                if(fp&&fp->player_id!=0){fp->cur_pos.y=-10000;defeated++;}
            }
            if(!defeated)_Exit(6);
            fprintf(stderr,"Classic seventh fixture: placed %u opponents below blast zone\n",defeated);
        }
        finished=1;
    }
    if(frame>18200&&mode==GM_CLASSIC&&scene==49&&gmVs_GetController_0()->match_over){
        if(frame%90==0)melee_startup_publish_start(1);
        if(frame%90==6)melee_startup_publish_start(0);
    }
    unsigned fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    if(frame==21000)fprintf(stderr,"Classic team: mode=%d scene=%d fighters=%u ground=%d\n",mode,scene,fighters,stage_info.grkind);
    return finished&&mode==GM_CLASSIC&&scene==57&&fighters>=2;
}

#undef NDEBUG
#include <assert.h>
#include <melee/gm/types.h>
#include <melee/gm/gm_16A2.h>
static int spawn_probe_value;
static void spawn_probe_callback(s32 slot,u8 count){spawn_probe_value=slot+count;}
static int spawn_probe_route(bool value){return value;}
int melee_startup_spawn_state_test(void)
{
        struct lbl_8046B488_t* state=gm_1601_GetUnkData();
        memset(state->x1C0,0x5a,sizeof(state->x1C0));
        fn_80169434(spawn_probe_route);
        gm_8016A404((intptr_t)spawn_probe_callback);
        assert(state->native_event_player_init_cb==spawn_probe_callback);
        assert(state->x1B8==spawn_probe_route);
        for(unsigned i=0;i<sizeof(state->x1C0);i++)assert(state->x1C0[i]==0x5a);
        state->native_event_player_init_cb(3,7);assert(spawn_probe_value==10);
        gm_8016A22C(33,33,33,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,1);
        assert(!state->native_event_player_init_cb&&!state->x1B8);
        for(unsigned i=0;i<sizeof(state->x1C0);i++)assert(state->x1C0[i]==0x5a);
        gm_8016A22C(CKind_Yoshi,CKind_Yoshi,CKind_Yoshi,0,0,0,0,0,0,CKind_Fox,0,1,0,0,0,0,0,1,1);
        assert(state->x7==0&&state->x20[0]==-2);
        puts("Opponent state: full-width callback, roster preservation and empty Adventure opponent list passed");return 0;
    }

int melee_startup_classic_race(unsigned frame)
{
    static unsigned defeated;
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    /* The ten-opponent round replenishes its roster. Finish each wave through
     * ordinary blast-zone processing to exercise spawning and teardown. */
    if(frame>=21100&&frame<23000&&frame%60==40&&mode==GM_CLASSIC&&scene==57&&
       !gmVs_GetController_0()->match_over){
        for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev){
            Fighter* fp=g->user_data;
            if(fp&&fp->player_id!=0){fp->cur_pos.y=-10000;defeated++;}
        }
    }
    if(frame>21200&&mode==GM_CLASSIC&&scene==57&&gmVs_GetController_0()->match_over){
        if(frame%90==0)melee_startup_publish_start(1);
        if(frame%90==6)melee_startup_publish_start(0);
    }
    unsigned fighters=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
    if(frame==24000)fprintf(stderr,"Classic race: mode=%d scene=%d fighters=%u ground=%d knockout placements=%u\n",mode,scene,fighters,stage_info.grkind,defeated);
    return mode==GM_CLASSIC&&scene==65&&fighters>=1;
}

int melee_startup_classic_gameover(unsigned frame)
{
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    if(frame==2400){
        /* Keep the setup fight alive until the scheduled final-stock loss. */
        for(int slot=0;slot<6;slot++)if(Player_GetEntity(slot))Player_SetStocks(slot,10);
    }
    if(frame==2800){
        if(mode!=GM_CLASSIC||scene!=1||gmVs_GetController_0()->match_over)_Exit(6);
        HSD_GObj* player=Player_GetEntity(0);if(!player)_Exit(6);
        Player_SetStocks(0,1);((Fighter*)player->user_data)->cur_pos.y=-10000;
    }
    if(frame>3000&&mode==GM_CLASSIC&&scene==1&&gmVs_GetController_0()->match_over){
        if(frame%90==0)melee_startup_publish_start(1);
        if(frame%90==6)melee_startup_publish_start(0);
    }
    if(frame==4500)fprintf(stderr,"Classic game over: mode=%d scene=%d\n",mode,scene);
    return mode==GM_CLASSIC&&scene==105;
}

int melee_startup_classic_continue(unsigned frame)
{
    static int resumed,second_loss;
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    if(frame==4600){if(mode!=GM_CLASSIC||scene!=105)_Exit(6);melee_startup_publish_confirm(1);}
    if(frame==4606)melee_startup_publish_confirm(0);
    if(frame>4700&&frame<6000&&mode==GM_CLASSIC&&scene==0){
        if(frame%90==0)melee_startup_publish_confirm(1);
        if(frame%90==6)melee_startup_publish_confirm(0);
    }
    if(frame>4700&&!resumed&&mode==GM_CLASSIC&&scene==1){
        HSD_GObj* player=Player_GetEntity(0);
        if(player){resumed=1;for(int slot=0;slot<6;slot++)if(Player_GetEntity(slot))Player_SetStocks(slot,10);}
    }
    if(frame==6000){fprintf(stderr,"Classic continue: resumed=%d mode=%d scene=%d\n",resumed,mode,scene);if(!resumed||mode!=GM_CLASSIC||scene!=1)_Exit(6);}
    if(frame==6200){
        HSD_GObj* player=Player_GetEntity(0);if(!player||mode!=GM_CLASSIC||scene!=1)_Exit(6);
        Player_SetStocks(0,1);((Fighter*)player->user_data)->cur_pos.y=-10000;second_loss=1;
    }
    if(frame==8000){fprintf(stderr,"Classic repeat loss: mode=%d scene=%d\n",mode,scene);if(!second_loss||mode!=GM_CLASSIC||scene!=105)_Exit(6);}
    if(frame==8100)melee_startup_publish_stick(80,0);
    if(frame==8106)melee_startup_publish_stick(0,0);
    if(frame==8200)melee_startup_publish_confirm(1);
    if(frame==8206)melee_startup_publish_confirm(0);
    if(frame==9000)fprintf(stderr,"Classic decline continue: mode=%d scene=%d\n",mode,scene);
    return resumed&&second_loss&&mode==GM_MENU;
}

#include <melee/ft/kinds/ftMasterHand/ftmasterhandfingerbeam.h>
#include <melee/it/kinds/ityaku.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/ftcommon.h>
int melee_startup_classic_campaign(unsigned frame);
static int melee_startup_allstar_opening(unsigned frame)
{
    static int last=-1;static unsigned entered,rounds,rests;
    int campaign=getenv("MELEE_TEST_ALLSTAR_CAMPAIGN")!=NULL;
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex(),key=mode*256+scene;
    if(last!=key){last=key;entered=frame;fprintf(stderr,"All-Star: frame=%u mode=%d scene=%d rounds=%x rests=%x\n",frame,mode,scene,rounds,rests);}
    unsigned age=frame-entered;
    if(mode==GM_ALLSTAR&&scene<=96&&scene%8==0){
        if(Player_GetEntity(0)&&age==120){Player_SetStocks(0,20);rounds|=1u<<(scene/8);}
        if(age>=600&&frame%120==0){
            for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev)
                if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data&&((Fighter*)g->user_data)->player_id!=0)((Fighter*)g->user_data)->cur_pos.y=-10000;
        }
        if(gmVs_GetController_0()->match_over){if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);}
    }
    if(mode==GM_ALLSTAR&&scene<=89&&scene%8==1&&Player_GetEntity(0)){
        if(age==120)rests|=1u<<(scene/8);
        if(campaign&&age==600){
            Fighter* fp=Player_GetEntity(0)->user_data;Vec3 goal;assert(Ground_801C2D24(0x99,&goal));
            goal.y+=5;fp->cur_pos=goal;fp->self_vel=(Vec3){0,0,0};mpColl_80043680(&fp->coll_data,&goal);
            fprintf(stderr,"All-Star portal placement: scene=%d at %.1f %.1f\n",scene,goal.x,goal.y);
        }
        if(campaign&&age==1200){fprintf(stderr,"All-Star portal fixture stalled: detector=%d flags=%x\n",stage_info.x714,stage_info.flags);_Exit(6);}
        if(campaign&&gmVs_GetController_0()->match_over){if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);}
    }
    if(campaign)return rounds==0x1fff&&rests==0xfff&&mode==GM_ALLSTAR_GOVER&&scene==1&&age>=600;
    return rounds==1&&mode==GM_ALLSTAR&&scene==1&&Player_GetEntity(0)&&age>=600;
}

static int melee_startup_training_focus(unsigned frame)
{
    static unsigned menu_seen,speed_changed,centered,confirmed;static int previous=-1;
    extern unsigned mnStageSel_NativeOnettGuidance(void);
    extern unsigned mnStageSel_NativeFrames(void);
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    int key=mode*256+scene;
    if(key!=previous){previous=key;fprintf(stderr,"Training navigation: frame=%u mode=%d scene=%d\n",frame,mode,scene);}
    if(confirmed&&frame==confirmed+6)melee_startup_publish_confirm(0);
    if(mode==GM_TRAINING&&scene==1&&!confirmed&&mnStageSel_NativeFrames()>=60){
        unsigned guidance=mnStageSel_NativeOnettGuidance();
        if(guidance&16){
            int x=(guidance&1)?-45:(guidance&2)?45:0,y=(guidance&4)?-45:(guidance&8)?45:0;
            melee_startup_publish_stick(x,y);centered=(x==0&&y==0)?centered+1:0;
            if(centered>=4){melee_startup_publish_confirm(1);confirmed=frame;}
        }
    }
    if(mode!=GM_TRAINING||scene!=2)return 0;
    if(frame>=2700&&frame<3200)melee_startup_publish_combat(0,frame%120<60?60:-60,0,frame%40<6?PAD_BUTTON_A:frame%40<12?PAD_BUTTON_X:0);
    if(frame==3200)publish(0);
    if(frame==3400||frame==3406||frame==3800||frame==3806)melee_startup_publish_start(frame==3400||frame==3800);
    if(frame==3500){assert(gm_80473814.x01==1);menu_seen=1;}
    if(frame==3550||frame==3556)melee_startup_publish_stick(frame==3550?80:0,0);
    if(frame==3600){assert(gm_80473814.menu_values[0]==3);speed_changed=1;}
    if(frame==3620||frame==3626)melee_startup_publish_stick(frame==3620?-80:0,0);
    if(frame==3700)assert(gm_80473814.menu_values[0]==2);
    if(frame==5000)fprintf(stderr,"Training result: menu=%u speed-change=%u closed=%u stage=%d\n",menu_seen,speed_changed,!gm_80473814.x01,stage_info.grkind);
    return frame>=5000&&menu_seen&&speed_changed&&!gm_80473814.x01&&Player_GetEntity(0)&&Player_GetEntity(1)&&gm_80473814.jobjs[38];
}

int melee_startup_classic_focus(unsigned frame,unsigned target)
{
    if(getenv("MELEE_TEST_HOMERUN")){
        static unsigned entered;static int previous=-1;
        int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
        int key=mode*256+scene;
        if(key!=previous){previous=key;fprintf(stderr,"Home Run navigation: frame=%u mode=%d scene=%d\n",frame,mode,scene);}
        if(mode==GM_HOME_RUN_CONTEST&&scene==1){
            if(!entered)entered=frame;
            if(frame==entered+300){assert(Player_GetEntity(0)&&Player_GetEntity(1));fprintf(stderr,"Home Run checkpoint: live human and Sandbag stage=%d\n",stage_info.grkind);if(!getenv("MELEE_TEST_HOMERUN_COMPLETE"))return 1;}
        }
        if(entered&&mode==GM_HOME_RUN_CONTEST&&scene==0&&frame>entered+600){fprintf(stderr,"Home Run timeout-to-character-select passed\n");return 1;}
        return 0;
    }
    if(getenv("MELEE_TEST_TRAINING"))return melee_startup_training_focus(frame);
    if(target==8)return getenv("MELEE_TEST_ALLSTAR")?melee_startup_allstar_opening(frame):melee_startup_classic_campaign(frame);
    if(target>=9&&target<=18){
        int checkpoint_scene=getenv("MELEE_TEST_ADVENTURE_CHECKPOINT_SCENE")?atoi(getenv("MELEE_TEST_ADVENTURE_CHECKPOINT_SCENE")):32;
        int kirby_checkpoint=target==18&&(getenv("MELEE_TEST_ADVENTURE_KIRBY_CHECKPOINT")||getenv("MELEE_TEST_ADVENTURE_CHECKPOINT_SCENE"));
        if(kirby_checkpoint)assert(checkpoint_scene==32||checkpoint_scene==40||checkpoint_scene==48||checkpoint_scene==56||checkpoint_scene==64||checkpoint_scene==72||checkpoint_scene==80||checkpoint_scene==88);
        if(!kirby_checkpoint){
            if(frame==3600)melee_startup_publish_start(1);
            if(frame==3606)melee_startup_publish_start(0);
        }
        int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
        if(kirby_checkpoint&&frame==2400){
            assert(mode==GM_ADVENTURE&&scene==0);
            fprintf(stderr,"Adventure diagnostic: jumping from initialized intro to scene %d\n",checkpoint_scene);
            if(checkpoint_scene==88)gm_GetAdventureData()->x0.x0.cpu_level=2;
            gm_SetNextGameModeStateId(checkpoint_scene);gm_801A4B60();
        }
        unsigned fighters=0;
        for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data)fighters++;
        if(kirby_checkpoint&&checkpoint_scene>=64){
            static int late_key=-1;static unsigned late_since,late_seen,final_seen;
            int key=mode*256+scene;
            if(key!=late_key){late_key=key;late_since=frame;fprintf(stderr,"Adventure late mode=%d scene=%d frame=%u fighters=%u\n",mode,scene,frame,fighters);}
            unsigned age=frame-late_since;
            if(mode==GM_ADVENTURE){
                if(fighters&&age==120)Player_SetStocks(0,20);
                if(scene==73&&checkpoint_scene==72){
                    HSD_GObj* human=Player_GetEntity(0);
                    if(human&&human->user_data){
                        Fighter* fp=human->user_data;fp->cur_pos=(Vec3){0,60,0};
                        fp->self_vel=(Vec3){0,0,0};fp->x8c_kb_vel=(Vec3){0,0,0};
                    }
                    if(age%600==0){
                        HSD_GObj* manager=Ground_GetMapGObj(10);
                        if(manager&&manager->user_data){Ground* gp=manager->user_data;
                            fprintf(stderr,"Icicle fixture: age=%u fighters=%u phase=%d encounter=%d stocks=%d\n",age,fighters,gp->u.icemt10.x1A,gp->u.icemt10.x14_b3,Player_GetStocks(0));}
                    }
                }

                if(scene==checkpoint_scene+1&&fighters>1)late_seen=1;
                if(scene==checkpoint_scene&&age>180){if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);}
                if((scene==checkpoint_scene+1||(checkpoint_scene==80&&scene==83)||(checkpoint_scene==88&&scene==92))&&age>900&&frame%120==0){
                    for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev)
                        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data&&((Fighter*)g->user_data)->player_id!=0)((Fighter*)g->user_data)->cur_pos.y=-10000;
                }
                if((scene==checkpoint_scene+1||(checkpoint_scene==80&&scene==83)||(checkpoint_scene==88&&scene==92))&&gmVs_GetController_0()->match_over){if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);}
                if(scene==checkpoint_scene+8&&age>180){if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);}
            }
            if(checkpoint_scene==88){
                if(mode==GM_ADVENTURE&&scene>=89&&scene<=93)final_seen|=1u<<(scene-89);
                if(final_seen==31&&mode==GM_ADVENTURE_GOVER&&scene==1&&age>=600){fprintf(stderr,"Adventure Bowser/Giga finale and credits entry passed\n");return 1;}
            }
            if(late_seen&&mode==GM_ADVENTURE&&scene==checkpoint_scene+9&&fighters&&age>=600){fprintf(stderr,"Adventure late checkpoint %d transition passed\n",checkpoint_scene);return 1;}
            return 0;
        }
        if(target>=12&&target<=18){
            static unsigned seen,cleared;static int last_phase=-1;
            if(mode==GM_ADVENTURE&&scene==1&&frame>=4100){
                HSD_GObj* ground=Ground_GetMapGObj(3);assert(ground&&ground->user_data);
                Ground* gp=ground->user_data;
                if(frame==4100)Player_SetStocks(0,20);
                for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev){
                    if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
                    Fighter* fp=g->user_data;
                    if(fp->player_id==0&&frame<4120){
                        Vec3 pos;assert(Ground_801C2D24(0xBD,&pos));
                        if(frame==4100)ftCo_Fall_Enter(g);
                        fp->cur_pos=pos;fp->cur_pos.y+=25;fp->prev_pos=fp->cur_pos;
                        Ground_801C38BC(pos.x,pos.y);
                        fp->self_vel=(Vec3){0,0,0};
                        if(frame==4100)fprintf(stderr,"Yoshi encounter trigger position=(%.1f,%.1f)\n",pos.x,pos.y);
                    }
                    if(fp->player_id!=0&&fp->kind==Ft_Kind_Yoshi){
                        seen=1;if(frame>=4600&&frame%120==0)fp->cur_pos.y=-10000;
                    }
                }
                if(gp->u.kinokoroute2.phase!=last_phase){
                    last_phase=gp->u.kinokoroute2.phase;
                    fprintf(stderr,"Adventure Yoshi encounter phase=%d frame=%u fighters=%u remaining=%u\n",last_phase,frame,fighters,gm_1601_GetUnkData()->x8);
                }
                if(seen&&last_phase==3)cleared=1;
            }
            if(target>=13){
                if(cleared&&mode==GM_ADVENTURE&&scene==1&&frame>=5500&&frame<5800&&!gmVs_GetController_0()->match_over){
                    Vec3 pos;assert(Ground_801C2D24(0x99,&pos));
                    HSD_GObj* human=Player_GetEntity(0);assert(human&&human->user_data);
                    Fighter* fp=human->user_data;
                    if(frame==5500){ftCommon_8007D60C(fp);ftCo_Fall_Enter(human);}
                    fp->cur_pos=pos;fp->cur_pos.y+=15;fp->prev_pos=fp->cur_pos;fp->self_vel=(Vec3){0,0,0};mpColl_80043680(&fp->coll_data,&fp->cur_pos);Ground_801C38BC(pos.x,pos.y);
                    if(frame%30==0)fprintf(stderr,"Adventure course finish trigger=(%.1f,%.1f) motion=%d air=%d hidden=%u detector=%d flags=%x\n",pos.x,pos.y,fp->motion_id,fp->ground_or_air,fp->x221F_b3,stage_info.x714,stage_info.flags);
                }
                if(frame>=5900&&mode==GM_ADVENTURE){
                    if(scene==1&&gmVs_GetController_0()->match_over){
                        if(frame%120==0)melee_startup_publish_start(1);
                        if(frame%120==6)melee_startup_publish_start(0);
                    }
                    if(scene==2){if(frame%120==0)melee_startup_publish_confirm(1);if(frame%120==6)melee_startup_publish_confirm(0);}
                }
                if(frame%300==0&&frame>=5500)fprintf(stderr,"Adventure course exit frame=%u mode=%d scene=%d fighters=%u\n",frame,mode,scene,fighters);
                if(target>=14){
                    static unsigned mario_seen,team_seen,giant_seen;static int last_scene=-1;static unsigned since;
                    if(mode==GM_ADVENTURE&&scene!=last_scene){
                        last_scene=scene;since=frame;melee_startup_publish_start(0);
                        fprintf(stderr,"Adventure jungle scene=%d frame=%u fighters=%u\n",scene,frame,fighters);
                        if(scene==3||scene==9||scene==10||(kirby_checkpoint&&(scene==33||scene==41||scene==49||scene==58)))Player_SetStocks(0,20);
                    }
                    if(mode==GM_ADVENTURE){
                        if(scene==3&&fighters>=3)mario_seen=1;
                        if(scene==9&&fighters>1)team_seen=1;
                        if(scene==10&&fighters>1)giant_seen=1;
                        if((scene==3||scene==9||(target>=15&&scene==10))&&frame-since>480){
                            if(frame%120==0)for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev){
                                if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
                                Fighter* fp=g->user_data;if(fp->player_id!=0)fp->cur_pos.y=-10000;
                            }
                            if(gmVs_GetController_0()->match_over){if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);}
                        }
                        if((scene==8||(target>=15&&scene==16))&&frame-since>180){if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);}
                    }
                    if(frame==12500)fprintf(stderr,"Adventure jungle result: mario=%u team=%u giant=%u mode=%d scene=%d fighters=%u\n",mario_seen,team_seen,giant_seen,mode,scene,fighters);
                    if(target>=16){
                        static unsigned maze_since,link_seen,link_cleared;static int marker=-1,phase=-1;
                        if(mode==GM_ADVENTURE&&scene==17&&fighters){
                            if(!maze_since)maze_since=frame;
                            HSD_GObj* ground=Ground_GetMapGObj(4);assert(ground&&ground->user_data);Ground* gp=ground->user_data;
                            unsigned age=frame-maze_since;
                            if(age>=180&&marker<0){
                                for(unsigned i=0;i<6;i++)if(gp->u.shrineroute.symbols[i]&&((Ground*)gp->u.shrineroute.symbols[i]->user_data)->map_id==1){marker=i;break;}
                                assert(marker>=0);fprintf(stderr,"Maze sword marker index=%d\n",marker);
                            }
                            if(age>=180&&age<200){
                                HSD_GObj* human=Player_GetEntity(0);Fighter* fp=human->user_data;Vec3 pos;assert(Ground_801C2D24(0xBD+marker,&pos));
                                if(age==180){ftCommon_8007D60C(fp);ftCo_Fall_Enter(human);}
                                fp->cur_pos=pos;fp->cur_pos.y+=15;fp->prev_pos=fp->cur_pos;fp->self_vel=(Vec3){0,0,0};
                                mpColl_80043680(&fp->coll_data,&fp->cur_pos);
                                Ground_801C38BC(pos.x,pos.y);
                                if(age==180)fprintf(stderr,"Maze sword drop marker=(%.1f,%.1f)\n",pos.x,pos.y);
                            }
                            for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev){
                                if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;Fighter* fp=g->user_data;
                                if(fp->player_id!=0&&fp->kind==Ft_Kind_Link){link_seen=1;if(age>900&&frame%120==0)fp->cur_pos.y=-10000;}
                            }
                            if(age%120==0){HSD_GObj* h=Player_GetEntity(0);if(h&&h->user_data){Fighter* f=h->user_data;fprintf(stderr,"Maze contact age=%u pos=(%.1f,%.1f) air=%d line=%d detector=%d phase=%d\n",age,f->cur_pos.x,f->cur_pos.y,f->ground_or_air,f->coll_data.floor.index,stage_info.x720,gp->u.shrineroute.xC4);}}
                            if(phase!=gp->u.shrineroute.xC4){phase=gp->u.shrineroute.xC4;fprintf(stderr,"Maze encounter phase=%d frame=%u fighters=%u clearedMask=%x\n",phase,frame,fighters,gp->u.shrineroute.xC6);}
                            if(link_seen&&phase==0&&marker>=0&&(gp->u.shrineroute.xC6&(1<<marker)))link_cleared=1;
                        }
                        if(target>=17){
                            static unsigned exit_since,zelda_seen,samus_seen;
                            if(link_cleared&&mode==GM_ADVENTURE&&scene==17){
                                if(!exit_since)exit_since=frame;
                                if(frame-exit_since>=600&&frame-exit_since<620){
                                    Ground* gp=Ground_GetMapGObj(4)->user_data;int exit_marker=-1;
                                    for(unsigned i=0;i<6;i++)if(gp->u.shrineroute.symbols[i]&&((Ground*)gp->u.shrineroute.symbols[i]->user_data)->map_id==3){exit_marker=i;break;}
                                    assert(exit_marker>=0);Vec3 pos;assert(Ground_801C2D24(0xBD+exit_marker,&pos));
                                    HSD_GObj* human=Player_GetEntity(0);Fighter* fp=human->user_data;
                                    if(frame-exit_since==600){ftCommon_8007D60C(fp);ftCo_Fall_Enter(human);fprintf(stderr,"Maze exit marker=%d pos=(%.1f,%.1f)\n",exit_marker,pos.x,pos.y);}
                                    fp->cur_pos=pos;fp->cur_pos.y+=15;fp->prev_pos=fp->cur_pos;fp->self_vel=(Vec3){0,0,0};
                                    mpColl_80043680(&fp->coll_data,&fp->cur_pos);Ground_801C38BC(pos.x,pos.y);
                                }
                            }
                            if(mode==GM_ADVENTURE){
                                if(scene==18&&fighters>1)zelda_seen=1;
                                if(scene==25&&fighters>1)samus_seen=1;
                                if((scene==18||(target>=18&&scene==25))&&frame-since>480&&frame%120==0){
                                    for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev)if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data&&((Fighter*)g->user_data)->player_id!=0)((Fighter*)g->user_data)->cur_pos.y=-10000;
                                }
                                if(((scene==17||scene==18||(target>=18&&scene==25))&&gmVs_GetController_0()->match_over)||(scene==24&&frame-since>180)){
                                    if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);
                                }
                            }
                            if(target==18){
                                if(mode==GM_ADVENTURE&&scene==26){
                                    if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);
                                }
                                if(frame==19000)fprintf(stderr,"Brinstar escape result: samus=%u mode=%d scene=%d fighters=%u\n",samus_seen,mode,scene,fighters);
                                if(getenv("MELEE_TEST_ESCAPE_COMPLETE")){
                                    static unsigned escape_seen,explosion_seen;
                                    if(mode==GM_ADVENTURE&&scene==27&&fighters==1){
                                        escape_seen=1;
                                        // Controlled course completion exercises the original transition;
                                        // it does not stand in for manual platform traversal.
                                        if(frame-since==900){stage_info.flags|=0x10;fprintf(stderr,"Escape fixture: completing course at frame=%u\n",frame);}
                                        if(gmVs_GetController_0()->match_over){if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);}
                                    }
                                    if(mode==GM_ADVENTURE&&scene==28)explosion_seen=1;
                                    if(mode==GM_ADVENTURE&&scene==32&&frame-since>180){if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);}
                                    if(frame==19000)fprintf(stderr,"Escape completion result: escape=%u explosion=%u mode=%d scene=%d fighters=%u\n",escape_seen,explosion_seen,mode,scene,fighters);
                                    if(getenv("MELEE_TEST_ADVENTURE_KIRBY")){
                                        static unsigned kirby_seen,team_seen,giant_seen;
                                        if(mode==GM_ADVENTURE){
                                            if(scene==33&&fighters>1)kirby_seen=1;
                                            if(scene==35&&fighters>1)team_seen=1;
                                            if(scene==37&&fighters>1)giant_seen=1;
                                            if((scene==33||scene==35||scene==37)&&frame-since>480&&frame%120==0){
                                                for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev)
                                                    if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data&&((Fighter*)g->user_data)->player_id!=0)
                                                        ((Fighter*)g->user_data)->cur_pos.y=-10000;
                                            }
                                            if(((scene==33||scene==35||scene==37)&&gmVs_GetController_0()->match_over)||(scene==40&&frame-since>180)){
                                                if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);
                                            }
                                        }
                                        if(frame==23000)fprintf(stderr,"Kirby progression: single=%u team=%u giant=%u mode=%d scene=%d fighters=%u\n",kirby_seen,team_seen,giant_seen,mode,scene,fighters);
                                        if(getenv("MELEE_TEST_ADVENTURE_STARFOX")){
                                            static unsigned first_fox,ships,second_fox;
                                            if(mode==GM_ADVENTURE){
                                                if(scene==41&&fighters>1)first_fox=1;
                                                if(scene==42)ships=1;
                                                if(scene==43&&fighters>1)second_fox=1;
                                                if((scene==41||scene==43)&&frame-since>480&&frame%120==0){
                                                    for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev)
                                                        if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data&&((Fighter*)g->user_data)->player_id!=0)
                                                            ((Fighter*)g->user_data)->cur_pos.y=-10000;
                                                }
                                                if(((scene==41||scene==43)&&gmVs_GetController_0()->match_over)||(scene==48&&frame-since>180)){
                                                    if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);
                                                }
                                            }
                                            if(getenv("MELEE_TEST_ADVENTURE_FZERO")){
                                                static unsigned pokemon_seen,intro_seen,course_seen;
                                                if(mode==GM_ADVENTURE){
                                                    if(scene==49&&fighters>1)pokemon_seen=1;
                                                    if(scene==56)intro_seen=1;
                                                    if(scene==58&&fighters==1)course_seen=1;
                                                    if(scene==58&&(frame-since==120||frame-since==980)){
                                                        HSD_GObj* manager=Ground_GetMapGObj(31);assert(manager&&manager->user_data);
                                                        Ground* gp=manager->user_data;assert(gp->u.bigblueroute2.car_gobj==Ground_GetMapGObj(4));
                                                        assert(gp->u.bigblueroute2.xC8>=0&&gp->u.bigblueroute2.xC8<=3);
                                                        fprintf(stderr,"F-Zero native manager: car pointer intact, checkpoint=%d age=%u\n",gp->u.bigblueroute2.xC8,frame-since);
                                                    }
                                                    if(scene==49&&frame-since>480&&frame%120==0){
                                                        for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev)
                                                            if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data&&((Fighter*)g->user_data)->player_id!=0)
                                                                ((Fighter*)g->user_data)->cur_pos.y=-10000;
                                                    }
                                                    if(scene==58&&frame-since>=900&&frame-since<920){
                                                        HSD_GObj* h=Player_GetEntity(0);assert(h&&h->user_data);Fighter* fp=h->user_data;Vec3 pos;
                                                        assert(Ground_801C2D24(0x99,&pos));
                                                        if(frame-since==900){ftCommon_8007D60C(fp);ftCo_Fall_Enter(h);fprintf(stderr,"F-Zero goal fixture: (%.1f,%.1f)\n",pos.x,pos.y);}
                                                        fp->cur_pos=pos;fp->cur_pos.y+=15;fp->prev_pos=fp->cur_pos;fp->self_vel=(Vec3){0,0,0};
                                                        mpColl_80043680(&fp->coll_data,&fp->cur_pos);Ground_801C38BC(pos.x,pos.y);
                                                    }
                                                    if(((scene==49||scene==58)&&gmVs_GetController_0()->match_over)||(scene==57&&frame-since>180)){
                                                        if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);
                                                    }
                                                    if(scene==58&&frame-since==1800&&!gmVs_GetController_0()->match_over){fprintf(stderr,"F-Zero goal fixture did not finish course: detector=%d flags=%x\n",stage_info.x714,stage_info.flags);_Exit(6);}
                                                }
                                                if(getenv("MELEE_TEST_ADVENTURE_ONETT_ENTRY")){
                                                    static unsigned captain_seen;
                                                    if(mode==GM_ADVENTURE){
                                                        if(scene==59&&fighters>1)captain_seen=1;
                                                        if(scene==59&&frame-since>600&&frame%120==0){
                                                            for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev)
                                                                if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data&&((Fighter*)g->user_data)->player_id!=0)
                                                                    ((Fighter*)g->user_data)->cur_pos.y=-10000;
                                                        }
                                                        if((scene==59&&gmVs_GetController_0()->match_over)||(scene==64&&frame-since>180)){
                                                            if(frame%120==0)melee_startup_publish_start(1);if(frame%120==6)melee_startup_publish_start(0);
                                                        }
                                                    }
                                                    return intro_seen&&course_seen&&captain_seen&&mode==GM_ADVENTURE&&scene==65&&fighters>0&&frame-since>=600;
                                                }
                                                return (pokemon_seen||(kirby_checkpoint&&checkpoint_scene==56))&&intro_seen&&course_seen&&mode==GM_ADVENTURE&&scene==59&&fighters>1&&frame-since>=600;
                                            }
                                            return first_fox&&ships&&second_fox&&mode==GM_ADVENTURE&&scene==49&&fighters>1&&frame-since>=600;
                                        }
                                        return (kirby_checkpoint||(link_cleared&&zelda_seen&&samus_seen&&escape_seen&&explosion_seen))&&kirby_seen&&team_seen&&giant_seen&&mode==GM_ADVENTURE&&scene==41&&fighters>1&&frame-since>=600;
                                    }
                                    return link_cleared&&zelda_seen&&samus_seen&&escape_seen&&explosion_seen&&mode==GM_ADVENTURE&&scene==33&&fighters>1&&frame-since>=600;
                                }
                                return link_cleared&&zelda_seen&&samus_seen&&mode==GM_ADVENTURE&&scene==27&&fighters==1&&frame-since>=600;
                            }
                            if(frame==18000)fprintf(stderr,"Maze exit result: link=%u cleared=%u zelda=%u samus=%u mode=%d scene=%d\n",link_seen,link_cleared,zelda_seen,samus_seen,mode,scene);
                            return link_cleared&&zelda_seen&&samus_seen&&mode==GM_ADVENTURE&&scene==25;
                        }
                        if(frame==16500)fprintf(stderr,"Maze Link result: seen=%u cleared=%u mode=%d scene=%d\n",link_seen,link_cleared,mode,scene);
                        return cleared&&mario_seen&&team_seen&&giant_seen&&link_seen&&link_cleared&&mode==GM_ADVENTURE&&scene==17;
                    }
                    if(target==15){
                        if(frame==14500)fprintf(stderr,"Adventure maze result: mode=%d scene=%d fighters=%u\n",mode,scene,fighters);
                        return cleared&&mario_seen&&team_seen&&giant_seen&&mode==GM_ADVENTURE&&scene==17&&fighters>0;
                    }
                    return cleared&&mario_seen&&team_seen&&giant_seen&&mode==GM_ADVENTURE&&scene==10&&fighters>1;
                }
                return cleared&&mode==GM_ADVENTURE&&scene==3&&fighters>=3;
            }
            if(frame==7500)fprintf(stderr,"Adventure Yoshi encounter result: seen=%u cleared=%u mode=%d scene=%d\n",seen,cleared,mode,scene);
            return seen&&cleared&&mode==GM_ADVENTURE&&scene==1;
        }
        if(target==11){
            static unsigned blocks;
            if(frame==4100){
                assert(mode==GM_ADVENTURE&&scene==1&&fighters==1);
                HSD_GObj* ground=Ground_GetMapGObj(3);assert(ground&&ground->user_data);
                Ground* gp=ground->user_data;
                for(HSD_GObj* g=plinklow_gobjs[9],*next;g;g=next){
                    next=g->prev;if(g->classifier!=HSD_GOBJ_CLASS_ITEM||!g->user_data)continue;
                    Item* item=g->user_data;
                    if(item->kind!=It_PKind_Random||item->xDD4_itemVar.yaku.x0!=8||item->xDD4_itemVar.yaku.x10!=gp)continue;
                    HSD_JObj* joint=item->xDD4_itemVar.yaku.x4;assert(joint&&!(joint->flags&JOBJ_HIDDEN));
                    gp->u.kinokoroute2.flags_0=false;
                    it_2E6A_Logic117_DmgReceived(g);
                    assert((joint->flags&JOBJ_HIDDEN)&&gp->u.kinokoroute2.flags_0);
                    blocks++;
                }
                fprintf(stderr,"Adventure block damage: %u hidden models and stage flag updates\n",blocks);assert(blocks==51);
            }
            if(frame==5000)fprintf(stderr,"Adventure blocks result: count=%u mode=%d scene=%d fighters=%u\n",blocks,mode,scene,fighters);
            return blocks==51&&mode==GM_ADVENTURE&&scene==1&&fighters==1;
        }
        if(target==10){
            static float max_x=-10000;static unsigned entered;
            Fighter* human=NULL;
            for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev)
                if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data&&((Fighter*)g->user_data)->player_id==0)human=g->user_data;
            if(mode==GM_ADVENTURE&&scene==1&&human){
                entered=1;if(human->cur_pos.x>max_x)max_x=human->cur_pos.x;
                if(frame>=3900){
                    unsigned t=frame-3900,buttons=0;
                    if(t%90<6)buttons|=PAD_BUTTON_X;
                    if(t%90>=35&&t%90<41)buttons|=PAD_BUTTON_X;
                    if(t%40<6)buttons|=PAD_BUTTON_A;
                    melee_startup_publish_combat(0,80,0,buttons);
                }
                if(frame%120==0)fprintf(stderr,"Adventure traversal frame=%u pos=(%.1f,%.1f) motion=%d damage=%.1f maxX=%.1f\n",frame,human->cur_pos.x,human->cur_pos.y,human->motion_id,human->dmg.x1830_percent,max_x);
            }
            if(frame==9000)fprintf(stderr,"Adventure traversal result: entered=%u mode=%d scene=%d maxX=%.1f\n",entered,mode,scene,max_x);
            return entered&&mode==GM_ADVENTURE&&scene>1&&scene<100;
        }
        if(frame==5000)fprintf(stderr,"Adventure opening: mode=%d scene=%d fighters=%u ground=%d\n",mode,scene,fighters,stage_info.grkind);
        return mode==GM_ADVENTURE&&scene==1&&fighters>0;
    }
    static int entered,lasers_seen,lasers_cleared,defeated,credits_entered,congrats_entered;
    static int unlock_kind=-1;
    int target_course=target==1&&getenv("MELEE_TEST_TARGET_COURSE");
    unsigned round=target_course?2:target==1?8:10;
    if(frame==1900){
        if(gm_GetCurrentGameMode()!=GM_CLASSIC||gm_GetCurrentSceneIndex()!=112)_Exit(6);
        /* CSS exit reads this native Classic starting-round setting and
         * constructs the normal round data, including its original rules. */
        gmMainLib_8015CDC8()->x5=round;
    }
    unsigned fighters=0,bosses=0;
    for(HSD_GObj* g=plinklow_gobjs?plinklow_gobjs[8]:NULL;g;g=g->prev){
        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
        Fighter* fp=g->user_data;fighters++;
        if(fp->kind==Ft_Kind_MasterH||fp->kind==Ft_Kind_CrezyH)bosses++;
        if(target>=3&&fp->kind==Ft_Kind_MasterH){
            if(frame==2800)ftMh_MS_359_80152BCC(g);
            unsigned lasers=!!FT_MH_LASER(fp,x34)+!!FT_MH_LASER(fp,x38)+!!FT_MH_LASER(fp,x3C)+!!FT_MH_LASER(fp,x40);
            if(lasers==4)lasers_seen=1;
            if(target>=4&&lasers==4&&!defeated){
                Fighter_TakeDamage_8006CC7C(fp,200.0f);
                defeated=Player_GetRemainingHPByIndex(fp->player_id,fp->x221F_b4)==0&&fp->motion_id==ftMh_MS_Damage;
                fprintf(stderr,"Master Hand defeat during lasers: hp=%d motion=%d defeated=%d\n",Player_GetRemainingHPByIndex(fp->player_id,fp->x221F_b4),fp->motion_id,defeated);
            }
            if(lasers_seen&&!lasers&&fp->motion_id==ftMh_MS_FingerBeamEnd)lasers_cleared=1;
        }
    }
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    if(mode==GM_CLASSIC&&scene==(int)(round*8+1)&&fighters>0)entered=1;
    if(target_course){
        static unsigned broken;
        if(frame==3500){
            assert(entered&&mode==GM_CLASSIC&&scene==17&&stage_info.x6D4==10);
            assert(Player_GetEntity(0)&&Player_GetPlayerCharacter(0)==atoi(getenv("MELEE_TEST_TARGET_COURSE")));
            for(HSD_GObj* g=plinklow_gobjs[9],*next;g;g=next){
                next=g->prev;Item* item=g->user_data;
                if(item&&item->kind==It_Kind_Mato){assert(it_802D85F4(g));Item_8026A8EC(g);broken++;}
            }
            assert(broken==10&&!stage_info.x6D4);
            fprintf(stderr,"Target course: original ten targets cleared at frame %u\n",frame);
        }
        if(frame>3600&&mode==GM_CLASSIC){
            if(frame%90==0)melee_startup_publish_start(1);
            if(frame%90==6)melee_startup_publish_start(0);
        }
        if(frame==5000)fprintf(stderr,"Target course result: kind=%s cleared=%u mode=%d scene=%d fighters=%u\n",getenv("MELEE_TEST_TARGET_COURSE"),broken,mode,scene,fighters);
        return entered&&broken==10&&mode==GM_CLASSIC&&scene==25&&fighters>=2;
    }

    if(frame==5000)fprintf(stderr,"Classic focused round %u: mode=%d scene=%d fighters=%u bosses=%u ground=%d\n",round,mode,scene,fighters,bosses,stage_info.grkind);
    if(target>=4&&frame>3200&&mode==GM_CLASSIC&&scene==81&&gmVs_GetController_0()->match_over){
        if(frame%90==0)melee_startup_publish_start(1);
        if(frame%90==6)melee_startup_publish_start(0);
    }
    if(mode==GM_CLASSIC_GOVER&&scene==1)credits_entered=1;
    if(target>=6){
        if(frame==4900)melee_startup_publish_start(0);
        if(frame==5000){assert(mode==GM_CLASSIC_GOVER&&scene==1);melee_startup_publish_start(1);}
        if(frame==5006)melee_startup_publish_start(0);
        if(mode==GM_CLASSIC_GOVER&&scene==3)congrats_entered=1;
        if(frame==7000){assert(mode==GM_CLASSIC_GOVER&&scene==3);melee_startup_publish_confirm(1);}
        if(frame==7006)melee_startup_publish_confirm(0);
        if(frame==7300&&mode==GM_CHALLENGER_APPROACH&&scene==0)melee_startup_publish_confirm(1);
        if(frame==7306)melee_startup_publish_confirm(0);
        if(target==7){
            if(frame==7500&&mode==GM_CHALLENGER_APPROACH&&scene==1)Player_SetStocks(gm_GetChallengerData()->human_slot,10);
            if(frame==8000){assert(mode==GM_CHALLENGER_APPROACH&&scene==1&&fighters==2);unlock_kind=gm_GetChallengerData()->cpu_ckind;}
            if(frame==8100&&mode==GM_CHALLENGER_APPROACH&&scene==1){
                for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev)if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data){
                    Fighter* fp=g->user_data;if(fp->player_id!=gm_GetChallengerData()->human_slot)fp->cur_pos.y=-10000;
                }
            }
            if(frame>8200&&mode==GM_CHALLENGER_APPROACH){
                if(scene==1&&gmVs_GetController_0()->match_over){if(frame%90==0)melee_startup_publish_start(1);if(frame%90==6)melee_startup_publish_start(0);}
                if(scene==2){if(frame%120==0)melee_startup_publish_confirm(1);if(frame%120==6)melee_startup_publish_confirm(0);}
            }
            if(frame==11500)fprintf(stderr,"Classic unlock win: kind=%d unlocked=%d mode=%d scene=%d\n",unlock_kind,unlock_kind>=0&&gm_IsCKindUnlocked(unlock_kind),mode,scene);
            return entered&&defeated&&credits_entered&&congrats_entered&&unlock_kind>=0&&gm_IsCKindUnlocked(unlock_kind)&&mode==GM_MENU;
        }
        if(frame==8000)fprintf(stderr,"Classic ending exit: credits=%d congratulations=%d mode=%d scene=%d\n",credits_entered,congrats_entered,mode,scene);
        return entered&&defeated&&credits_entered&&congrats_entered&&(mode==GM_MENU||(mode==GM_CHALLENGER_APPROACH&&scene==1&&fighters==2));
    }
    if(target==5){
        if(frame==10500)fprintf(stderr,"Classic credits completion: entered=%d mode=%d scene=%d\n",credits_entered,mode,scene);
        return entered&&defeated&&credits_entered&&mode==GM_CLASSIC_GOVER&&(scene==2||scene==3);
    }
    if(target==4)return entered&&defeated&&mode==GM_CLASSIC_GOVER&&scene==1;
    if(frame==5000&&target==3)fprintf(stderr,"Master Hand finger lasers: four spawned=%d cleared at attack end=%d\n",lasers_seen,lasers_cleared);
    return (target!=3||(lasers_seen&&lasers_cleared))&&mode==GM_CLASSIC&&entered&&(target==1?(scene==65||scene==72||scene==73):(scene==81&&bosses>0));
}

int melee_startup_hand_laser_state_test(void)
{
    static Fighter fp;
    static HSD_GObj lasers[4];
    FT_MH_LASER(&fp,x34)=&lasers[0];FT_MH_LASER(&fp,x38)=&lasers[1];
    FT_MH_LASER(&fp,x3C)=&lasers[2];FT_MH_LASER(&fp,x40)=&lasers[3];
    fp.mv.mh.unk0.x28=320004;fp.mv.mh.unk0.x2C=320005;fp.mv.mh.unk0.x30=320006;
    assert(FT_MH_LASER(&fp,x34)==&lasers[0]&&FT_MH_LASER(&fp,x38)==&lasers[1]&&
           FT_MH_LASER(&fp,x3C)==&lasers[2]&&FT_MH_LASER(&fp,x40)==&lasers[3]);
    fp.mv.mh.unk0.x28=fp.mv.mh.unk0.x2C=fp.mv.mh.unk0.x30=-1;
    assert(FT_MH_LASER(&fp,x34)==&lasers[0]&&FT_MH_LASER(&fp,x38)==&lasers[1]);
    FT_CH_LASER(&fp,x28)=&lasers[0];FT_CH_LASER(&fp,x2C)=&lasers[1];
    FT_CH_LASER(&fp,x30)=&lasers[2];FT_CH_LASER(&fp,x34)=&lasers[3];
    fp.mv.ch.unk0.x38=320004;fp.mv.ch.unk0.x3C=320005;fp.mv.ch.unk0.x40=320006;
    assert(FT_CH_LASER(&fp,x28)==&lasers[0]&&FT_CH_LASER(&fp,x2C)==&lasers[1]&&
           FT_CH_LASER(&fp,x30)==&lasers[2]&&FT_CH_LASER(&fp,x34)==&lasers[3]);
    fp.mv.ch.unk0.x38=fp.mv.ch.unk0.x3C=fp.mv.ch.unk0.x40=-1;
    assert(FT_CH_LASER(&fp,x30)==&lasers[2]&&FT_CH_LASER(&fp,x34)==&lasers[3]);
    puts("Boss laser state: both hands retain all four pointers across sound ID updates and cleanup");return 0;
}

int melee_startup_empty_demo_motion_test(void)
{
    static Fighter fp;
    static struct Fighter_WaitAnimData empty;
    static FigaTree stale;
    fp.kind=Ft_Kind_Fox;fp.x24=&empty;fp.x58C=1;
    fp.x590=fp.x598=&stale;fp.x5A4=fp.x5A8=&stale;
    ftData_80085CD8(&fp,&fp,0);
    assert(!fp.x590&&!fp.x5A4);
    assert(!ftData_80085E50(&fp,0)&&!fp.x598&&!fp.x5A8);
    puts("Empty demo motion: primary and secondary animation slots clear stale state without an owner");return 0;
}


int melee_startup_classic_campaign(unsigned frame)
{
    static int last=-1,credits,congrats,unlock_kind=-1;
    static unsigned entered,rounds,broken;
    if(frame<2700)return 0;
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex(),key=mode*256+scene;
    if(key!=last){
        fprintf(stderr,"Continuous Classic: frame=%u mode=%d scene=%d rounds=%03x\n",frame,mode,scene,rounds);
        last=key;entered=frame;melee_startup_publish_confirm(0);
        if(mode==GM_CLASSIC&&scene<=81&&scene%8==1){
            unsigned round=scene/8;rounds|=1u<<round;
            if(round!=2&&round!=5&&round!=8)Player_SetStocks(0,20);
        }
        if(mode==GM_CHALLENGER_APPROACH&&scene==1){unlock_kind=gm_GetChallengerData()->cpu_ckind;Player_SetStocks(gm_GetChallengerData()->human_slot,20);}
    }
    unsigned age=frame-entered;
    if(frame%90==6)melee_startup_publish_confirm(0);
    if(mode==GM_CLASSIC){
        if(scene<=80&&scene%8==0){if(age>120&&frame%90==0)melee_startup_publish_confirm(1);}
        if(scene<=81&&scene%8==1){
            unsigned round=scene/8;
            if(!gmVs_GetController_0()->match_over&&age>=480&&frame%120==0){
                if(round==2&&!broken){
                    for(HSD_GObj* g=plinklow_gobjs[9],*next;g;g=next){
                        next=g->prev;Item* item=g->user_data;
                        if(item&&item->kind==It_Kind_Mato){assert(it_802D85F4(g));Item_8026A8EC(g);broken++;}
                    }
                    assert(broken==10&&!stage_info.x6D4);
                }else if(round!=2&&round!=5&&round!=8){
                    for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev){
                        if(g->classifier!=HSD_GOBJ_CLASS_FIGHTER||!g->user_data)continue;
                        Fighter* fp=g->user_data;
                        int enemy=(round==1||round==4)?Player_GetTeam(fp->player_id)!=Player_GetTeam(0):fp->player_id!=0;
                        if(!enemy)continue;
                        if(fp->kind==Ft_Kind_MasterH||fp->kind==Ft_Kind_CrezyH){
                            if(Player_GetRemainingHPByIndex(fp->player_id,fp->x221F_b4)>0)Fighter_TakeDamage_8006CC7C(fp,999.0f);
                        }else fp->cur_pos.y=-10000;
                    }
                }
            }
            if(gmVs_GetController_0()->match_over&&age>120&&frame%90==0)melee_startup_publish_start(1);
        }
        if(scene==105){fprintf(stderr,"Unexpected game over in continuous Classic\n");_Exit(6);}
    }else if(mode==GM_CLASSIC_GOVER){
        if(scene==1)credits=1;
        if(scene==3){congrats=1;if(age>180&&frame%90==0)melee_startup_publish_confirm(1);}
    }else if(mode==GM_CHALLENGER_APPROACH){
        if(scene==0&&age>240&&frame%90==0)melee_startup_publish_confirm(1);
        if(scene==1){
            if(age>480&&frame%120==0&&!gmVs_GetController_0()->match_over){
                for(HSD_GObj* g=plinklow_gobjs[8];g;g=g->prev)if(g->classifier==HSD_GOBJ_CLASS_FIGHTER&&g->user_data){
                    Fighter* fp=g->user_data;if(fp->player_id!=gm_GetChallengerData()->human_slot)fp->cur_pos.y=-10000;
                }
            }
            if(gmVs_GetController_0()->match_over&&frame%90==0)melee_startup_publish_start(1);
        }
        if(scene==2&&age>120&&frame%90==0)melee_startup_publish_confirm(1);
    }
    int complete=rounds==0x7ff&&broken==10&&credits&&congrats&&mode==GM_MENU&&(unlock_kind<0||gm_IsCKindUnlocked(unlock_kind));
    if(complete||frame==24000)fprintf(stderr,"Continuous Classic result: rounds=%03x targets=%u credits=%d congratulations=%d unlock=%d unlocked=%d mode=%d scene=%d\n",rounds,broken,credits,congrats,unlock_kind,unlock_kind>=0&&gm_IsCKindUnlocked(unlock_kind),mode,scene);
    return complete;
}

int melee_startup_adventure_entry_progress(void)
{
    int mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    fprintf(stderr,"Adventure entry: mode=%d scene=%d (expected 4/112)\n",mode,scene);
    return mode==GM_ADVENTURE&&scene==112;
}

#include <melee/it/kinds/it_2E5A.h>
int melee_startup_match_coins(unsigned frame)
{
    static unsigned tiers;
    if(frame==2100){
        HSD_GObj* human=Player_GetEntity(0);assert(human&&human->user_data);
        Fighter* fp=human->user_data;
        const int amounts[]={1,5,10};
        for(unsigned i=0;i<3;i++){
            Vec3 pos=fp->cur_pos,velocity={0,0,0};pos.x+=20*(int)i-20;pos.y+=25;
            it_802E5F00(human,&pos,&velocity,amounts[i]);
        }
    }
    if(frame>=2100&&frame<=2400){
        for(HSD_GObj* g=plinklow_gobjs[9];g;g=g->prev){
            if(g->classifier!=HSD_GOBJ_CLASS_ITEM||!g->user_data)continue;
            Item* item=g->user_data;if(item->kind==It_Kind_Unk4){
                unsigned tier=item->xDD4_itemVar.it_2E5A.x4;assert(tier<3);tiers|=1u<<tier;
            }
        }
    }
    if(frame==2400)fprintf(stderr,"Match coin regression: observed tier mask=%x\n",tiers);
    return tiers==7;
}

#include <melee/it/kinds/itwhitebea.h>
int melee_startup_ottosea_link_test(void)
{
    static Item enemy;HSD_GObj owner={0},ice={0};owner.user_data=&enemy;
    enemy.xDD4_itemVar.oldottosea.x20=&ice;
    enemy.xDD4_itemVar.oldottosea.x24=11;enemy.xDD4_itemVar.oldottosea.x28=22;
    it_802E37A4(&owner);
    assert(!enemy.xDD4_itemVar.oldottosea.x20);
    assert(enemy.xDD4_itemVar.oldottosea.x24==11&&enemy.xDD4_itemVar.oldottosea.x28==22);
    it_802E37A4(NULL);
    puts("Ottosea ice cleanup: native owner pointer clears without overwriting adjacent timers");return 0;
}

int melee_startup_player_mapping_test(void)
{
    for(int kind=0;kind<ChKind_Max;kind++){
        Player_SetPlayerCharacter(0,kind);
        assert(Player_80032610(0,0)==Player_800325C8(kind,0));
        assert(Player_80032610(0,1)==Player_800325C8(kind,1));
    }
    puts("Player mapping: all character IDs and secondary fighters match the explicit mapping table");return 0;
}
