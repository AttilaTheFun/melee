# September 28, 2026: loading and hitch profiling

## October 3: iPhone 13 Pro follow-up

The phone still had the September personal-team build; its signature no longer
allowed launch. After Xcode authentication was restored, a current Release build
was signed with the user's business team and installed. The old memory card and
system settings were backed up and transferred to the new app container; the
memory card was verified byte-for-byte before removing the old app. Signing and
device identifiers remain in ignored local build artifacts.

An actual 30-second on-device Instruments Time Profiler capture succeeded
(`native/build/iphone13-hitches-oct03/startup.trace`). It includes opening-movie
decoding and input-wait polling; it is not a controlled combat benchmark.
With the migrated 3× MetalFX setting, a subsequent diagnostic window recorded
a 333.1 ms frame, 324.3 ms maximum render wait, and 448.6 ms aggregate pipeline
wait across that window. This supports shader compilation as one contributor,
but does not attribute every stall to it. Those navigation samples also included
diagnostic screenshots, so they must not be used as clean performance benchmarks.

A separate performance-only run switched to direct Metal presentation. Its
26 windows contained 3,098 frames and two intervals over 33.37 ms; the largest
was 173.5 ms in the window spanning the presentation-mode switch. That window
also includes 78 readbacks from before the switch; subsequent direct windows
reported zero. This is not a matched before/after gameplay comparison. A second
Instruments attachment failed to locate the running process; only the first
trace is claimed. Full combat hitch elimination remains unverified.

MetalFX diagnostics now honor `MELEE_PERF_ONLY=1`: timing-only runs no longer
add periodic upscaled GPU readbacks, CPU copies, and PNG encoding. The iPhone
runtime/app builds passed with this correction. The app was finally relaunched
normally, without diagnostic flags, with MetalFX off. Logs, signed binaries,
save backups, and signing/build output remain in the ignored directory above.

The port still has first-use shader hitches. This change reduces asset conversion
cost and shader waits; it does not certify hitch-free gameplay on every stage.

## Findings and changes

- A previous physical iPhone 15 Pro gameplay capture had a 211.9 ms frame interval
  with a maximum presentation cost of only 0.104 ms and zero CPU readbacks. This
  points beyond the display-copy path for at least some of the reported hitches.
- Windowless Mac sampling (`sample`, 1 ms interval) identified
  `melee_archive_pointer` as the largest active CPU hotspot during the sampled
  loading interval. It linearly scanned each archive's relocation table for
  every pointer, including null pointers. Actual stage files contained thousands
  of relocation entries.
- `melee_archive_open` now records whether the already-validated table is sorted.
  Pointer resolution binary-searches sorted tables without allocating an index;
  unordered files retain the original scan. External references are still checked
  first. Borrowed archive bytes must remain immutable, as documented by the API.
- Per-frame profiling then attributed roughly 90 ms of individual ~95 ms frames
  to shader waits. Metal now queues pipeline work during recording and waits at
  encoding, allowing independent jobs to overlap across two compiler workers.
  Required draws still wait for their pipeline; this does not skip uncompiled
  draws or introduce placeholders. Cache warmup remains asynchronous. Other
  backends retain the existing scheduling path.
- `AURORA_SERIAL_PIPELINES=1` selects the previous shader scheduling for diagnostic
  comparisons. Runtime diagnostics now include pipeline wait/build counts and
  durations. `MELEE_FRAME_PROFILE=/path/file.csv` enables equivalent attribution
  in the windowless startup fixture.

## Measurements

These are **Mac measurements**, not iPhone speedup claims. The HSD/game fixtures
use the existing AddressSanitizer configuration. Stage checks include decoding,
original HSD animation playback, source-disposal checks, and cleanup. Medians of
three interleaved runs:

| Stage archive | Before | After | Ratio |
| --- | ---: | ---: | ---: |
| GrCs.dat | 778 ms | 99 ms | 7.8× |
| GrNKr.dat | 801 ms | 66 ms | 12.1× |
| GrIz.dat | 352 ms | 64 ms | 5.5× |

