#pragma once
#include <cstdint>

// Wait for already-enqueued CPU render work, then for all GPU queue submissions
// preceding the fence. Does not close/submit an active recording frame and is
// therefore not, by itself, an implementation of GXWaitDrawDone.
bool melee_gpu_wait_submitted(std::uint64_t timeout_nanoseconds);
