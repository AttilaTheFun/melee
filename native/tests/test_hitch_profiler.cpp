#include "../aurora/hitch_profiler.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <thread>
using namespace melee::profiling;
static std::atomic<bool> ready{false}, done{false};
static ThreadSampler sampler;
__attribute__((noinline)) static void blocked_game_fixture() {
    sampler.registerCurrent(); ready.store(true);
    while (!done.load()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
int main() {
#if defined(__arm64__)
    // System frames may contain authenticated pointers even in an arm64 app.
    const uintptr_t original=reinterpret_cast<uintptr_t>(&blocked_game_fixture);
    register uintptr_t signedAddress asm("x30")=original;
    asm volatile("hint #25" : "+r"(signedAddress)); // PACIASP
    assert(stripCodePointer(signedAddress)==original);
#endif
    HitchGate gate;
    assert(!gate.due(100000000, 0, true));
    assert(!gate.due(100000000, 1, false));
    assert(!gate.due(50000000, 1, true));
    assert(gate.due(50050001, 1, true));
    assert(!gate.due(1050050000, 1, true));
    assert(gate.due(1050050001, 1, true));
    assert(!gate.due(2050050001, 2050000000, true)); // Healthy frames rearm.
    assert(!gate.due(2050050001, 0, true)); // Pause clears heartbeat.
    std::thread thread(blocked_game_fixture);
    while(!ready.load())std::this_thread::yield();
    for(int i=0;i<100;++i) {
        auto stack=sampler.capture();
        assert(stack.status==KERN_SUCCESS && stack.resumeStatus==KERN_SUCCESS);
        assert(stack.count>=3 && stack.count<=64);
        for(unsigned j=0;j<stack.count;++j)assert(stack.pc[j]);
    }
    done.store(true);thread.join();
    puts("PASS: missed-deadline threshold, one-second debounce, pause exclusion, 100 live stack captures and resumes");
}
