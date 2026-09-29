#include "melee_card_backend.h"
#include <stdio.h>
#include <unistd.h>
#include <emscripten.h>
int melee_browser_open_card(void) {
    if(MAIN_THREAD_EM_ASM_INT({return Module.disableSaving?1:0;}))return 0;
    if(access("/save/import.card",F_OK)==0){
        s32 error;
        MeleeCardStore* imported=melee_card_store_open("/save/import.card",false,&error);
        if(!imported)return error;
        melee_card_store_close(imported);
        if(rename("/save/import.card","/save/card"))return CARD_RESULT_IOERROR;
    }
    return melee_card_insert(0,"/save/card",true);
}
