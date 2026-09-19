#include "melee_reset.h"
#include <dolphin/os.h>
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

static MeleeResetRequest captured;
static int cookie;
static void transfer_to_host(MeleeResetRequest request, void* context)
{
    assert(context==&cookie);
    assert(OSDisableInterrupts()); /* Reset must release the old game gate. */
    OSRestoreInterrupts(1);
    captured=request;
    pthread_exit(&cookie);
}
static void* run_game(void* kind)
{
    assert(OSDisableInterrupts());
    OSResetSystem((int)(intptr_t)kind,0x12345678,1);
    assert(!"Reset returned into the old game");
    return NULL;
}
int main(void)
{
    assert(OSGetResetCode()==0);
    for(int kind=0;kind<=2;kind++){
        melee_reset_configure(0x80000000,transfer_to_host,&cookie);
        assert(OSGetResetCode()==0x80000000UL);
        pthread_t thread;assert(!pthread_create(&thread,NULL,run_game,(void*)(intptr_t)kind));
        void* result=NULL;assert(!pthread_join(thread,&result)&&result==&cookie);
        assert(captured.kind==kind&&captured.code==0x12345678&&captured.force_menu==1);
        assert(OSDisableInterrupts());OSRestoreInterrupts(1);
    }
    melee_reset_configure(0,NULL,NULL);assert(OSGetResetCode()==0);
    puts("Native reset: boot reason, three request kinds and non-returning host handoff passed");
}
