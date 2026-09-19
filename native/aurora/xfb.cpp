// Native display copies preserve an image independently of the EFB. The
// initial path supports unfiltered, unity-scale RGB snapshots; additional
// GameCube copy conversions must be implemented before those modes can run.
#include "xfb.hpp"
#include "lib/dolphin/gx/gx.hpp"
#include "lib/dolphin/gx/__gx.h"
#include "lib/gx/fifo.hpp"
#include "lib/gx/gx.hpp"
#include "lib/gfx/recording.hpp"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <unordered_map>

namespace {
struct CopyState {
  u16 left=0, top=0, width=640, height=480, stride=640;
  u32 scale=256, clamp=3;
  GXGamma gamma=GX_GM_1_0;
  bool aa=false, identityFilter=true;
} copy;
std::mutex imagesMutex;
std::unordered_map<const void*, aurora::gfx::TextureHandle> images;
void require(bool condition, const char* message) {
  if (!condition) { std::fprintf(stderr,"GX display copy: %s\n",message); std::abort(); }
}
}
namespace melee::xfb {
aurora::gfx::TextureHandle image(const void* address) {
  std::lock_guard lock{imagesMutex};
  auto found=images.find(address);
  return found==images.end()?nullptr:found->second;
}
void release(const void* address) {
  std::lock_guard lock{imagesMutex}; images.erase(address);
}
}
extern "C" void melee_xfb_shutdown() {
  std::lock_guard lock{imagesMutex}; images.clear(); copy={};
}
extern "C" void GXSetDispCopySrc(u16 left,u16 top,u16 width,u16 height) {
  require(width && height && width<=1024 && height<=1024 && left<=1023 && top<=1023,
          "invalid source rectangle");
  copy.left=left;copy.top=top;copy.width=width;copy.height=height;
}
extern "C" void GXSetDispCopyDst(u16 width,u16 height) {
  (void)height; // SDK programs a row stride, not an output height.
  require(width>=16 && width<=1024,"invalid destination stride");
  copy.stride=(width>>4)<<4; // Original SDK counts 32-byte blocks of YUYV.
}
extern "C" u32 GXSetDispCopyYScale(f32 scale) {
  require(std::isfinite(scale) && scale>=1.0f && scale<=256.0f,"invalid vertical scale");
  copy.scale=static_cast<u32>(256.0f/scale)&0x1ff;
  return static_cast<u32>(copy.height*(256.0f/static_cast<float>(copy.scale)));
}
extern "C" void GXSetCopyClamp(GXFBClamp clamp) {
  copy.clamp=static_cast<u32>(clamp)&3;
  // This setting also affects texture copies in the original SDK.
  __gx->cpTex=(__gx->cpTex&~3u)|copy.clamp;
}
extern "C" void GXSetCopyFilter(GXBool aa,u8 pattern[12][2],GXBool filtered,u8 weights[7]) {
  (void)pattern;
  copy.aa=aa!=GX_FALSE;
  require(!filtered || weights,"missing vertical filter coefficients");
  copy.identityFilter=!filtered ||
    (((weights[0]|weights[1]|weights[5]|weights[6])&63)==0 &&
     ((weights[2]&63)+(weights[3]&63)+(weights[4]&63))==64);
}
extern "C" void GXSetDispCopyGamma(GXGamma gamma) { copy.gamma=gamma; }
extern "C" void GXCopyDisp(void* destination,GXBool clear) {
  using namespace aurora;
  require(destination,"null framebuffer address");
  require(copy.scale==256 && copy.stride>=copy.width,"scaled/overlapping display copies are not implemented");
  require(!copy.aa && copy.identityFilter && copy.gamma==GX_GM_1_0,
          "AA, nonidentity filter or gamma conversion is not implemented");
  gx::fifo::drain();
  require(!gfx::is_offscreen(),"display copy from an offscreen pass is not supported");
  const auto [width,height]=gfx::get_render_target_size();
  require(width && height && static_cast<u32>(copy.left)+copy.width<=width &&
          static_cast<u32>(copy.top)+copy.height<=height,"source is outside the native EFB");
  const auto [logicalWidth,logicalHeight]=gx::logical_fb_size();
  require(gx::g_gxState.viewportPolicy==AURORA_VIEWPORT_NATIVE ||
          (width==logicalWidth && height==logicalHeight),"internally scaled EFB copies are not implemented");
  require(!clear || (copy.left==0 && copy.top==0 && copy.width==width && copy.height==height),
          "partial-rectangle EFB clear is not implemented");
  // A fresh image per copy keeps an already-latched image immutable while the
  // game reuses the same framebuffer address. Frame packets retain GPU handles.
  auto texture=gfx::new_render_texture(copy.width,copy.height,GX_TF_RGBA8,"Melee XFB");
  require(static_cast<bool>(texture),"image allocation failed");
  gfx::resolve_pass_into(texture,{copy.left,copy.top,copy.width,copy.height},
    clear && gx::g_gxState.colorUpdate,clear && gx::g_gxState.alphaUpdate,
    clear && gx::g_gxState.depthUpdate,gx::g_gxState.clearColor,
    gx::clear_depth_value(),GX_TF_RGBA8);
  std::lock_guard lock{imagesMutex};images[destination]=std::move(texture);
}
