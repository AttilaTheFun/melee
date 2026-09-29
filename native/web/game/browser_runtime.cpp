#include "lib/internal.hpp"
#include "lib/webgpu/gpu.hpp"
#include "lib/gfx/frame.hpp"
#include "lib/gfx/recording.hpp"
#include "lib/gx/fifo.hpp"
#include "lib/gx/texture.hpp"
#include "melee_vi.h"
#include "melee_audio_producer.h"
#include "xfb.hpp"
#include <filesystem>
#include <atomic>
#include <emscripten/threading.h>
#include <emscripten.h>
#include <cstdio>
#include <cstdlib>
extern "C" {
int melee_game_main(void);
void GXWaitDrawDone(void);
int OSEnableInterrupts(void);
int OSRestoreInterrupts(int);
bool melee_browser_mount_disc();
int melee_browser_open_card(void);
void melee_browser_update_input(void);
void melee_browser_net_init(void);
void melee_browser_net_phase(unsigned);
int melee_browser_net_enabled(void);
void melee_browser_resume_input(void);
void melee_browser_lifecycle_attach(void*);
void melee_browser_lifecycle_changed(int);
void melee_browser_publish_state(unsigned);
void melee_audio_ring_browser_layout(MeleeAudioRing*,uint32_t*);
void melee_browser_audio_attach(const uint32_t*);
void melee_browser_present(WGPUDevice, WGPUTexture, int);
}
static void require(bool value,const char* message){if(!value){std::fprintf(stderr,"Browser runtime: %s\n",message);std::abort();}}
static std::atomic<unsigned> pause_requested{0};
static std::atomic<double> queue_submitted_at{0};
static unsigned frames=0;
extern "C" unsigned melee_browser_render_frame_count(void){return frames;}
static void submit_frame(){
    using namespace aurora;
    melee_browser_net_phase(20);
    const double started=emscripten_get_now();
    static double last_end=0, logic_ms=0, gpu_ms=0, draw_wait_ms=0, present_ms=0, render_wait_ms=0, completion_ms=0;
    if(last_end)logic_ms+=started-last_end;
    gx::fifo::drain();gx::fifo::end_frame();gx::texture::end_frame();gfx::finish();
    gfx::end_frame([](wgpu::CommandEncoder& encoder,std::vector<gfx::AfterSubmitCallback> callbacks){
        auto commands=encoder.Finish();webgpu::g_queue.Submit(1,&commands);
        queue_submitted_at.store(emscripten_get_now(),std::memory_order_release);
        for(auto& callback:callbacks)callback();gfx::after_submit();
    });
    const double submitted=emscripten_get_now();
    melee_browser_net_phase(21);
    GXWaitDrawDone();
    melee_browser_net_phase(22);
    const double drawn=emscripten_get_now();
    const double queued=queue_submitted_at.load(std::memory_order_acquire);
    render_wait_ms+=queued>submitted?queued-submitted:0;
    completion_ms+=drawn-(queued>submitted?queued:submitted);
    int interrupts=OSEnableInterrupts();
    static MeleeAudioRing* audio=nullptr;
    if(!audio){
        audio=melee_audio_ring_create();require(audio!=nullptr,"audio ring allocation");
        require(melee_audio_producer_start(audio)!=nullptr,"audio producer start");
        uint32_t layout[7];melee_audio_ring_browser_layout(audio,layout);melee_browser_audio_attach(layout);
    }
    if(!melee_browser_net_enabled())melee_browser_update_input();
    auto presentation=melee_vi_presentation();
    auto image=melee::xfb::image(presentation.framebuffer);
    if(image)melee_browser_present(webgpu::g_device.Get(),image->texture.Get(),presentation.black);
    const double presented=emscripten_get_now();
    gpu_ms+=submitted-started;draw_wait_ms+=drawn-submitted;present_ms+=presented-drawn;
    ++frames;
    if(!melee_browser_net_enabled())melee_browser_publish_state(frames);
    if(frames==1)melee_browser_lifecycle_changed(0);
    if(frames%60==0){
        MAIN_THREAD_EM_ASM({
            Module.runtimeProfile=({frame:$0,logicMs:$1/60,gpuSubmitMs:$2/60,drawWaitMs:$3/60,presentMs:$4/60,renderQueueMs:$5/60,completionMs:$6/60});
        },frames,logic_ms,gpu_ms,draw_wait_ms,present_ms,render_wait_ms,completion_ms);
        logic_ms=gpu_ms=draw_wait_ms=present_ms=render_wait_ms=completion_ms=0;
        std::printf("Browser game frame %u\n",frames);
    }
    if(pause_requested.load()){
        melee_vi_set_display_clock(true);
        melee_browser_lifecycle_changed(1);
        while(pause_requested.load())emscripten_futex_wait(&pause_requested,1,1000);
        melee_browser_resume_input();
        melee_vi_set_display_clock(false);
        melee_browser_lifecycle_changed(0);
    }
    require(gfx::begin_frame(),"begin frame");gx::fifo::begin_frame();
    OSRestoreInterrupts(interrupts);
    last_end=emscripten_get_now();
}
int main(){
    using namespace aurora;
    melee_browser_net_init();
    melee_browser_lifecycle_attach(&pause_requested);
    require(melee_browser_mount_disc(),"mount disc");
    require(melee_browser_open_card()==0,"open memory card (existing save preserved on validation failure)");
    std::filesystem::create_directories("/cache");
    g_config={};g_config.appName="Melee Browser";g_config.cachePath="/cache";
    g_config.userPath="/cache";g_config.resourcesPath="/cache";g_config.msaa=1;
    require(webgpu::initialize(BACKEND_AUTO,false,640,480),"WebGPU initialize");
    gfx::initialize();gx::fifo::init();
    require(gfx::begin_frame(),"first frame");gx::fifo::begin_frame();
    gx::fifo::set_draw_done_submitter(submit_frame);
    return melee_game_main();
}