All 18 stage checks passed. The release-mode pointer microbenchmark on these
stage/fighter files improved by 97–331× with matching result checksums; this is
only the lookup operation and must not be presented as whole-game speedup.

The shader comparison used the same bounded Fox-versus-Ness combat script and
separate copies of the same starting pipeline/Dawn databases, 2,800 frames/run:

| Run order | Serial shader waits | New shader waits | Reduction |
| --- | ---: | ---: | ---: |
| Serial, then new | 672 ms | 368 ms | 45% |
| New, then serial | 315 ms | 124 ms | 61% |

In the first pair, the measured gameplay window (frames 2000–2800) had four
intervals over 33.4 ms with serial scheduling and two with the new scheduling;
worst intervals were 120.5 and 102.5 ms respectively. In the reverse-order pair,
both had zero intervals over 33.4 ms. OS/driver caches were not reset, and exact
AI/effect execution varies, so these runs do not establish a universal hitch
reduction percentage. The remaining ~100 ms first-use stalls are real.

## Validation and artifacts

Local, ignored artifacts are under `native/build/hitch-profile/`:

- `mac-before.sample`, `mac-stage-before.sample`: CPU sample stacks.
- `archive-lookup{,-release}.log`, `stage-comparison.json`: lookup and stage checks.
- `pipeline-comparison.json`, `reverse/comparison.json`: shader comparisons.
- `serial.csv`, `deferred.csv`, `reverse/*.csv`: per-frame attribution.
- `serial.png`, `deferred.png`: gameplay framebuffer captures.
- `shutdown-render.log`: actual Metal rendering, EFB copy/retrace, and clean
  compiler/render-worker shutdown passed under ASan. An initial attempt supplied
  an incompatible demo animation and failed before GPU setup; retrying with the
  actual Fox animation archive passed.
- Core archive/PAD tests and the new lookup-equivalence regression passed.
- Device and Simulator runtime/app builds passed. The signed Release app was
  installed successfully on the iPhone (`release-install.log`); it uses optimized
  Swift and the existing optimized native runtime. The app was left for the user
  to launch while the phone was in use.
- The dedicated Simulator UI test passed direct rendering, zero additional CPU
  readbacks, background/resume and settings pause/resume
  (`simulator-ui.xcresult`). The dedicated simulator was shut down afterward.

`tools/profile_native_performance.py` repeats the shader comparison with
`--disc`, `--seed-cache`, `--output`, and optional `--reverse`. It only writes
new disposable output/cache directories. Run sequentially with other GPU tests.
`make -C native test-archive-lookup` covers sorted/unsorted relocation tables,
null pointers, zero targets, one-past-end targets, and invalid pointer words;
existing archive tests retain external-chain coverage.

## Remaining verification

The physical phone profiling attempt was interrupted by the app entering the
background. Instruments also failed to enumerate/attach to the device process;
no successful device Time Profiler trace is claimed. A new uninterrupted phone
run is still required to quantify real-device frame pacing, stage load duration,
thermal behavior and MetalFX-on performance. MetalFX still uses the CPU-backed
presentation path; this change does not remove that readback. Saves and disc
contents were not modified by these benchmarks.

## Opt-in hitch profiling (September 28)

Settings → Performance profiling → **Record performance and hitch stacks** is
persistent and off by default. It records native frame/submit/GPU/presentation
and shader counters every two seconds. A separate utility queue checks every
10 ms for a gap of at least 50.05 ms since the last completed game frame (the
expected next frame plus two missed NTSC deadlines). It captures the game
thread while it is still stalled, globally limited to one capture per second,
including across pause/resume. This is a long-frame watchdog, not a record of
every missed display refresh: smaller repeated hitches remain visible in the
aggregate timing counters. It does not automatically tune performance.

