#pragma once
#include "lib/gfx/texture.hpp"
namespace melee::xfb {
// Opaque original framebuffer address -> owned native image. Read only after
// its draw-done fence; returned ownership survives later copies/reallocation.
aurora::gfx::TextureHandle image(const void* address);
void release(const void* address);
}
extern "C" void melee_xfb_shutdown();
