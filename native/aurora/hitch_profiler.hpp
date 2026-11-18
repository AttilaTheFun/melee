#pragma once
// In-process sampling for the opt-in profiler. Never allocate, symbolize, log,
// or acquire application locks while the target is suspended.
#include <cstdint>
#include <mach/mach.h>
#include <pthread.h>

namespace melee::profiling {
inline uintptr_t stripCodePointer(uintptr_t address) {
#if defined(__arm64__)
    // ptrauth_strip is a no-op in an arm64 (non-arm64e) build, but system
    // libraries can still leave signed return addresses on this thread's stack.
    // XPACLRI is in the architectural HINT space (a NOP without PAC support).
    register uintptr_t link asm("x30") = address;
    asm volatile("hint #7" : "+r"(link));
    return link;
#else
    return address;
#endif
}
struct HitchGate {
    static constexpr uint64_t threshold = 50'050'000; // Three NTSC frame periods.
    uint64_t lastCapture = 0;
    bool due(uint64_t now, uint64_t heartbeat, bool active) {
        if (!active || !heartbeat || now < heartbeat || now - heartbeat < threshold) return false;
        if (lastCapture && now - lastCapture < 1'000'000'000) return false;
        lastCapture = now;
        return true;
    }
};
struct Stack {
    uintptr_t pc[64]{};
    unsigned count = 0;
    kern_return_t status = KERN_SUCCESS;
    kern_return_t resumeStatus = KERN_SUCCESS;
};
struct ThreadSampler {
    thread_t thread = MACH_PORT_NULL;
    uintptr_t low = 0, high = 0;
    // Called by the game thread before publishing RUNNING. Thread lives for the
    // session; the send right is deliberately retained until process exit.
    void registerCurrent() {
        thread = mach_thread_self();
        high = reinterpret_cast<uintptr_t>(pthread_get_stackaddr_np(pthread_self()));
        low = high - pthread_get_stacksize_np(pthread_self());
    }
    Stack capture() const {
        Stack out;
        if (!thread) { out.status = KERN_INVALID_ARGUMENT; return out; }
        // Caller must be a different thread. All storage exists before suspend.
        out.status = thread_suspend(thread);
        if (out.status != KERN_SUCCESS) return out;
        uintptr_t fp = 0;
#if defined(__arm64__)
        arm_thread_state64_t regs{};
        mach_msg_type_number_t count = ARM_THREAD_STATE64_COUNT;
        out.status = thread_get_state(thread, ARM_THREAD_STATE64,
            reinterpret_cast<thread_state_t>(&regs), &count);
        if (out.status == KERN_SUCCESS) {
            out.pc[out.count++] = stripCodePointer(arm_thread_state64_get_pc(regs));
            out.pc[out.count++] = stripCodePointer(arm_thread_state64_get_lr(regs));
            fp = arm_thread_state64_get_fp(regs);
        }
#elif defined(__x86_64__)
        x86_thread_state64_t regs{};
        mach_msg_type_number_t count = x86_THREAD_STATE64_COUNT;
        out.status = thread_get_state(thread, x86_THREAD_STATE64,
            reinterpret_cast<thread_state_t>(&regs), &count);
        if (out.status == KERN_SUCCESS) {
            out.pc[out.count++] = regs.__rip;
            fp = regs.__rbp;
        }
#endif
        while (out.status == KERN_SUCCESS && out.count < 64 && fp >= low &&
               high >= 16 && fp <= high - 16 && (fp & (sizeof(uintptr_t)-1)) == 0) {
            uintptr_t pair[2]{};
            vm_size_t read = 0;
            if (vm_read_overwrite(mach_task_self(), fp, sizeof(pair),
                reinterpret_cast<vm_address_t>(pair), &read) != KERN_SUCCESS || read != sizeof(pair)) break;
            auto address = stripCodePointer(pair[1]);
            if (address) out.pc[out.count++] = address;
            if (pair[0] <= fp) break;
            fp = pair[0];
        }
        out.resumeStatus = thread_resume(thread);
        return out;
    }
};
} // namespace melee::profiling
