/* Read-only boundary state for the browser UI and integration tests.
 * Online publishes after each logic tick; offline publishes after rendering.
 * The game thread owns this snapshot; JavaScript never follows game pointers. */
#include <melee/gm/gm_1A3F.h>
#include <melee/gm/gmvsmode.h>
#include <melee/gm/forward.h>
#include <melee/ft/types.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/gobj.h>
#include <emscripten.h>
#include <sysdolphin/baselib/random.h>
extern unsigned melee_browser_net_tick_count(void);
extern bool mnCharSel_NativeTargetDelta(unsigned,unsigned,float*,float*);
extern unsigned mnStageSel_NativeOnettGuidance(void);
void melee_browser_publish_state(unsigned frame) {
    unsigned mode=gm_GetCurrentGameMode(),scene=gm_GetCurrentSceneIndex();
    float values[28]={0};
    unsigned guidance=0;
    if(mode==GM_VS && scene==gmVsMode_State_Css){
        mnCharSel_NativeTargetDelta(0,CKind_Mario,&values[0],&values[1]);
        mnCharSel_NativeTargetDelta(1,CKind_Fox,&values[2],&values[3]);
    }
    if(mode==GM_VS && scene==gmVsMode_State_Sss)guidance=mnStageSel_NativeOnettGuidance();
    if(mode==GM_VS && (scene==gmVsMode_State_Vs||scene==gmVsMode_State_SuddenDeath)){
        for(unsigned slot=0;slot<2;slot++){
            HSD_GObj* object=Player_GetEntity(slot);
            HSD_GObj* live=plinklow_gobjs?plinklow_gobjs[8]:NULL;
            while(live&&live!=object)live=live->prev;
            if(!live||live->classifier!=HSD_GOBJ_CLASS_FIGHTER||!live->user_data)continue;
            Fighter* fp=live->user_data;float* out=values+4+slot*12;
            out[0]=1;out[1]=fp->kind;out[2]=fp->cur_pos.x;out[3]=fp->cur_pos.y;
            out[4]=fp->motion_id;out[5]=fp->dmg.x1830_percent;out[6]=fp->player_id;
            out[7]=Player_GetStocks(slot);out[8]=fp->facing_dir;
            out[9]=fp->self_vel.x;out[10]=fp->self_vel.y;out[11]=fp->ground_or_air;
        }
    }
    MAIN_THREAD_EM_ASM({
        const v=Array.from(HEAPF32.subarray($4>>2,($4>>2)+28));
        Module.meleeState=({frame:$0,mode:$1,scene:$2,stageGuidance:$3,netTick:$5,rng:$6>>>0,
          cursorTargets:[v.slice(0,2),v.slice(2,4)],fighters:[v.slice(4,16),v.slice(16,28)]});
        Module.onGameState?.(Module.meleeState);
        Module.netSession?.observeState?.(Module.meleeState);
    },frame,mode,scene,guidance,values,melee_browser_net_tick_count(),*seed_ptr);
}
