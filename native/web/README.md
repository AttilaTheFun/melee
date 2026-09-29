# Melee browser port

The original game builds as threaded WebAssembly with Aurora GX rendered through
WebGPU. Offline tests cover a two-player Onett match, independent keyboard and
synthetic Gamepad API controls, attacks/jumps, results, rematch, audio output,
card persistence and explicit pause/resume. Online tests cover actual game
instances over WebRTC, shared inputs, checksums, full direct matches, variable
input-delivery timing, and a local forced-TURN relay.

This is still a development build. Smooth 60 Hz online performance, ten-minute
stability, broader stage/character/browser coverage, Cloudflare deployment and
separate-network testing remain unfinished. See [MILESTONES.md](MILESTONES.md)
for precise evidence and [room setup](server/README.md) for deployment.

## Full-game build

From the repository root, install and activate the pinned Emscripten 6.0.9 SDK at
`native/build/deps/emsdk` using the commands in the viewer section below. The
native Aurora checkout and its patch are prepared by the pinned setup script.
Then run:

```sh
python3 native/tools/prepare_aurora.py
python3 native/web/build_game.py --game
python3 native/web/test-foundation.py
python3 native/web/stage_site.py
```

Build output is `native/build/wasm-renderer/`. The staging command copies nine
allowlisted public files to `native/build/wasm-site/`, including the online room
client and generated build identity. Host **only the staged directory** over
HTTPS with:

```text
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

The Cloudflare Worker sets those headers. Offline play can also use any static
host that supplies them; online room creation/joining requires the Worker.

The full runtime requires WebGPU, Wasm threads/SharedArrayBuffer and cross-origin
isolation. Its local disc picker performs Blob range reads without uploading or
copying the whole disc. Each user supplies their own USA revision 2 disc. No game
assets belong in the hosting output. Text/debug font bytes are loaded from that
disc's executable at startup; browser builds do not use generated font includes.
The viewer's `serve.py` serves a different
build and does not serve this full runtime.

The launcher lists both keyboard layouts and accepts two standard-mapped Gamepad
API controllers. Audio uses a shared PCM ring consumed by an AudioWorklet. Saves
use IDBFS and an exclusive Web Lock; the launcher offers import/export and a
play-without-saving option. Browser save reload/import/export, cross-tab exclusion and corrupt-import
preservation have passed with a nonempty test card.

## Full-game browser checks

Install Playwright separately, or set `PLAYWRIGHT_MODULE` to its `index.mjs`.
`CHROME_BINARY` optionally overrides the installed Chrome executable. These tests
run headless, use intercepted HTTPS routes, and open no listening server or host
window:

```sh
node native/web/test-gpu-bridge.mjs
MELEE_GPU_PROBE=melee_aurora_probe node native/web/test-gpu-bridge.mjs
MELEE_GPU_PROBE=melee_browser MELEE_DISC=/path/to/melee.ciso \
  node native/web/test-gpu-bridge.mjs
MELEE_GPU_PROBE=melee_browser MELEE_DISC=/path/to/melee.ciso \
  MELEE_NAVIGATE=1 MELEE_EXPECT_MATCH=1 \
  MELEE_INPUT_FILE=native/web/scenarios/stage-entry.json \
  node native/web/test-gpu-bridge.mjs
