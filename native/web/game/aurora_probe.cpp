#include "lib/internal.hpp"
#include "lib/webgpu/gpu.hpp"
#include "lib/gfx/frame.hpp"
#include "lib/gfx/recording.hpp"
#include "lib/gx/fifo.hpp"
#include "lib/gx/gx.hpp"
#include "lib/gx/texture.hpp"
#include <dolphin/gx.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <filesystem>
// This fixture reads the EFB directly; display-copy state is intentionally unused.
extern "C" void GXSetDispCopyGamma(GXGamma) {}
extern "C" void GXSetDispCopySrc(u16,u16,u16,u16) {}
extern "C" void GXSetDispCopyDst(u16,u16) {}
extern "C" u32 GXSetDispCopyYScale(f32) {return 480;}
extern "C" void GXSetCopyFilter(GXBool,u8[12][2],GXBool,u8[7]) {}
extern "C" void melee_xfb_shutdown() {}
// Isolated GX fixture doesn't run game interrupt/VI callbacks.
extern "C" int OSEnableInterrupts(){return 1;}
extern "C" int OSRestoreInterrupts(int){return 1;}
extern "C" int OSDisableInterrupts(){return 1;}
namespace aurora::window {
// Embedded browser UI has no native surface lock or native window.
}
static void require(bool value,const char* message){if(!value){std::fprintf(stderr,"FAIL %s\n",message);std::abort();}}
int main() {
    using namespace aurora;
    std::filesystem::create_directories("/cache");
    g_config={};g_config.appName="Melee WebGPU test";g_config.cachePath="/cache";
    g_config.userPath="/cache";g_config.resourcesPath="/cache";g_config.msaa=1;
    require(webgpu::initialize(BACKEND_AUTO,false,640,480),"GPU initialize");
    gfx::initialize();gx::fifo::init();
    for(unsigned frame=0;frame<12;frame++){
        require(gfx::begin_frame(),"begin frame");gx::fifo::begin_frame();
        alignas(32) static unsigned char fifo[65536];if(frame==0)GXInit(fifo,sizeof(fifo));
        GXSetViewport(0,0,640,480,0,1);GXSetScissor(0,0,640,480);
        float projection[4][4]={{1,0,0,0},{0,1,0,0},{0,0,-1,0},{0,0,0,1}};
        float model[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
        GXSetProjection(projection,GX_ORTHOGRAPHIC);GXLoadPosMtxImm(model,GX_PNMTX0);GXSetCurrentMtx(GX_PNMTX0);
        GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
        GXSetNumChans(1);GXSetChanCtrl(GX_COLOR0A0,GX_FALSE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
        GXSetNumTexGens(0);GXSetNumTevStages(1);GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0,GX_PASSCLR);GXSetCullMode(GX_CULL_NONE);
        GXSetZMode(GX_FALSE,GX_ALWAYS,GX_FALSE);GXSetColorUpdate(GX_TRUE);GXSetAlphaUpdate(GX_TRUE);
        GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
        GXSetBlendMode(GX_BM_NONE,GX_BL_ONE,GX_BL_ZERO,GX_LO_COPY);
        for(unsigned half=0;half<2;half++){
            GXSetScissor(half*320,0,320,480);
            const unsigned color=(frame+half)%3;
            GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
            GXPosition3f32(-0.8f,-0.8f,0);GXColor4u8(color==0?255:0,color==1?255:0,color==2?255:0,255);
            GXPosition3f32(0.8f,-0.8f,0);GXColor4u8(color==0?255:0,color==1?255:0,color==2?255:0,255);
            GXPosition3f32(0,0.8f,0);GXColor4u8(color==0?255:0,color==1?255:0,color==2?255:0,255);GXEnd();
        }
        gx::fifo::drain();gx::fifo::end_frame();gx::texture::end_frame();gfx::finish();
        gfx::end_frame([](wgpu::CommandEncoder& encoder,std::vector<gfx::AfterSubmitCallback> callbacks){
            auto command=encoder.Finish();webgpu::g_queue.Submit(1,&command);
            for(auto& callback:callbacks)callback();gfx::after_submit();
        });
        gfx::gpu_synchronize();
        auto texture=webgpu::g_frameBuffer.texture;
        wgpu::BufferDescriptor bd{.usage=wgpu::BufferUsage::MapRead|wgpu::BufferUsage::CopyDst,.size=640*480*4};
        auto readback=webgpu::g_device.CreateBuffer(&bd);auto encoder=webgpu::g_device.CreateCommandEncoder();
        wgpu::TexelCopyTextureInfo source{.texture=texture};
        wgpu::TexelCopyBufferInfo target{.layout={.bytesPerRow=640*4,.rowsPerImage=480},.buffer=readback};
        wgpu::Extent3D size{640,480,1};encoder.CopyTextureToBuffer(&source,&target,&size);
        auto command=encoder.Finish();webgpu::g_queue.Submit(1,&command);
        std::atomic<bool> ready{false};
        readback.MapAsync(wgpu::MapMode::Read,0,640*480*4,wgpu::CallbackMode::AllowSpontaneous,
            [&](wgpu::MapAsyncStatus status,wgpu::StringView){require(status==wgpu::MapAsyncStatus::Success,"readback");ready=true;});
        auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(20);
        while(!ready){require(std::chrono::steady_clock::now()<deadline,"readback timeout");std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        auto pixels=static_cast<const unsigned char*>(readback.GetConstMappedRange());
        for(unsigned half=0;half<2;half++){
            const auto sample=(240*640+(half?400:240))*4;
            const unsigned color=(frame+half)%3;
            require(pixels[sample]==(color==0?255:0)&&pixels[sample+1]==(color==1?255:0)&&
                    pixels[sample+2]==(color==2?255:0)&&pixels[sample+3]==255,
                    "GX per-draw offset values and changing reused staging slots");
        }
        readback.Unmap();
    }
    gx::fifo::shutdown();gfx::shutdown();webgpu::shutdown();
    std::puts("PASS browser WebGPU: Aurora GX per-draw uniforms and 12 changing frames across reused staging slots");
}
