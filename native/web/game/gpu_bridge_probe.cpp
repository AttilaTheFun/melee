#include <webgpu/webgpu_cpp.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <array>
#include <emscripten.h>

static void require(bool okay, const char* message) {
    if (!okay) { std::fprintf(stderr,"FAIL %s\n",message); std::abort(); }
}
static void wait(std::atomic<bool>& done) {
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);
    while(!done.load(std::memory_order_acquire)) {
        require(std::chrono::steady_clock::now()<deadline,"GPU callback timed out");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
int main() {
    auto instance=wgpu::CreateInstance();
    wgpu::Adapter adapter;
    std::atomic<bool> done{false};
    instance.RequestAdapter(nullptr,wgpu::CallbackMode::AllowSpontaneous,
      [&](wgpu::RequestAdapterStatus status,wgpu::Adapter value,wgpu::StringView message) {
        if(status!=wgpu::RequestAdapterStatus::Success)std::fprintf(stderr,"adapter: %.*s\n",int(message.length),message.data);
        adapter=std::move(value);done.store(true,std::memory_order_release);
      });
    wait(done);require(bool(adapter),"WebGPU adapter");done=false;
    wgpu::Device device;
    wgpu::DeviceDescriptor descriptor{};
    descriptor.SetUncapturedErrorCallback([](const wgpu::Device&,wgpu::ErrorType,wgpu::StringView message) {
        std::fprintf(stderr,"GPU VALIDATION: %.*s\n",int(message.length),message.data);std::abort();
    });
    adapter.RequestDevice(&descriptor,wgpu::CallbackMode::AllowSpontaneous,
      [&](wgpu::RequestDeviceStatus,wgpu::Device value,wgpu::StringView) {
        device=std::move(value);done.store(true,std::memory_order_release);
      });
    wait(done);require(bool(device),"WebGPU device");
    // Recycle same-sized handles on independent workers. The JS registry must
    // be cleared before malloc can hand an address to another GPU object.
    std::array<std::thread,3> recyclers;
    for(auto& worker:recyclers)worker=std::thread([&]{
        for(unsigned i=0;i<2000;i++){
            wgpu::BufferDescriptor description{.usage=wgpu::BufferUsage::CopyDst,.size=4};
            auto buffer=device.CreateBuffer(&description);
            buffer.SetLabel("Concurrent handle reuse");buffer.Destroy();
        }
    });
    for(auto& worker:recyclers)worker.join();
    std::puts("PASS browser WebGPU: 6000 concurrent handle lifetimes");
    // Use a second C++ thread to prove GPU handles share one JS registry.
    std::thread render([&] {
        auto queue=device.GetQueue();
        wgpu::TextureDescriptor td{.usage=wgpu::TextureUsage::RenderAttachment|wgpu::TextureUsage::CopySrc,
          .size={4,4,1},.format=wgpu::TextureFormat::RGBA8Unorm};
        auto texture=device.CreateTexture(&td);
        auto view=texture.CreateView();
        wgpu::BufferDescriptor bd{.usage=wgpu::BufferUsage::MapRead|wgpu::BufferUsage::CopyDst,.size=1024};
        auto buffer=device.CreateBuffer(&bd);
        auto encoder=device.CreateCommandEncoder();
        wgpu::RenderPassColorAttachment color{.view=view,.loadOp=wgpu::LoadOp::Clear,
          .storeOp=wgpu::StoreOp::Store,.clearValue={1,0.25,0,1}};
        wgpu::RenderPassDescriptor pd{.colorAttachmentCount=1,.colorAttachments=&color};
        wgpu::ShaderSourceWGSL source{};
        source.code=R"(
          @vertex fn vs(@builtin(vertex_index) i:u32)->@builtin(position) vec4f {
            let p=array<vec2f,3>(vec2f(-1,-1),vec2f(3,-1),vec2f(-1,3));
            return vec4f(p[i],0,1);
          }
          @fragment fn fs()->@location(0) vec4f { return vec4f(0,1,0,1); }
        )";
        wgpu::ShaderModuleDescriptor shaderDescriptor{.nextInChain=&source};
        auto shader=device.CreateShaderModule(&shaderDescriptor);
        wgpu::ColorTargetState target{.format=wgpu::TextureFormat::RGBA8Unorm};
        wgpu::FragmentState fragment{.module=shader,.entryPoint="fs",.targetCount=1,.targets=&target};
        wgpu::RenderPipelineDescriptor pipelineDescriptor{};
        pipelineDescriptor.vertex.module=shader;pipelineDescriptor.vertex.entryPoint="vs";
        pipelineDescriptor.fragment=&fragment;
        auto pipeline=device.CreateRenderPipeline(&pipelineDescriptor);
        wgpu::BufferDescriptor indexDescriptor{.usage=wgpu::BufferUsage::Index|wgpu::BufferUsage::CopyDst,.size=8};
        auto indices=device.CreateBuffer(&indexDescriptor);
        const uint16_t indexData[4]={0,1,2,0};queue.WriteBuffer(indices,0,indexData,sizeof(indexData));
        auto pass=encoder.BeginRenderPass(&pd);pass.SetPipeline(pipeline);
        pass.SetIndexBuffer(indices,wgpu::IndexFormat::Uint16,0,8);
        // A burst of asynchronous scalar commands must finish in FIFO order
        // before End() returns and the pass's C++ owner is released.
        for(unsigned i=0;i<1000;i++){
            pass.SetViewport(0,0,4,4,0,1);pass.SetScissorRect(0,0,2,4);pass.SetPipeline(pipeline);pass.DrawIndexed(3);
        }
        // Drop these owners before End: synchronous JS registry deletion must
        // drain queued uses first, while WebGPU retains the encoded resources.
        pipeline=nullptr;indices=nullptr;
        pass.End();pass=nullptr;
        wgpu::TexelCopyTextureInfo src{.texture=texture};
        wgpu::TexelCopyBufferInfo dst{.layout={.bytesPerRow=256,.rowsPerImage=4},.buffer=buffer};
        wgpu::Extent3D size{4,4,1};encoder.CopyTextureToBuffer(&src,&dst,&size);
        auto command=encoder.Finish();queue.Submit(1,&command);
        done=false;
        buffer.MapAsync(wgpu::MapMode::Read,0,1024,wgpu::CallbackMode::AllowSpontaneous,
          [&](wgpu::MapAsyncStatus status,wgpu::StringView) {
            require(status==wgpu::MapAsyncStatus::Success,"readback map");done.store(true,std::memory_order_release);
          });
        wait(done);
        auto* pixels=static_cast<const unsigned char*>(buffer.GetConstMappedRange());
        require(pixels&&pixels[0]==0&&pixels[1]==255&&pixels[2]==0&&pixels[3]==255&&pixels[8]==255&&pixels[9]==64&&pixels[10]==0&&pixels[11]==255,"rendered pixels");
        buffer.Unmap();
    });
    render.join();
    std::puts("PASS browser WebGPU: shared worker handles, asynchronous completion, ordered scalar draw bursts, exact scissored pixels");
    return 0;
}
