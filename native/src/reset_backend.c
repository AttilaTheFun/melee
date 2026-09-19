#include "melee_reset.h"
#include <dolphin/os.h>
#include <pthread.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t boot_code;
static MeleeResetHandler handler;
static void* handler_context;
static int resetting;

void melee_reset_configure(uint32_t code, MeleeResetHandler callback, void* context)
{
    pthread_mutex_lock(&lock);
    boot_code=code;handler=callback;handler_context=context;resetting=0;
    pthread_mutex_unlock(&lock);
}

unsigned long OSGetResetCode(void)
{
    pthread_mutex_lock(&lock);
    uint32_t code=boot_code;
    pthread_mutex_unlock(&lock);
    return code;
}

void OSResetSystem(int kind, u32 code, BOOL force_menu)
{
    if(kind<OS_RESET_RESTART||kind>OS_RESET_SHUTDOWN)
        OSPanic(__FILE__,__LINE__,"Invalid native reset kind");
    pthread_mutex_lock(&lock);
    MeleeResetHandler callback=handler;
    void* context=handler_context;
    int duplicate=resetting;
    resetting=1;
    pthread_mutex_unlock(&lock);
    if(duplicate)OSPanic(__FILE__,__LINE__,"Concurrent or recursive native reset");
    if(!callback)OSPanic(__FILE__,__LINE__,"Native host reset handler is not installed");
    /* A terminating game thread must not strand cooperating workers behind
     * the interrupt gate. Runtime teardown belongs to the host handler. */
    OSEnableInterrupts();
    callback((MeleeResetRequest){kind,code,force_menu!=0},context);
    OSPanic(__FILE__,__LINE__,"Native host reset handler returned");
}
