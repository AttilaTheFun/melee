#ifndef MELEE_RUNTIME_H
#define MELEE_RUNTIME_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* One game session per process. Call run on a dedicated thread. The original
 * game owns HSD input/frame processing; UI producers only publish PADStatus.
 * Frame copies are opaque RGBA8 (alpha 255), top-left origin, 640x480 with
 * a 2560-byte row. */
enum { MELEE_RUNTIME_IDLE, MELEE_RUNTIME_STARTING, MELEE_RUNTIME_RUNNING,
       MELEE_RUNTIME_FAILED };
int melee_runtime_run(const char* disc_path, const char* cache_path);
/* Optional disk-backed Slot A card. Existing files are never reformatted. */
int melee_runtime_run_with_save(const char* disc_path, const char* cache_path,
                                const char* card_path);
int melee_runtime_state(void);
/* Retains a CAMetalLayer until replaced/detached; dimensions are drawable pixels.
 * Only the runtime thread performs surface work. State: 0 pending/off, 1 direct,
 * -1 failed (UI should detach and use CPU fallback). */
void melee_runtime_set_metal_layer(void* layer, uint32_t width, uint32_t height);
int melee_runtime_metal_state(void);
uint64_t melee_runtime_frame_sequence(void);
uint64_t melee_runtime_cpu_readbacks(void);
void melee_runtime_error(char* output, size_t capacity);
/* Returns the latest nonzero sequence, or zero if no new frame is available.
 * A successful copy owns its bytes independently of the game/GPU threads. */
uint64_t melee_runtime_copy_frame(void* rgba, size_t capacity, uint64_t after);
void melee_runtime_set_paused(int paused);
/* Foreground display-link pacing; disable when the link stops. */
void melee_runtime_set_display_clock(int enabled);
void melee_runtime_display_tick(void);
/* Diagnostic interval totals. Taking a snapshot resets totals, not timing. */
typedef struct MeleeRuntimeTiming {
    uint64_t frames, interval_ns, max_interval_ns, over20ms, over33ms;
    uint64_t submit_ns, max_submit_ns, wait_ns, max_wait_ns;
    /* Legacy field names: entire presentation phase, including direct GPU draw. */
    uint64_t readback_ns, max_readback_ns;
    uint64_t cpu_readbacks, direct_presents;
} MeleeRuntimeTiming;
void melee_runtime_take_timing(MeleeRuntimeTiming* output);
#ifdef __cplusplus
}
#endif
#endif
