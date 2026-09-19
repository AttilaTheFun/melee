#ifndef MELEE_AUDIO_PRODUCER_H
#define MELEE_AUDIO_PRODUCER_H
#include "melee_audio_ring.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct MeleeAudioProducer MeleeAudioProducer;
enum { MELEE_AUDIO_PREBUFFER_FRAMES = 1600 };
enum { MELEE_AUDIO_PRODUCING, MELEE_AUDIO_FAILED, MELEE_AUDIO_STOPPED };
/* Start the sole producer for an initialized AX pool/aux and an empty queue.
 * The queue is borrowed and must outlive the producer. No device is started.
 * Queue 50 ms of 32 kHz PCM, increasing for larger device render requests.
 * Keep at least one render quantum plus 20 ms of scheduling headroom, rounded
 * to an AX frame and bounded by the ring capacity.
 * Do not reset the pool or add another producer while it is running. */
MeleeAudioProducer* melee_audio_producer_start(MeleeAudioRing* queue);
int melee_audio_producer_status(const MeleeAudioProducer* producer);
/* Async request is callable from an AX callback. Destroy joins the thread and
 * must be called by its control thread without holding the interrupt gate.
 * Destroy returns false from the worker itself, leaving ownership intact. */
void melee_audio_producer_request_stop(MeleeAudioProducer* producer);
bool melee_audio_producer_destroy(MeleeAudioProducer* producer);
#ifdef __cplusplus
}
#endif
#endif