```

Build the two probe executables first with
`cmake --build native/build/wasm-renderer --target melee_gpu_bridge_probe melee_aurora_probe -j8`.
The last command verifies stage entry, not a complete match by itself. Its
keyboard timings can vary with machine load. Logs include read-only scene/fighter snapshots; artifacts remain
under the build directory. `MELEE_INTERACTIVE=1` allows JSON keyboard commands on
stdin for diagnosis. `MELEE_TEST_SAVING=1` enables persistence in the test harness;
normal boot tests otherwise disable saves to avoid changing a test browser's card.

### Gameplay regression options

Combine these with the full-game probe variables:

- `MELEE_TEST_CONTROLS=1`: verify each keyboard port moves its fighter.
- `MELEE_TEST_GAMEPADS=1`: verify each synthetic standard Gamepad API port.
- `MELEE_TEST_MOVES=1`: verify attack and jump motion transitions for both players.
- `MELEE_WAIT_RESULTS=360000`: wait up to six minutes for a complete match.
- `MELEE_TEST_REMATCH=1`: follow results with character/stage selection and a second match.
- `MELEE_TEST_SAVING=1 MELEE_TEST_PERSISTENCE=1`: test nonempty card persistence,
  import/export, corrupt import preservation and exclusive cross-tab ownership.

Recorded rematch and action passes are in `MILESTONES.md`. Physical controllers,
broader browser/device coverage and long-session performance still need testing.

## Online transport development

`node native/web/test-net-transport.mjs` exercises the real WebRTC transport in
two isolated headless browser contexts. It requires the same Playwright/Chrome
configuration as the GPU test. No HTTP server is opened. By default it tests
direct ICE only. To verify a TURN path, put short-lived `iceServers` in an
untracked local JSON file and set `MELEE_ICE_CONFIG=/path/to/file.json` and
`MELEE_RELAY_ONLY=1`; the test checks the selected candidate type. Never put the
TURN API secret in client files.

The transport is connected to a logic-tick input gate in the development runtime;
the room client and Worker are implemented. Local forced-TURN testing is
available; Cloudflare deployment and separate-network verification remain open.
The implementation follows the [WebRTC specification](https://www.w3.org/TR/webrtc/);
Cloudflare’s [credential API](https://developers.cloudflare.com/realtime/turn/generate-credentials/)
is called server-side by the Worker once its secrets are configured.

`MELEE_TEST_LIFECYCLE=1` checks three pause/resume cycles during gameplay. The
launcher automatically requests pause on tab hiding and requires Resume to
continue. `node native/web/test-net-inputs.mjs` exercises the standalone input
serialization and lockstep queue. See the two-engine test below for integration.

`MELEE_DISC=/path/to/melee.ciso node native/web/test-net-game.mjs` runs two real
Wasm engines connected through WebRTC. `MELEE_NET_TICKS=2200` requires the scripted
scenario to reach a live match and compares each recorded logic-tick snapshot;
`MELEE_NET_TICKS=9600` additionally requires match results. These are development
tests, separate from the host/join launcher. Both instances select the same local disc, and
the test supplies that shared-source identity; the production session helper
computes bounded-memory fingerprints of exact files. Different ISO/CISO file
representations currently have different fingerprints.

### Online pacing checks

Online uses input-delay lockstep, without rollback. A stable NTSC simulation
clock permits up to three logic updates before a draw when rendering is late.
Each update waits for both players' inputs and publishes a post-update snapshot;
state checks do not skip undrawn updates. Long loading stalls or waiting more
than one tick for input rebase the local clock. The latest two-instance test
measures roughly 59 simulation ticks/s and 28 draws/s per peer, so gameplay speed
and rendering smoothness are separate remaining concerns.

`MELEE_NET_CATCHUP=1,3` deliberately gives the two test peers different catch-up
limits. `MELEE_NET_MIN_TPS=55` enables a measured gameplay-speed assertion after
the match's first 300 ticks. The test prints ticks/s and rendered frames/s
separately. These controls are test settings, not a negotiated gameplay rule.

### Host/join launcher and deployment staging

The full-game launcher (`game/index.html`) now offers offline, host-room and
join-room modes. Run `python3 native/web/stage_site.py` after building to produce
`native/build/wasm-site`; serve this directory for the full launcher. It includes
the room client and generated build identity. The repository-root prototype
launcher is a separate character viewer.

See [room service setup](server/README.md) for Cloudflare configuration and
verification gates. Cloudflare deployment and wide-area TURN remain unverified.
`test-room-launcher.mjs` tests the launcher with two isolated real WebRTC peers
and a mocked room service; `test-room-server.mjs` tests the Worker boundaries.

The running game publishes `Module.runtimeProfile` every 60 rendered frames.
Its millisecond averages separate time between submissions (`logicMs`, including
input/network/pacing waits), CPU frame finalization/enqueue (`gpuSubmitMs`), draw-done
completion waiting (`drawWaitMs`), and presentation/audio/input work
(`presentMs`). These are wall-clock phase timings, not GPU timestamp queries;
draw waiting includes browser scheduling and callback delivery. They do not by
themselves identify shader execution cost. `renderQueueMs` measures the portion
of draw waiting before the render worker actually submits to WebGPU;
`completionMs` measures the remaining wait for GPU completion and callback
handling. The two-engine test logs these values.

Draw, indexed-draw, viewport, scissor, pipeline, index-buffer and vertex-buffer
operations use Emscripten's asynchronous
main-thread proxy queue. These void operations copy scalar arguments and owned resource handles only.
End-of-pass remains a synchronous barrier on the same queue, before the pass
handle can be released. Resource-handle deletion is also synchronous on that
queue, draining prior commands before registry removal. Descriptor and buffer-offset-array calls remain
synchronous because their Wasm memory may be temporary. This relies on the
pinned Emscripten 6.0.9 proxy queue's argument-copying and FIFO behavior (see
[the proxying API](https://emscripten.org/docs/api_reference/proxying.h.html)).
The GPU bridge probe verifies queued draw ordering using scissored pixel
readback, in addition to concurrent handle-lifetime stress.

### Extended tests

Full-game long-session check: combine the full-game stage-entry variables above
with `MELEE_SOAK_MS=600000`. The harness completes and rematches for at least ten
minutes, exercises both keyboard ports, records memory sizes and screenshots,
and finishes the current match/rematch before stopping. It may therefore run
longer than ten minutes. The report is `browser-soak-result.json` under the build
directory. This option's long run is pending; consult the milestone evidence.
Full online tests (`MELEE_NET_TICKS>=9000`) now treat that count as a minimum and
allow up to 6,000 additional ticks for sudden death/results. They still require
the actual results scene and compare every recorded logic tick.

# Separate character viewer

An asset-free desktop-browser character viewer built from the native port's C
code. This is **not a playable match or a port of the full Aurora renderer**.
It selects a local USA Melee ISO/CISO, decodes Mario/Fox/Peach, runs the original
skeletal animation and CPU skinning in Wasm, and draws textured geometry through
WebGPU. Orbit, zoom, pause, scrub, and character switching are implemented.

## Build and run

From the repository root, install the pinned SDK locally if it is missing:

```sh
git clone https://github.com/emscripten-core/emsdk.git native/build/deps/emsdk
native/build/deps/emsdk/emsdk install 6.0.9
native/build/deps/emsdk/emsdk activate 6.0.9
python3 native/web/build.py
python3 native/web/serve.py
```

Open `http://localhost:8080` in desktop Safari with WebGPU support (Safari 26+).
See [WebKit’s Safari 26 WebGPU release notes](https://webkit.org/blog/17333/webkit-features-in-safari-26-0/#webgpu).
Select your USA Melee disc with the file picker. Stop the server with Ctrl-C.
The server binds only to loopback and serves an explicit allowlist of the seven
application files; disc files, test fixtures, screenshots and logs are excluded.
Do not open `index.html` through `file://`; modules and workers need HTTP/HTTPS.

Output is `native/build/web/`. To host elsewhere, copy **only** `index.html`,
`app.js`, `renderer.js`, `worker.js`, `scene-data.js`, `melee.mjs`, and
`melee.wasm` to an HTTPS static host. There are no runtime npm dependencies or
CDN requests. The browser uses a dedicated module worker and Emscripten WORKERFS
for synchronous, read-only ranges of the selected File. It does not upload the
disc or copy the entire disc into Wasm. This single-threaded worker slice does
not need SharedArrayBuffer or cross-origin isolation.

## Scope and limitations

- Real native disc/DAT/endian decoding, joint animation, skinning, textures,
  fighter visibility setup, triangulation, and HSD pixel-register setup.
- The JS WebGPU renderer is a small counterpart of the native geometry
  diagnostic, **not Aurora**. One supported texture layer, base mip, approximate
  lighting and alpha/depth/blend behavior. Omitted texture layers are reported.
  Complete TEV, material animation, early-depth ordering, culling and all GX
  pixel behavior are not reproduced. Unsupported logic/destination-alpha/dither
  states fail explicitly.
- Only the first animation clip and initial fighter visibility are selected.
  Animation does not execute gameplay or subsequent action-event visibility.
- No game loop, stage, collisions, gameplay input, audio, memory cards, or saves.
- GPU device loss requires reload. Disc parsing can consume significant memory;
  Wasm memory is capped at 512 MiB. Performance is not established in Safari.
- `_Alignas(32)` preserves the native heap header on wasm32; panic backtraces
  exclude POSIX `execinfo.h` on Emscripten. Native heap/diagnostic regressions pass.

## Verification

```sh
node native/web/test-scene.mjs /path/to/melee.ciso
npm install --prefix native/build/web-gpu-test --save-exact webgpu@0.6.1
node native/web/test-renderer.mjs /path/to/melee.ciso
python3 native/web/test-webkit.py /path/to/melee.ciso
```

The first test checks all three characters, finite skinned vertices, index
bounds, changed animation frames, ownership/reload, and cleanup in actual Wasm.
The second uses Dawn's Node WebGPU binding with Metal, no window or host UI,
and the exact browser renderer. It checks shader/pipeline validation, nonempty
pixel readback, and distinct rendered animation frames. It writes local PPM
artifacts under `native/build/web/`. This **does not prove Safari compatibility**.
See [Dawn's Node binding](https://github.com/dawn-gpu/node-webgpu).

`smoke.html` exercises module-worker import, WORKERFS File input, Wasm loading,
character changes, and GPU validation in WebKit. `make-test-fixture.py` can make
a small, **nonplayable, private** ISO-shaped fixture from the nine required DAT
files of a local disc, using `native/build/inspect-disc`. Never distribute that
fixture. The smoke page and fixture are excluded from normal build/serve output.

Desktop Safari verification remains pending while the private Mac VM is
unreachable. Host desktop automation is prohibited by `../AGENTS.md` in the
Developer directory. The current WebKit Simulator test passes worker/File/Wasm animation and reloads,
exposes WebGPU but
returns no adapter; it cannot establish a Safari GPU pass.

## Dedicated simulator

Use `python3 native/tools/melee_simulator.py` to select/create the Melee-owned
simulator, with `--boot` or `--shutdown` for explicit lifecycle operations.
Use the identifier returned by that tool in `simctl` and Xcode
`-destination` arguments; keep machine-specific identifiers in local configuration. Never use the ambiguous `booted` alias, a shared
default device name, or a command that shuts down all simulators.
