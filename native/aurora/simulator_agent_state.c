/* Read-only observations, called on the game thread. Simulator testing only. */
#include <melee/gm/gm_1A3F.h>
#include <melee/gm/forward.h>
#include <melee/ft/types.h>
#include <melee/pl/player.h>
#include <melee/mn/mnmain.h>
#include <melee/gr/ground.h>
#include <melee/gr/stage.h>
#include <melee/mp/mplib.h>
#include <melee/it/types.h>
#include <math.h>
#include <melee/gr/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stddef.h>
extern bool mnCharSel_NativeAgentTargets(float*);
void melee_sim_agent_observe(char* out, size_t capacity) {
    unsigned mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    size_t n=snprintf(out,capacity,"{\"mode\":%u,\"scene\":%u,\"menu\":%u,\"selection\":%u,\"stage\":%d,\"fighters\":[",mode,scene,mn_804A04F0.cur_menu,mn_804A04F0.hovered_selection,stage_info.grkind);
    unsigned count=0; Fighter* player=NULL;
    /* Validate entity membership before reading its user data during transitions. */
    for(unsigned slot=0;slot<6;slot++) {
        HSD_GObj* obj=Player_GetEntity(slot),*live=plinklow_gobjs?plinklow_gobjs[8]:NULL;
        while(live&&live!=obj)live=live->prev;
        if(!live||live->classifier!=HSD_GOBJ_CLASS_FIGHTER||!live->user_data)continue;
        Fighter* f=live->user_data;
        if(slot==0)player=f;
        if(n+512>=capacity)break;
        n+=snprintf(out+n,capacity-n,"%s{\"slot\":%u,\"kind\":%d,\"x\":%.2f,\"y\":%.2f,\"vx\":%.2f,\"vy\":%.2f,\"action\":%d,\"damage\":%.1f,\"stocks\":%d,\"facing\":%.0f,\"airborne\":%d,\"jumpsUsed\":%u}",count++?",":"",slot,f->kind,f->cur_pos.x,f->cur_pos.y,f->self_vel.x,f->self_vel.y,f->motion_id,f->dmg.x1830_percent,Player_GetStocks(slot),f->facing_dir,f->ground_or_air,f->x1968_jumpsUsed);
    }
    n+=snprintf(out+n,capacity-n,"]");
    if(mode==4 && player){
        n+=snprintf(out+n,capacity-n,",\"terrain\":[");
        const float offsets[]={-100,-60,-20,0,20,60,100};
        for(unsigned i=0;i<7;i++){
            float x=player->cur_pos.x+offsets[i];Vec3 hit,normal;int line;u32 flags;
            bool found=mpCheckFloor(x,player->cur_pos.y+100,x,player->cur_pos.y-300,0,&hit,&line,&flags,&normal,-1,-1,-1,NULL,NULL);
            if(found)n+=snprintf(out+n,capacity-n,"%s{\"dx\":%.0f,\"dy\":%.1f}",i?",":"",offsets[i],hit.y-player->cur_pos.y);
            else n+=snprintf(out+n,capacity-n,"%s{\"dx\":%.0f,\"dy\":null}",i?",":"",offsets[i]);
        }
        n+=snprintf(out+n,capacity-n,"],\"objects\":[");
        unsigned items=0;
        for(HSD_GObj* o=plinklow_gobjs[9];o && items<12;o=o->prev){
            if(o->classifier!=HSD_GOBJ_CLASS_ITEM||!o->user_data)continue;
            Item* it=o->user_data;
            if(fabsf(it->pos.x-player->cur_pos.x)>120||fabsf(it->pos.y-player->cur_pos.y)>100)continue;
            n+=snprintf(out+n,capacity-n,"%s{\"kind\":%d,\"dx\":%.1f,\"dy\":%.1f}",items++?",":"",it->kind,it->pos.x-player->cur_pos.x,it->pos.y-player->cur_pos.y);
        }
        n+=snprintf(out+n,capacity-n,"]");
    }
    float css[11];
    if(mode==4&&scene==0x70&&mnCharSel_NativeAgentTargets(css))
        n+=snprintf(out+n,capacity-n,",\"css\":{\"x\":%.2f,\"y\":%.2f,\"difficultyDown\":[%.2f,%.2f],\"stocksUp\":[%.2f,%.2f],\"difficulty\":%.0f,\"stocks\":%.0f,\"heldToken\":%.0f,\"kirby\":[%.2f,%.2f]}",css[0],css[1],css[2],css[3],css[4],css[5],css[6],css[7],css[8],css[9],css[10]);
    snprintf(out+n,capacity-n,"}");
}

#include "melee_input.h"
#include "melee_pad_backend.h"
void melee_sim_agent_input(unsigned buttons,float x,float y,float cx,float cy) {
    PADStatus pads[4]={melee_analog_pad(buttons,x,y,cx,cy,0,0),{0},{0},{0}};
    for(int i=1;i<4;i++)pads[i].err=-1;
    melee_pad_publish(pads,0);
}
