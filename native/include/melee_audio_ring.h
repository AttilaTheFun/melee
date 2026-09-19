#ifndef MELEE_AUDIO_RING_H
#define MELEE_AUDIO_RING_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MeleeAudioRing MeleeAudioRing;
enum { MELEE_AUDIO_RING_FRAMES = 8192 };
/* One producer and one consumer. Create/destroy only while both are stopped.
 * Allocation verifies that the counter atomics are lock-free on this host. */
typedef struct MeleeAudioStats {
    uint64_t requested_frames, missing_frames, underruns;
    uint32_t largest_request;
} MeleeAudioStats;
/* Lock-free counters; safe to sample off the render thread. */
MeleeAudioStats melee_audio_ring_stats(const MeleeAudioRing* ring);
MeleeAudioRing* melee_audio_ring_create(void);
void melee_audio_ring_destroy(MeleeAudioRing* ring);
/* Discard queued PCM only after both producer and consumer have stopped. */
void melee_audio_ring_reset(MeleeAudioRing* ring);
/* Producer-side occupancy snapshot, for pacing. Only the producer calls this. */
size_t melee_audio_ring_queued(MeleeAudioRing* ring);
/* Nonblocking interleaved signed-16 L/R transport. Write accepts as many frames
 * as fit and returns that count; producer retains any unwritten suffix.
 * Read returns available frames and zero-fills the requested remainder.
 * Each call is bounded to 8192 frames; larger or invalid requests return zero
 * without modifying buffers. No locks, allocation, callbacks or game code. */
size_t melee_audio_ring_write(MeleeAudioRing* ring, const int16_t* samples, size_t frames);
size_t melee_audio_ring_read(MeleeAudioRing* ring, int16_t* samples, size_t frames);
#ifdef __cplusplus
}
#endif
#endif
