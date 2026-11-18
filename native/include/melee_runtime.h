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
/* Last completed-frame context: mode:16, scene:16, stage:16; UINT64_MAX before first frame. */
uint64_t melee_runtime_session_context(void);
/* The startup barrier prepares all compatible known pipelines before gameplay.
 * Counts are snapshots; remaining includes new in-flight shaders after startup. */
typedef struct MeleeShaderStatus {
    uint64_t known, remaining, discoveries, dropped_events;
    int preparing, log_failed, catalog_failed;
} MeleeShaderStatus;
void melee_runtime_shader_status(MeleeShaderStatus* output);
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
/* Experimental anamorphic 16:9. Present the 640x480 framebuffer at 16:9;
 * render-thread projection correction preserves proportions, without changing
 * game/camera logic or offscreen render targets. Applied at frame boundaries. */
void melee_runtime_set_widescreen(int enabled);
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
    uint64_t pipeline_waits, pipeline_wait_ns, pipeline_builds, pipeline_build_ns;
} MeleeRuntimeTiming;
void melee_runtime_take_timing(MeleeRuntimeTiming* output);
/* Non-resetting lifetime frame/phase counters; pipeline fields are not populated. */
void melee_runtime_session_timing(MeleeRuntimeTiming* output);
/* Call only from one serial background queue, never the game thread. A due
 * capture briefly suspends the game thread, copies bounded frame-pointer stack
 * addresses, resumes it, then symbolizes. JSON includes UUIDs for offline atos.
 * Two missed NTSC deadlines (~50ms gap), at most one capture/second. Paused and
 * startup periods are excluded. Returns JSON bytes, or zero if no capture due. */
size_t melee_runtime_capture_hitch(char* output, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
