#include "gpu_completion.hpp"
#include "lib/gfx/frame.hpp"
#include "lib/webgpu/gpu.hpp"
#include <atomic>
#include <cstdio>
#include <memory>

bool melee_gpu_wait_submitted(std::uint64_t timeout_nanoseconds) {
  using namespace aurora;
  if (!webgpu::g_instance || !webgpu::g_queue) return false;
  // Aurora's synchronization only drains its CPU worker. Submissions queued by
  // that worker must exist before placing the actual GPU completion fence.
  gfx::gpu_synchronize();
  struct Completion {
    std::atomic<bool> success{false};
  };
  // A timed-out callback can outlive this call and run during later event
  // processing or teardown. Never capture caller stack storage here.
  auto completion = std::make_shared<Completion>();
  auto future = webgpu::g_queue.OnSubmittedWorkDone(
      wgpu::CallbackMode::WaitAnyOnly,
      [completion](wgpu::QueueWorkDoneStatus status, wgpu::StringView message) {
        completion->success.store(status == wgpu::QueueWorkDoneStatus::Success,
                                  std::memory_order_release);
        if (status != wgpu::QueueWorkDoneStatus::Success) {
          std::fprintf(stderr, "Metal queue completion failed: %.*s\n",
                       static_cast<int>(message.length), message.data);
        }
      });
  auto wait = webgpu::g_instance.WaitAny(future, timeout_nanoseconds);
  if (wait != wgpu::WaitStatus::Success) {
    std::fprintf(stderr, "Metal queue completion wait failed: %u\n", static_cast<unsigned>(wait));
    return false;
  }
  return completion->success.load(std::memory_order_acquire);
}
