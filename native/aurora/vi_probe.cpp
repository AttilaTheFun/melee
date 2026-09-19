// Headless configuration check: no Aurora/window initialization.
#include <dolphin/vi.h>
#include "lib/dolphin/vi/vi_internal.hpp"
#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <thread>

int main() {
  GXRenderModeObj a{};
  a.fbWidth = 640; a.efbHeight = 480;
  GXRenderModeObj b{};
  b.fbWidth = 320; b.efbHeight = 240;
  VIConfigure(&a);
  std::atomic<bool> done{false};
  std::thread writer([&] {
    for (unsigned i = 0; i < 10000; ++i) VIConfigure(i & 1 ? &a : &b);
    done.store(true, std::memory_order_release);
  });
  do {
    auto size = aurora::vi::configured_fb_size();
    if (!((size.x == 640 && size.y == 480) || (size.x == 320 && size.y == 240))) {
      std::fputs("VI returned mixed render-mode dimensions\n", stderr);
      std::abort();
    }
  } while (!done.load(std::memory_order_acquire));
  writer.join();
  std::puts("Aurora VI: concurrent render-mode snapshots preserve paired dimensions");
}
