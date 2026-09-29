/* Online mode replaces the wall-clock raw-input queue at each logic tick.
 * Local play keeps the original alarm-driven queue unchanged. */
#include <emscripten.h>
#include <emscripten/threading.h>
#include <dolphin/os.h>
#include <sysdolphin/baselib/controller.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
extern void melee_browser_publish_input(const float*);
extern void melee_browser_net_exchange(float*,_Atomic int*);
extern void melee_browser_publish_state(unsigned);
extern unsigned melee_browser_render_frame_count(void);
static int enabled;
static uint32_t session_seed;
static double next_tick;
static unsigned net_ticks;
static _Atomic unsigned debug_phase;
static _Atomic unsigned alarm_phase, alarm_calls;
static _Atomic unsigned phase_interrupts;
extern int melee_native_interrupts_enabled(void);
void melee_browser_net_phase(unsigned phase){
    atomic_store(&phase_interrupts,melee_native_interrupts_enabled());
    atomic_store(&debug_phase,phase);
}
void melee_browser_input_alarm_phase(unsigned phase){
    atomic_store(&alarm_phase,phase);
    if(phase==80)atomic_fetch_add(&alarm_calls,1);
}
/* Original draw callbacks update gameplay state (including offscreen damage).
 * Peer-dependent skipped draws therefore cannot be a production pacing policy.
 * Keep the override only for explicit regression/diagnostic comparisons. */
static unsigned catchup_limit=1;
/* Both peers use the same cadence, independent of lateness. Two ticks per draw
 * retain a 60 Hz simulation target on the current roughly 30 Hz renderer.
 * The shared build identity covers this policy; overrides are test-only. */
static unsigned fixed_draw_ticks=2;
static const double tick_period=1001.0/60.0;
unsigned melee_browser_net_tick_count(void){return net_ticks;}
void melee_browser_net_init(void) {
    MAIN_THREAD_EM_ASM({
        const address=$0;
        Module.netNativePhase=()=>Atomics.load(HEAPU32,address>>2);
        const alarm=$1; const calls=$2; const interrupts=$3;
        Module.inputAlarmDiagnostics=()=>({phase:Atomics.load(HEAPU32,alarm>>2),calls:Atomics.load(HEAPU32,calls>>2),gameInterruptsEnabled:!!Atomics.load(HEAPU32,interrupts>>2)});
    },&debug_phase,&alarm_phase,&alarm_calls,&phase_interrupts);
    enabled=MAIN_THREAD_EM_ASM_INT({return Module.netSession?1:0;});
    if(enabled)catchup_limit=(unsigned)MAIN_THREAD_EM_ASM_INT({return Module.netCatchupLimit||1;});
    if(catchup_limit<1||catchup_limit>4)catchup_limit=1;
    if(enabled)fixed_draw_ticks=(unsigned)MAIN_THREAD_EM_ASM_INT({return Module.netDrawTicks||2;});
    if(fixed_draw_ticks<1||fixed_draw_ticks>2)fixed_draw_ticks=2;
    if(enabled)session_seed=(uint32_t)MAIN_THREAD_EM_ASM_INT({return Module.netSession.seed>>>0;});
}
int melee_browser_net_enabled(void){return enabled;}
uint32_t melee_browser_net_seed(uint32_t fallback){return enabled?session_seed:fallback;}
unsigned melee_browser_net_pending_ticks(void) {
    if(catchup_limit==1)return fixed_draw_ticks;
    if(!next_tick)return 1;
    double late=emscripten_get_now()-next_tick;
    if(late<tick_period)return 1;
    unsigned count=1+(unsigned)(late/tick_period);
    return count>catchup_limit?catchup_limit:count;
}
void melee_browser_net_after_tick(void) {
    melee_browser_net_phase(6);
    if(enabled)melee_browser_publish_state(melee_browser_render_frame_count());
}
void melee_browser_net_tick(void) {
    float samples[16];_Atomic int status=0;
    melee_browser_net_phase(3);
    const double input_started=emscripten_get_now();
    // No game input/physics proceeds until both peers commit this tick.
    melee_browser_net_exchange(samples,&status);
    while(atomic_load(&status)==0)emscripten_futex_wait(&status,0,1000);
    if(atomic_load(&status)!=1){fprintf(stderr,"Online input session stopped\n");abort();}
    melee_browser_net_phase(4);
    double now=emscripten_get_now();
    // Keep a stable simulation deadline across ordinary rendering delays.
    // Rebase after loading or a blocked peer: missing remote inputs cannot
    // be caught up locally, and must not leave a permanent render backlog.
    if(!next_tick||now-next_tick>250.0||now-input_started>tick_period)next_tick=now;
    if(next_tick>now)emscripten_futex_wait(&status,1,next_tick-now);
    next_tick+=tick_period;
    int interrupts=OSDisableInterrupts();
    melee_browser_publish_input(samples);
    HSD_PadFlushQueue(HSD_PAD_FLUSH_QUEUE_THROWAWAY);
    HSD_PadRenewRawStatus(false);
    HSD_PadRenewMasterStatus();
    ++net_ticks;
    OSRestoreInterrupts(interrupts);
    melee_browser_net_phase(5);
}
