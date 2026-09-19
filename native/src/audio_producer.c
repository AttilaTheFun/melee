#include "melee_audio_producer.h"
#include "melee_ax_stream.h"
#include "melee_ax_aux.h"
#include "melee_ax_voice.h"
#include <dolphin/os.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <time.h>

struct MeleeAudioProducer {
    MeleeAudioRing* queue;
    pthread_t thread;
    _Atomic bool stop;
    _Atomic int status;
};
static _Atomic bool producer_claimed;

static void* produce(void* opaque)
{
    MeleeAudioProducer* producer = opaque;
    MeleeAXStream stream;
    melee_ax_stream_init(&stream);
    while (!atomic_load_explicit(&producer->stop, memory_order_acquire)) {
        /* A 48 kHz device with a 1024-frame I/O buffer requests 682/683
         * source frames at once. The old 640-frame ceiling guaranteed a hole
         * in every callback even with an infinitely fast mixer. Keep a full
         * device quantum plus scheduling headroom; adapt if the route changes.
         * Stats are lock-free and the render callback never waits on us. */
        size_t target = melee_audio_ring_stats(producer->queue).largest_request + 640;
        if (target < MELEE_AUDIO_PREBUFFER_FRAMES) target = MELEE_AUDIO_PREBUFFER_FRAMES;
        target = ((target + 159) / 160) * 160;
        if (target > MELEE_AUDIO_RING_FRAMES / 160 * 160)
            target = MELEE_AUDIO_RING_FRAMES / 160 * 160;
        if (melee_audio_ring_queued(producer->queue) <= target - 160) {
            s16 samples[320];
            if (!melee_ax_stream_read(&stream, samples, 160)) {
                atomic_store_explicit(&producer->status, MELEE_AUDIO_FAILED, memory_order_release);
                return NULL;
            }
            if (atomic_load_explicit(&producer->stop, memory_order_acquire)) break;
            /* One producer and a consuming reader mean space cannot shrink
             * between the occupancy check and publishing this frame. */
            if (melee_audio_ring_write(producer->queue, samples, 160) != 160) {
                atomic_store_explicit(&producer->status, MELEE_AUDIO_FAILED, memory_order_release);
                return NULL;
            }
        } else {
            /* Outside the device render callback. Polling avoids making the
             * real-time consumer take a mutex or signal a condition variable. */
            const struct timespec interval = {.tv_nsec = 1000000};
            nanosleep(&interval, NULL);
        }
    }
    atomic_store_explicit(&producer->status, MELEE_AUDIO_STOPPED, memory_order_release);
    return NULL;
}

MeleeAudioProducer* melee_audio_producer_start(MeleeAudioRing* queue)
{
    if (!queue || atomic_exchange_explicit(&producer_claimed, true, memory_order_acq_rel)) return NULL;
    if (melee_audio_ring_queued(queue)) goto failed;
    int old = OSDisableInterrupts();
    bool ready = melee_ax_voice_generation() && melee_ax_aux_ready();
    OSRestoreInterrupts(old);
    if (!ready) goto failed;
    MeleeAudioProducer* producer = calloc(1, sizeof(*producer));
    if (!producer) goto failed;
    producer->queue = queue;
    atomic_init(&producer->stop, false);
    atomic_init(&producer->status, MELEE_AUDIO_PRODUCING);
    if (pthread_create(&producer->thread, NULL, produce, producer)) {
        free(producer); goto failed;
    }
    return producer;
failed:
    atomic_store_explicit(&producer_claimed, false, memory_order_release);
    return NULL;
}

int melee_audio_producer_status(const MeleeAudioProducer* producer)
{
    return producer ? atomic_load_explicit(&producer->status, memory_order_acquire) : MELEE_AUDIO_STOPPED;
}

void melee_audio_producer_request_stop(MeleeAudioProducer* producer)
{
    if (producer) atomic_store_explicit(&producer->stop, true, memory_order_release);
}

bool melee_audio_producer_destroy(MeleeAudioProducer* producer)
{
    if (!producer) return true;
    if (pthread_equal(pthread_self(), producer->thread)) return false;
    melee_audio_producer_request_stop(producer);
    if (pthread_join(producer->thread, NULL)) return false;
    free(producer);
    atomic_store_explicit(&producer_claimed, false, memory_order_release);
    return true;
}
