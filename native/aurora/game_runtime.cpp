#include "lib/internal.hpp"
#include "lib/webgpu/gpu.hpp"
#include "lib/webgpu/gpu_prof.hpp"
#include "lib/gfx/frame.hpp"
#include "lib/gfx/recording.hpp"
#include "lib/gx/fifo.hpp"
#include "lib/gx/texture.hpp"
#include "melee_runtime.h"
#include "melee_reset.h"
#include "melee_vi.h"
#include "xfb.hpp"
#include "metal_present.hpp"
extern "C" {
#include "melee_dvd.h"
#include "melee_card_backend.h"
#ifdef MELEE_RUNTIME_PROBES
int melee_native_probe_rumble(int setting);
int melee_native_probe_unlock(int setting);
#endif
int melee_game_main(void);
void GXWaitDrawDone(void);
int OSEnableInterrupts(void);
int OSRestoreInterrupts(int);
}
#include <array>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>

namespace {
constexpr unsigned width=640,height=480,bytes=width*height*4;
std::atomic<int> state{MELEE_RUNTIME_IDLE};
std::mutex mutex;
std::condition_variable wake;
std::string error;
std::string configured_cache;
std::array<unsigned char,bytes> pixels{};
uint64_t sequence=0, pixel_sequence=0, cpu_readbacks=0;
MeleeMetalPresent metal_present;
MeleeRuntimeTiming timing{};
uint64_t last_submit=0;
uint64_t clock_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
bool paused=false;
wgpu::Buffer readback;
int fail(const std::string& message) {
    std::lock_guard lock(mutex);error=message;
    state.store(MELEE_RUNTIME_FAILED,std::memory_order_release);
    return -1;
}
void publish_frame() {
    using namespace aurora;
    const auto presentation=melee_vi_presentation();
    auto source=melee::xfb::image(presentation.framebuffer);
    if(!source)return;
    const int direct=metal_present.draw(source->texture,presentation.black);
    if(direct) {
        if(direct==1){std::lock_guard lock(mutex);++sequence;++timing.direct_presents;}
        return;
    }
    if(presentation.black){
        std::lock_guard lock(mutex);pixels.fill(0);
        for(unsigned i=3;i<bytes;i+=4)pixels[i]=255;
        pixel_sequence=++sequence;return;
    }
    auto encoder=webgpu::g_device.CreateCommandEncoder();
    const wgpu::TexelCopyTextureInfo texture{.texture=source->texture};
    const wgpu::TexelCopyBufferInfo destination{.layout={.bytesPerRow=width*4,.rowsPerImage=height},.buffer=readback};
    const wgpu::Extent3D extent{width,height,1};
    encoder.CopyTextureToBuffer(&texture,&destination,&extent);
    auto command=encoder.Finish();webgpu::g_queue.Submit(1,&command);
    auto mapped=std::make_shared<std::atomic<bool>>(false);
    auto future=readback.MapAsync(wgpu::MapMode::Read,0,bytes,wgpu::CallbackMode::WaitAnyOnly,
        [mapped](wgpu::MapAsyncStatus status,wgpu::StringView){mapped->store(status==wgpu::MapAsyncStatus::Success);});
    if(webgpu::g_instance.WaitAny(future,10000000000)!=wgpu::WaitStatus::Success||!mapped->load()){
        fail("Unable to read the rendered game frame.");return;
    }
    {
        std::lock_guard lock(mutex);
        std::memcpy(pixels.data(),readback.GetConstMappedRange(),bytes);
        // The VI/XFB is an opaque display image. GX's render-target alpha is
        // unrelated to window compositing and can be zero across the frame.
        for(unsigned i=3;i<bytes;i+=4)pixels[i]=255;
        pixel_sequence=++sequence;
        ++timing.cpu_readbacks;++cpu_readbacks;
    }
    readback.Unmap();
}
void submit_frame() {
    const auto started=clock_ns();
    using namespace aurora;
    gx::fifo::drain();gx::fifo::end_frame();gx::texture::end_frame();gfx::finish();
    gfx::end_frame([](wgpu::CommandEncoder& encoder,std::vector<gfx::AfterSubmitCallback> callbacks){
        webgpu::gpu_prof::frame_end(encoder);
        auto commands=encoder.Finish();webgpu::g_queue.Submit(1,&commands);
        webgpu::gpu_prof::after_submit();for(auto& callback:callbacks)callback();gfx::after_submit();
    });
    const auto submitted=clock_ns();
    GXWaitDrawDone();
    const auto completed=clock_ns();
    // HSD submits draw-done with interrupts disabled. GPU readback and a UI
    // pause must not hold that gate against the audio/VI workers.
    const int interrupts=OSEnableInterrupts();
    publish_frame();
    const auto published=clock_ns();
    {
        std::lock_guard lock(mutex);
        ++timing.frames;
        if(last_submit) {
            const auto elapsed=started-last_submit;
            timing.interval_ns+=elapsed;
            timing.max_interval_ns=std::max(timing.max_interval_ns,elapsed);
            timing.over20ms+=elapsed>20000000;
            timing.over33ms+=elapsed>33366666;
        }
        last_submit=started;
        const auto submit=submitted-started,wait=completed-submitted,copy=published-completed;
        timing.submit_ns+=submit;timing.max_submit_ns=std::max(timing.max_submit_ns,submit);
        timing.wait_ns+=wait;timing.max_wait_ns=std::max(timing.max_wait_ns,wait);
        timing.readback_ns+=copy;timing.max_readback_ns=std::max(timing.max_readback_ns,copy);
    }
    {
        std::unique_lock lock(mutex);
        wake.wait(lock,[]{return !paused&&state.load()!=MELEE_RUNTIME_FAILED;});
    }
    if(!gfx::begin_frame()){
        fail("Unable to begin the next game frame.");
        std::unique_lock lock(mutex);wake.wait(lock,[]{return false;});
    }
    gx::fifo::begin_frame();
    OSRestoreInterrupts(interrupts);
}
void reset(MeleeResetRequest,void*) {
    OSEnableInterrupts();
    fail("The game requested a restart. Close and reopen the app to start again.");
    std::unique_lock lock(mutex);wake.wait(lock,[]{return false;});
}
}
extern "C" void melee_runtime_set_display_clock(int enabled){melee_vi_set_display_clock(enabled!=0);}
extern "C" void melee_runtime_display_tick(void){melee_vi_display_tick();}
extern "C" void melee_runtime_take_timing(MeleeRuntimeTiming* output){
    if(!output)return;
    std::lock_guard lock(mutex);*output=timing;timing={};
}
extern "C" void melee_runtime_set_metal_layer(void* layer,uint32_t w,uint32_t h){metal_present.set(layer,w,h);}
extern "C" int melee_runtime_metal_state(void){return metal_present.state();}
extern "C" uint64_t melee_runtime_frame_sequence(void){std::lock_guard lock(mutex);return sequence;}
extern "C" uint64_t melee_runtime_cpu_readbacks(void){std::lock_guard lock(mutex);return cpu_readbacks;}
extern "C" int melee_runtime_state(void){return state.load(std::memory_order_acquire);}
extern "C" void melee_runtime_error(char* output,size_t capacity){
    if(!output||!capacity)return;
    std::lock_guard lock(mutex);size_t n=std::min(capacity-1,error.size());
    std::memcpy(output,error.data(),n);output[n]=0;
}
extern "C" uint64_t melee_runtime_copy_frame(void* output,size_t capacity,uint64_t after){
    if(!output||capacity<bytes)return 0;
    std::lock_guard lock(mutex);if(pixel_sequence<=after)return 0;
    std::memcpy(output,pixels.data(),bytes);return pixel_sequence;
}
extern "C" void melee_runtime_set_paused(int value){
    {std::lock_guard lock(mutex);paused=value!=0;}wake.notify_all();
}
extern "C" int melee_runtime_run(const char* disc,const char* cache_path){
    return melee_runtime_run_with_save(disc,cache_path,nullptr);
}
extern "C" int melee_runtime_run_with_save(const char* disc,const char* cache_path,const char* card_path){
    int expected=MELEE_RUNTIME_IDLE;
    if(!state.compare_exchange_strong(expected,MELEE_RUNTIME_STARTING))return -1;
    if(!disc||!cache_path)return fail("Choose a game disc image and a cache directory.");
    try {
        if(!melee_dvd_mount(disc))return fail("The game disc image could not be opened.");
        if(card_path && *card_path){
            const int result=melee_card_insert(0,card_path,true);
            if(result!=CARD_RESULT_READY)return fail("The save file could not be opened (memory card error "+std::to_string(result)+"). Your existing save has not been replaced.");
        }
        configured_cache=std::filesystem::absolute(cache_path).string();
        std::filesystem::create_directories(configured_cache);
        melee_reset_configure(0,reset,nullptr);
        using namespace aurora;
        g_config={};g_config.appName="Melee Native";
        g_config.cachePath=configured_cache.c_str();g_config.userPath=configured_cache.c_str();g_config.resourcesPath=configured_cache.c_str();
        g_config.msaa=1;g_config.maxTextureAnisotropy=4;
        if(!webgpu::initialize(BACKEND_METAL,false,width,height))return fail("Metal initialization failed.");
        gfx::initialize();gx::fifo::init();
        wgpu::BufferDescriptor descriptor{.label="Melee app frame readback",
            .usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=bytes};
        readback=webgpu::g_device.CreateBuffer(&descriptor);
        if(!gfx::begin_frame())return fail("Unable to begin the first game frame.");
        gx::fifo::begin_frame();gx::fifo::set_draw_done_submitter(submit_frame);
        state.store(MELEE_RUNTIME_RUNNING,std::memory_order_release);
        int result=melee_game_main();
        OSEnableInterrupts();
        return fail("The game stopped with code "+std::to_string(result)+".");
    }catch(const std::exception& exception){OSEnableInterrupts();return fail(exception.what());}
}

#ifdef MELEE_RUNTIME_PROBES
// Controlled save roundtrip fixture; only touch game state while presentation
// is paused after initialization. Not exposed through the app's public bridge.
extern "C" int melee_runtime_test_rumble(int setting){
    std::lock_guard lock(mutex);
    if(!paused||sequence<60)return -1;
    return melee_native_probe_rumble(setting);
}
extern "C" int melee_runtime_test_unlock(int setting){
    std::lock_guard lock(mutex);
    if(!paused||sequence<60)return -1;
    return melee_native_probe_unlock(setting);
}
#endif
