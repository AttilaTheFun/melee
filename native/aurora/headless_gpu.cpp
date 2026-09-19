// Offscreen integration harness. Does not initialize SDL video, create a window,
// acquire a surface, or present. Uses Aurora's real GX and Metal/Dawn pipeline.
#include "gpu_completion.hpp"
#include "xfb.hpp"
extern "C" void* melee_hsd_video_gpu_copy();
extern "C" void melee_hsd_video_gpu_before_submit();
extern "C" void melee_hsd_video_gpu_wait();
extern "C" void melee_hsd_video_gpu_shutdown();
#include "lib/internal.hpp"
#include "lib/webgpu/gpu.hpp"
#include "lib/webgpu/gpu_prof.hpp"
#include "lib/gfx/frame.hpp"
#include "lib/gfx/recording.hpp"
#include "lib/gx/fifo.hpp"
#include "lib/gx/texture.hpp"
#include "lib/gx/gx.hpp"
#include <filesystem>
#include <atomic>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <dolphin/gx.h>
extern "C" void GXWaitDrawDone();
extern "C" int OSDisableInterrupts();
extern "C" int OSRestoreInterrupts(int);
#include <png.h>
#include <aurora/gfx.h>
#include <cstdio>
namespace {
constexpr uint32_t Width=640,Height=480;
std::string cache;
std::mutex doneMutex;
std::condition_variable doneWake;
unsigned doneCalls;
GXDrawDoneCallback previousDone;
void drawDoneProbe() {
    std::lock_guard lock{doneMutex};
    ++doneCalls;
    doneWake.notify_all();
}
bool markDrawDone() {
    { std::lock_guard lock{doneMutex}; doneCalls=0; }
    previousDone=GXSetDrawDoneCallback(drawDoneProbe);
    GXSetDrawDone();
    aurora::gx::fifo::drain();
    aurora::gfx::gpu_synchronize();
    std::lock_guard lock{doneMutex};
    if(doneCalls){std::fputs("GX callback ran before frame submission\n",stderr);return false;}
    return true;
}
bool waitDrawDone() {
    std::unique_lock lock{doneMutex};
    if(!doneWake.wait_for(lock,std::chrono::seconds(10),[]{return doneCalls!=0;})||doneCalls!=1){
        std::fputs("GX draw-done callback missing or duplicated\n",stderr);return false;
    }
    GXSetDrawDoneCallback(previousDone);
    std::fprintf(stderr,"GX callback delivered once after actual GPU completion\n");
    return true;
}
}
extern "C" bool melee_gpu_begin(const char* cachePath){
    using namespace aurora;
    cache=std::filesystem::absolute(cachePath).string();std::filesystem::create_directories(cache);
    g_config={};g_config.appName="Melee offscreen GX";g_config.cachePath=cache.c_str();
    g_config.userPath=cache.c_str();g_config.resourcesPath=cache.c_str();
    g_config.msaa=1;g_config.maxTextureAnisotropy=4;
    if(!webgpu::initialize(BACKEND_METAL,false,Width,Height))return false;
    // Initialize the offscreen EFB explicitly; no swapchain clear/present runs.
    const wgpu::RenderPassColorAttachment color{.view=webgpu::g_frameBuffer.view,
        .loadOp=wgpu::LoadOp::Clear,.storeOp=wgpu::StoreOp::Store,.clearValue={13.0/255,18.0/255,26.0/255,1}};
    const wgpu::RenderPassDepthStencilAttachment depth{.view=webgpu::g_depthBuffer.view,
        .depthLoadOp=wgpu::LoadOp::Clear,.depthStoreOp=wgpu::StoreOp::Store,.depthClearValue=0};
    const wgpu::RenderPassDescriptor pass{.colorAttachmentCount=1,.colorAttachments=&color,.depthStencilAttachment=&depth};
    const auto encoder=webgpu::g_device.CreateCommandEncoder();encoder.BeginRenderPass(&pass).End();
    const auto commands=encoder.Finish();webgpu::g_queue.Submit(1,&commands);
    gfx::initialize();gx::fifo::init();
    gx::g_gxState.clearColor={13.f/255,18.f/255,26.f/255,1.f};
    if(!gfx::begin_frame())return false;
    gx::fifo::begin_frame();return true;
}
namespace {
void submitMarkedFrame() {
    using namespace aurora;
    gx::fifo::drain();gx::fifo::end_frame();gx::texture::end_frame();gfx::finish();
    gfx::end_frame([](wgpu::CommandEncoder& encoder,std::vector<gfx::AfterSubmitCallback> callbacks){
        webgpu::gpu_prof::frame_end(encoder);
        const auto commands=encoder.Finish();webgpu::g_queue.Submit(1,&commands);
        webgpu::gpu_prof::after_submit();for(auto& callback:callbacks)callback();gfx::after_submit();
    });
}
}
extern "C" bool melee_gpu_next_frame(){
    using namespace aurora;
    { std::lock_guard lock{doneMutex}; doneCalls=0; }
    previousDone=GXSetDrawDoneCallback(drawDoneProbe);
    auto previousSubmitter=gx::fifo::set_draw_done_submitter(submitMarkedFrame);
    // Exercise the actual SDK blocking entry point while the game gate is
    // held. Its wait must yield the gate so the GPU callback can finish.
    const int enabled=OSDisableInterrupts();
    GXDrawDone();
    if(OSDisableInterrupts()!=0){std::fputs("GX wait lost interrupt state\n",stderr);return false;}
    GXWaitDrawDone(); // An already-completed marker must not add another frame.
    OSRestoreInterrupts(enabled);
    gx::fifo::set_draw_done_submitter(previousSubmitter);
    if(!waitDrawDone())return false;
    const auto* stats=aurora_get_stats();
    std::fprintf(stderr,"SDK GXDrawDone returned after frame %u GPU completion: %u draws\n",gfx::current_frame(),stats->drawCallCount);
    if(!stats->drawCallCount||!gfx::begin_frame())return false;
    gx::fifo::begin_frame();return true;
}
extern "C" bool melee_gpu_finish(const char* output){
    using namespace aurora;
    void* framebufferIdentity=melee_hsd_video_gpu_copy();
    auto copied=melee::xfb::image(framebufferIdentity);
    static char clearIdentity;
    GXCopyDisp(&clearIdentity,GX_FALSE);
    auto cleared=melee::xfb::image(&clearIdentity);
    if(!copied||!cleared||copied==cleared)return false;
    gx::fifo::drain();gfx::gpu_synchronize();
    melee_hsd_video_gpu_before_submit();
    gx::fifo::drain();gx::fifo::end_frame();gx::texture::end_frame();gfx::finish();
    wgpu::BufferDescriptor descriptor{.label="Melee screenshot",.usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=Width*Height*8};
    wgpu::Buffer buffer=webgpu::g_device.CreateBuffer(&descriptor);
    gfx::end_frame([buffer,copied,cleared](wgpu::CommandEncoder& encoder,std::vector<gfx::AfterSubmitCallback> callbacks){
        const wgpu::TexelCopyTextureInfo source{.texture=copied->texture};
        const wgpu::TexelCopyBufferInfo destination{.layout={.bytesPerRow=Width*4,.rowsPerImage=Height},.buffer=buffer};
        const wgpu::Extent3D size{Width,Height,1};encoder.CopyTextureToBuffer(&source,&destination,&size);
        const wgpu::TexelCopyTextureInfo clearedSource{.texture=cleared->texture};
        const wgpu::TexelCopyBufferInfo clearedDestination{.layout={.offset=Width*Height*4,.bytesPerRow=Width*4,.rowsPerImage=Height},.buffer=buffer};
        encoder.CopyTextureToBuffer(&clearedSource,&clearedDestination,&size);
        webgpu::gpu_prof::frame_end(encoder);const auto commands=encoder.Finish();webgpu::g_queue.Submit(1,&commands);
        webgpu::gpu_prof::after_submit();for(auto& callback:callbacks)callback();gfx::after_submit();
    });
    if(!melee_gpu_wait_submitted(10000000000ULL)){
        gx::fifo::shutdown();gfx::shutdown();webgpu::shutdown();return false;
    }
    std::fprintf(stderr,"Metal queue completion confirmed before readback\n");
    melee_hsd_video_gpu_wait();
    if(melee::xfb::image(framebufferIdentity)!=copied)return false;
    melee_hsd_video_gpu_shutdown();
    melee::xfb::release(framebufferIdentity);melee::xfb::release(&clearIdentity);
    if(melee::xfb::image(framebufferIdentity))return false;
    const auto* stats=aurora_get_stats();
    std::fprintf(stderr,"GX stats: draws=%u created=%u queued=%u vertices=%u storage=%u\n",stats->drawCallCount,stats->createdPipelines,stats->queuedPipelines,stats->lastVertSize,stats->lastStorageSize);
    auto mapped=std::make_shared<std::atomic<bool>>(false);
    auto future=buffer.MapAsync(wgpu::MapMode::Read,0,Width*Height*8,wgpu::CallbackMode::WaitAnyOnly,
        [mapped](wgpu::MapAsyncStatus status,wgpu::StringView message){
            mapped->store(status==wgpu::MapAsyncStatus::Success,std::memory_order_release);
            if(status!=wgpu::MapAsyncStatus::Success)std::fprintf(stderr,"Readback failed: %.*s\n",int(message.length),message.data);
        });
    bool okay=webgpu::g_instance.WaitAny(future,10000000000)==wgpu::WaitStatus::Success&&mapped->load(std::memory_order_acquire);
    if(okay){
        const auto* pixels=static_cast<const uint8_t*>(buffer.GetConstMappedRange());
        const auto* clearPixels=pixels+Width*Height*4;
        size_t uncleared=0;for(size_t i=0;i<Width*Height;i++)
            uncleared+=clearPixels[i*4]!=13||clearPixels[i*4+1]!=18||clearPixels[i*4+2]!=26;
        if(uncleared){std::fprintf(stderr,"XFB clear snapshot has %zu unexpected pixels\n",uncleared);buffer.Unmap();return false;}
        std::fprintf(stderr,"HSD framebuffer image survived EFB clear, retrace handoff and map release\n");
        size_t changed=0;for(size_t i=1;i<Width*Height;i++)if(std::memcmp(pixels,pixels+i*4,4))changed++;
        png_image image{};image.version=PNG_IMAGE_VERSION;image.width=Width;image.height=Height;image.format=PNG_FORMAT_RGBA;
        okay=png_image_write_to_file(&image,output,0,pixels,Width*4,nullptr)!=0;
        std::printf("Metal GX readback: %zu non-background pixels; %s\n",changed,output);
        png_image_free(&image);buffer.Unmap();okay=okay&&changed>100;
    }
    gx::fifo::shutdown();gfx::shutdown();webgpu::shutdown();return okay;
}
