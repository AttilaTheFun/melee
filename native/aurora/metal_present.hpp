#pragma once
#include "lib/webgpu/gpu.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <atomic>
#include <memory>
#include <mutex>

// The UI publishes retained layer/size snapshots. Only the game thread owns
// Dawn surfaces and command encoding. No CPU pixel mapping is involved.
class MeleeMetalPresent {
    struct Target {
        CFTypeRef layer;
        uint32_t width, height;
        Target(void* p,uint32_t w,uint32_t h):layer(CFRetain(p)),width(w),height(h){}
        ~Target(){CFRelease(layer);}
    };
    std::mutex mutex;
    std::shared_ptr<Target> pending, current;
    wgpu::Surface surface;
    wgpu::RenderPipeline pipeline;
    unsigned failures=0;
    std::atomic<int> status{0};
public:
    void set(void* layer,uint32_t w,uint32_t h) {
        std::lock_guard lock(mutex);
        if(pending && pending->layer==layer && pending->width==w && pending->height==h)return;
        pending=layer&&w&&h?std::make_shared<Target>(layer,w,h):nullptr;
        status.store(0);
    }
    int state()const{return status.load();}
    // 0 = CPU fallback, 1 = presented, 2 = transient drawable unavailability.
    int draw(const wgpu::Texture& source,bool black) {
        using namespace aurora::webgpu;
        std::shared_ptr<Target> target;
        {std::lock_guard lock(mutex);target=pending;}
        if(target!=current) {
            if(surface)surface.Unconfigure();
            surface=nullptr;current=target;failures=0;
            if(current) {
                wgpu::SurfaceSourceMetalLayer metal;
                metal.layer=const_cast<void*>(current->layer);
                const wgpu::SurfaceDescriptor descriptor{.nextInChain=&metal,.label="Melee direct Metal display"};
                surface=g_instance.CreateSurface(&descriptor);
                configure();
            }
        }
        if(!surface || status.load()<0)return 0;
        if(!pipeline) {
            wgpu::ShaderSourceWGSL code;
            code.code=R"(
struct Vertex { @builtin(position) position: vec4f, @location(0) uv: vec2f };
@vertex fn vs(@builtin(vertex_index) i:u32)->Vertex {
    var p=array<vec2f,3>(vec2f(-1,-1),vec2f(3,-1),vec2f(-1,3));
    var v:Vertex; v.position=vec4f(p[i],0,1);
    v.uv=vec2f((p[i].x+1)*0.5,(1-p[i].y)*0.5); return v;
}
@group(0) @binding(0) var image:texture_2d<f32>;
@fragment fn fs(v:Vertex)->@location(0) vec4f {
    let size=textureDimensions(image);
    let pixel=vec2i(clamp(v.uv,vec2f(0),vec2f(0.999999))*vec2f(size));
    return vec4f(textureLoad(image,pixel,0).rgb,1);
})";
            const wgpu::ShaderModuleDescriptor moduleDescriptor{.nextInChain=&code};
            auto module=g_device.CreateShaderModule(&moduleDescriptor);
            const wgpu::ColorTargetState color{.format=wgpu::TextureFormat::BGRA8Unorm};
            const wgpu::FragmentState fragment{.module=module,.entryPoint="fs",.targetCount=1,.targets=&color};
            const wgpu::RenderPipelineDescriptor descriptor{
                .label="Melee opaque nearest-neighbor presentation",
                .vertex={.module=module,.entryPoint="vs"},.fragment=&fragment};
            pipeline=g_device.CreateRenderPipeline(&descriptor);
        }
        wgpu::SurfaceTexture drawable;
        surface.GetCurrentTexture(&drawable);
        if(drawable.status!=wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
           drawable.status!=wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
            if(++failures>=60 || drawable.status==wgpu::SurfaceGetCurrentTextureStatus::Error) {
                status.store(-1);return 0;
            }
            if(drawable.status==wgpu::SurfaceGetCurrentTextureStatus::Outdated ||
               drawable.status==wgpu::SurfaceGetCurrentTextureStatus::Lost)configure();
            return 2;
        }
        failures=0;
        const wgpu::RenderPassColorAttachment attachment{.view=drawable.texture.CreateView(),
            .loadOp=wgpu::LoadOp::Clear,.storeOp=wgpu::StoreOp::Store,.clearValue={0,0,0,1}};
        const wgpu::RenderPassDescriptor passDescriptor{.colorAttachmentCount=1,.colorAttachments=&attachment};
        auto encoder=g_device.CreateCommandEncoder();
        auto pass=encoder.BeginRenderPass(&passDescriptor);
        if(!black) {
            const wgpu::BindGroupEntry entry{.binding=0,.textureView=source.CreateView()};
            const wgpu::BindGroupDescriptor groupDescriptor{.layout=pipeline.GetBindGroupLayout(0),.entryCount=1,.entries=&entry};
            auto group=g_device.CreateBindGroup(&groupDescriptor);
            pass.SetPipeline(pipeline);pass.SetBindGroup(0,group);pass.Draw(3);
        }
        pass.End();auto command=encoder.Finish();g_queue.Submit(1,&command);
        surface.Present();status.store(1);return 1;
    }
private:
    void configure() {
        const wgpu::SurfaceConfiguration configuration{
            .device=aurora::webgpu::g_device,.format=wgpu::TextureFormat::BGRA8Unorm,
            .usage=wgpu::TextureUsage::RenderAttachment,.width=current->width,.height=current->height,
            .alphaMode=wgpu::CompositeAlphaMode::Opaque,.presentMode=wgpu::PresentMode::Fifo};
        surface.Configure(&configuration);
    }
};
