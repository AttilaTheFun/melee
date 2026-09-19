#include "melee_audio_ring.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

struct MeleeAudioRing {
    _Alignas(64) _Atomic uint32_t produced;
    _Alignas(64) _Atomic uint32_t consumed;
    _Atomic uint64_t requested_frames, missing_frames, underruns;
    _Atomic uint32_t largest_request;
    _Alignas(64) int16_t samples[MELEE_AUDIO_RING_FRAMES * 2];
};

MeleeAudioRing* melee_audio_ring_create(void)
{
    MeleeAudioRing* ring = NULL;
    if (posix_memalign((void**) &ring, _Alignof(MeleeAudioRing), sizeof(*ring))) return NULL;
    atomic_init(&ring->produced, 0);
    atomic_init(&ring->consumed, 0);
    atomic_init(&ring->requested_frames, 0);
    atomic_init(&ring->missing_frames, 0);
    atomic_init(&ring->underruns, 0);
    atomic_init(&ring->largest_request, 0);
    if (!atomic_is_lock_free(&ring->produced) || !atomic_is_lock_free(&ring->consumed) ||
        !atomic_is_lock_free(&ring->requested_frames)) {
        free(ring); return NULL;
    }
    return ring;
}

MeleeAudioStats melee_audio_ring_stats(const MeleeAudioRing* ring)
{
    if (!ring) return (MeleeAudioStats){0};
    return (MeleeAudioStats){
        atomic_load_explicit(&ring->requested_frames, memory_order_relaxed),
        atomic_load_explicit(&ring->missing_frames, memory_order_relaxed),
        atomic_load_explicit(&ring->underruns, memory_order_relaxed),
        atomic_load_explicit(&ring->largest_request, memory_order_relaxed)};
}

void melee_audio_ring_destroy(MeleeAudioRing* ring) { free(ring); }

void melee_audio_ring_reset(MeleeAudioRing* ring)
{
    if (!ring) return;
    atomic_store_explicit(&ring->produced, 0, memory_order_relaxed);
    atomic_store_explicit(&ring->consumed, 0, memory_order_relaxed);
}

size_t melee_audio_ring_queued(MeleeAudioRing* ring)
{
    if (!ring) return 0;
    uint32_t produced = atomic_load_explicit(&ring->produced, memory_order_relaxed);
    uint32_t consumed = atomic_load_explicit(&ring->consumed, memory_order_acquire);
    return (uint32_t) (produced - consumed);
}

size_t melee_audio_ring_write(MeleeAudioRing* ring, const int16_t* samples, size_t frames)
{
    if (!ring || frames > MELEE_AUDIO_RING_FRAMES || (frames && !samples)) return 0;
    uint32_t produced = atomic_load_explicit(&ring->produced, memory_order_relaxed);
    uint32_t consumed = atomic_load_explicit(&ring->consumed, memory_order_acquire);
    size_t count = MELEE_AUDIO_RING_FRAMES - (uint32_t) (produced - consumed);
    if (count > frames) count = frames;
    size_t offset = produced % MELEE_AUDIO_RING_FRAMES;
    size_t first = MELEE_AUDIO_RING_FRAMES - offset;
    if (first > count) first = count;
    if (first) memcpy(ring->samples + offset * 2, samples, first * 2 * sizeof(int16_t));
    if (count > first) memcpy(ring->samples, samples + first * 2, (count - first) * 2 * sizeof(int16_t));
    atomic_store_explicit(&ring->produced, produced + (uint32_t) count, memory_order_release);
    return count;
}

size_t melee_audio_ring_read(MeleeAudioRing* ring, int16_t* samples, size_t frames)
{
    if (!ring || frames > MELEE_AUDIO_RING_FRAMES || (frames && !samples)) return 0;
    uint32_t consumed = atomic_load_explicit(&ring->consumed, memory_order_relaxed);
    uint32_t produced = atomic_load_explicit(&ring->produced, memory_order_acquire);
    size_t count = (uint32_t) (produced - consumed);
    if (count > frames) count = frames;
    size_t offset = consumed % MELEE_AUDIO_RING_FRAMES;
    size_t first = MELEE_AUDIO_RING_FRAMES - offset;
    if (first > count) first = count;
    if (first) memcpy(samples, ring->samples + offset * 2, first * 2 * sizeof(int16_t));
    if (count > first) memcpy(samples + first * 2, ring->samples, (count - first) * 2 * sizeof(int16_t));
    if (frames > count) memset(samples + count * 2, 0, (frames - count) * 2 * sizeof(int16_t));
    atomic_fetch_add_explicit(&ring->requested_frames, frames, memory_order_relaxed);
    if (count < frames) {
        atomic_fetch_add_explicit(&ring->missing_frames, frames - count, memory_order_relaxed);
        atomic_fetch_add_explicit(&ring->underruns, 1, memory_order_relaxed);
    }
    if (frames > atomic_load_explicit(&ring->largest_request, memory_order_relaxed))
        atomic_store_explicit(&ring->largest_request, (uint32_t) frames, memory_order_relaxed);
    atomic_store_explicit(&ring->consumed, consumed + (uint32_t) count, memory_order_release);
    return count;
}