The watchdog disarms during startup, Settings, background, and other game pauses;
it rearms after a completed active frame. No screenshot/pixel readback is added.
A bounded frame-pointer walk samples PC/LR and up to 64 return addresses using
Apple thread-state APIs. The game thread is resumed before symbol lookup, JSON
encoding, or I/O. Traces include capture duration, runtime phase, Mach errors,
image UUID, load address and module offset. Optimized/inlined functions or frames
without frame pointers can be absent. A game thread waiting on another worker
or GPU identifies the wait, not the other worker's stack.

Logs stay in `Documents/Diagnostics/profile.jsonl` and
`profile-previous.jsonl`, rotating at 5 MB each (10 MB total). Settings provides
**Share profiling logs** and **Clear profiling logs**. Sharing is user initiated;
there is no upload or telemetry. Turn profiling off when not diagnosing issues:
the periodic samples and brief thread suspension have some overhead. A write
failure is shown in Settings and stops further capture until re-enabled.

Keep the exact installed runtime binary for offline symbolication:

```sh
python3 native/tools/symbolicate_hitches.py profile.jsonl \
  --binary native/build/aurora-iphoneos/libmelee_game_runtime.dylib
```

The tool checks UUIDs before invoking `atos`; unknown/system frames still retain
their in-process symbols or offsets. Rebuilding can change UUIDs. The arm64
sampler uses Apple's SDK register accessors and strips authenticated return
addresses; see [Apple's thread-state wrapper](https://github.com/apple-oss-distributions/xnu/blob/main/libsyscall/wrappers/thread_register_state.c)
and [thread_get_state](https://developer.apple.com/documentation/kernel/1418576-thread_get_state).

A windowless native test covers threshold/debounce/pause behavior and 100 live
suspend/capture/resume cycles:

```sh
clang++ -std=c++20 -O2 -g -fno-omit-frame-pointer \
  native/tests/test_hitch_profiler.cpp -o native/build/test_hitch_profiler
native/build/test_hitch_profiler
```

`GameUITests.testOptInHitchProfiling` uses simulator-only injected 180 ms stalls
to verify the Settings toggle, local export availability, pause, background,
and disable flow. The injection code is excluded from physical iOS builds.

Validation evidence for this change: `profiler-ui-retry.xcresult` passed the
simulator UI flow, and `simulator-profile-symbols.txt` resolves the injected stall
inside `profile_test_stall`. Seven simulator captures took 0.017–0.056 ms, with
at least 2.17 seconds between them. These are sampling costs, not total logging
costs or a device performance claim. The first physical Release produced a
successful 14-frame stack at `VIWaitForRetrace` (57.28 ms gap, 0.063 ms capture),
with 3× MetalFX enabled. This identifies a retrace wait, not its underlying cause.
The final installed revision additionally handles signed return addresses from
system frames in non-arm64e builds; the windowless test covers PAC stripping.
`installed-profiler-runtime.dylib` preserves the final device UUID. The earlier
phone smoke-test trace has a different UUID and must not be symbolicated against
that final binary. Native `uptimeNs` is the runtime's steady-clock timebase;
Swift summary `uptime` uses Foundation's timebase, which can differ on a device.
Compare native hitch timestamps to each other, not directly to summary uptime.

## Learned shader preparation (October 3)

The Apple game runtime now waits for every compatible pipeline loaded from its
existing catalog before entering the game. Startup displays "Preparing shaders"
with completed/total counts; this work happens on the existing two compiler
threads, not the display thread. Failed pipeline creation fails startup rather
than silently reporting preparation complete. Newly encountered configurations
continue to be saved by Aurora's asynchronous SQLite writer. Subsequent sessions
prepare them too. Preparation is per process launch, not per match.

This is configuration-driven pipeline preparation on the device, with the
existing Dawn disk cache underneath. It does not ship precompiled GPU binaries,
introduce a new Metal binary-archive implementation, or claim to have discovered
all Melee shader variants. The catalog already filters configuration versions
and sizes, and prunes incompatible versions. The catalog and Dawn cache live in
`Library/Caches/MeleeNative/GPU`; they normally survive app upgrades, but OS cache
purging or uninstalling can remove them. Initial discovery, asset loading, and
other non-shader work can still stall.

Shader logging is automatic and independent of opt-in stack profiling:

- `discovered`: an uncached configuration first requested this session;
- `compiled`: pipeline hash, duration, historical first-use frame, and whether
  the configuration came from the startup catalog;
- `waited`: a draw had to wait for a pipeline, with hash and wait duration;
- `warmup_complete`: the startup barrier completed. Its duration is the remaining
  wait after catalog loading, not all initialization or total worker CPU time.

Records use steady-clock nanoseconds, compatible with native hitch timestamps,
plus a wall-clock session identifier. The render-worker wait has no frame index
because the recorder's frame counter cannot safely be read there. The runtime
queues at most 2,048 events; a separate worker formats/writes JSONL and rotates
`shader-events.jsonl` / `shader-events-previous.jsonl` at approximately 2 MB each.
Overflow and logging/catalog errors are visible in Settings. Logs remain local.
Settings → Shader preparation shows known/new counts and **Share shader logs**.
Sharing logs does not export or merge another device's shader catalog.

Validation: the real windowless Metal runtime's cold/warm/incompatible test
passed (`native/build/shader-learning-compatibility/report.json`). Six learned
configurations were all compiled before the next first frame, with zero shader
waits in that warm test. Marking one configuration incompatible caused it to be
excluded and rediscovered. This is a short startup fixture, not combat coverage.
A separate catalog with 415 configurations prepared all 415 before the first
frame, waited approximately 1.08 seconds at the barrier on this Mac, and passed
nonblack-frame and pause/resume assertions (`shader-learning-populated/`).
Driver/OS caches were not reset; these timings are not iPhone speedup claims.

Reproduce with a newly named output directory:

```
python3 native/tools/test_shader_learning.py --disc /path/to/Melee.ciso \
  --output native/build/shader-learning-new
```

Device and Simulator app builds passed. The simulator direct-rendering and
background/settings pause/resume regression also passed (`shader-learning-ui.xcresult`).
Physical gameplay coverage remains necessary to grow the phone's own catalog
and determine how much of its remaining hitching comes from new shader variants.

## Automatic play-session diagnostics (October 3)

Settings → Debugging → **Share debugging logs** shares shader event
files, the new session history, and any opt-in profiling files together. Use
**Mark a visual problem** after noticing a rendering issue to leave a timestamped
marker with the last rendered mode, scene and stage identifiers. This does not
capture an image; a screenshot can still help diagnose the visual discrepancy.

`Documents/Diagnostics/session{,-previous}.jsonl` records app-session UUID,
wall-clock time and monotonic uptime, build/OS, game transitions, pause/resume,
foreground/background, controller product categories, graphics-setting changes,
Metal presentation state, thermal/low-power state, shader/cache failures, runtime
failures and audio errors. Every two seconds while the display clock runs it
records completed-frame progress and cumulative frame-gap/submit/GPU-wait/
presentation counters. Maxima are lifetime values, not per-sample maxima;
frame progress can include intentional Settings pauses, so correlate with pause
and lifecycle events. Native interval counters exclude explicit pause gaps.

Game context is observed on the game thread at frame boundaries and published
atomically; it is the last observed context, not a promise that a stage is still
active during a transition. Shader events now carry the same numeric context
when available. Startup catalog events may precede any game context.

Session logging does not reset the opt-in profiler's timing counters. File I/O
uses a utility queue with at most eight pending records; queue saturation or
write failure is surfaced in Settings. Each of the two session files is bounded
to approximately 2 MB. No screenshots, raw inputs, disc paths, device identifiers
or uploads are added. Expensive stack sampling remains opt-in. Sharing live
files can omit the last queued record; logs are diagnostic evidence, not a crash
reporter or a complete record of every game action.
