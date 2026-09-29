// Browser-owned WebGPU device; Emdawn JS imports are proxied to the main browser
// event loop while all blocking C++ work remains on pthread workers.
#include "lib/internal.hpp"
#include "lib/webgpu/gpu.hpp"
#include "lib/gfx/frame.hpp"
#include <atomic>
#include <chrono>
#include <thread>

extern "C" void melee_browser_gpu_debug();
namespace aurora {
AuroraConfig g_config{};
uint32_t g_sdlCustomEventsStart=0;
char g_gameName[4]={'G','A','L','E'};
}
namespace aurora::webgpu {
wgpu::Device g_device;
wgpu::Queue g_queue;
wgpu::Surface g_surface;
wgpu::BackendType g_backendType=wgpu::BackendType::WebGPU;
GraphicsConfig g_graphicsConfig{};
TextureWithSampler g_frameBuffer, g_frameBufferResolved, g_depthBuffer;
wgpu::RenderPipeline g_CopyPipeline, g_CopyPremultipliedAlphaPipeline;
wgpu::BindGroup g_CopyBindGroup;
wgpu::Instance g_instance;
wgpu::AdapterInfo g_adapterInfo;
bool g_hasCoreFeatures=true;
bool g_bcTexturesSupported=false, g_astcTexturesSupported=false, g_textureComponentSwizzleSupported=false;
namespace {
void wait(std::atomic<bool>& done) {
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(20);
    while(!done.load(std::memory_order_acquire)) {
        if(std::chrono::steady_clock::now()>deadline) {
            std::fputs("Browser WebGPU initialization timed out\n",stderr);std::abort();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
}
TextureWithSampler create_render_texture(uint32_t w,uint32_t h,bool multisampled) {
    const wgpu::TextureDescriptor descriptor{
        .usage=wgpu::TextureUsage::RenderAttachment|wgpu::TextureUsage::TextureBinding|wgpu::TextureUsage::CopySrc|wgpu::TextureUsage::CopyDst,
        .size={w,h,1},.format=wgpu::TextureFormat::RGBA8Unorm,
        .sampleCount=multisampled?g_graphicsConfig.msaaSamples:1};
    auto texture=g_device.CreateTexture(&descriptor);
    return {texture,texture.CreateView(),descriptor.size,descriptor.format,g_device.CreateSampler()};
}
bool initialize(AuroraBackend,bool,uint32_t width,uint32_t height) {
    if(!width||!height)return false;
    melee_browser_gpu_debug();
    g_instance=wgpu::CreateInstance();
    wgpu::Adapter adapter;
    std::atomic<bool> ready{false};
    g_instance.RequestAdapter(nullptr,wgpu::CallbackMode::AllowSpontaneous,
        [&](wgpu::RequestAdapterStatus status,wgpu::Adapter value,wgpu::StringView message) {
            if(status!=wgpu::RequestAdapterStatus::Success)
                std::fprintf(stderr,"WebGPU adapter: %.*s\n",int(message.length),message.data);
            adapter=std::move(value);ready.store(true,std::memory_order_release);
        });
    wait(ready);if(!adapter)return false;ready=false;
    wgpu::DeviceDescriptor descriptor{};
    descriptor.SetUncapturedErrorCallback([](const wgpu::Device&,wgpu::ErrorType,wgpu::StringView message) {
        std::fprintf(stderr,"WebGPU validation: %.*s\n",int(message.length),message.data);std::abort();
    });
    adapter.RequestDevice(&descriptor,wgpu::CallbackMode::AllowSpontaneous,
        [&](wgpu::RequestDeviceStatus status,wgpu::Device value,wgpu::StringView message) {
            if(status!=wgpu::RequestDeviceStatus::Success)
                std::fprintf(stderr,"WebGPU device: %.*s\n",int(message.length),message.data);
            g_device=std::move(value);ready.store(true,std::memory_order_release);
        });
    wait(ready);if(!g_device)return false;
    g_queue=g_device.GetQueue();
    g_graphicsConfig={.surfaceConfiguration={.format=wgpu::TextureFormat::RGBA8Unorm,
      .usage=wgpu::TextureUsage::RenderAttachment,.width=width,.height=height},
      .depthFormat=wgpu::TextureFormat::Depth32Float,.msaaSamples=1,.textureAnisotropy=4,
      .synchronousPipelines=true};
    g_frameBuffer=create_render_texture(width,height,false);
    g_frameBufferResolved=g_frameBuffer;
    const wgpu::TextureDescriptor depth{.usage=wgpu::TextureUsage::RenderAttachment|wgpu::TextureUsage::TextureBinding,
      .size={width,height,1},.format=wgpu::TextureFormat::Depth32Float};
    auto texture=g_device.CreateTexture(&depth);
    g_depthBuffer={texture,texture.CreateView(),depth.size,depth.format,{}};
    return true;
}
// Native Dawn's disk cache isn't a browser GPU cache. Browser-managed shader
// caching remains enabled by the browser implementation.
void cache_prune() {}
void cache_shutdown() {}
void shutdown() {
    gfx::gpu_synchronize();g_frameBuffer={};g_frameBufferResolved={};g_depthBuffer={};
    g_queue={};g_surface={};g_device={};g_instance={};
}
}
namespace aurora::window {
AuroraWindowSize get_window_size() {
    const auto size=webgpu::g_frameBuffer.size;
    return {size.width,size.height,size.width,size.height,size.width,size.height,1.f};
}
void request_frame_buffer_resize() {
    // Canvas presentation is independent of the fixed native-resolution EFB.
}
}
