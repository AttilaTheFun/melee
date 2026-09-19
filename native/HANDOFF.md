# Native Melee ARM port — Astra / Codex CLI handoff

Prepared September 19, 2026. Latest engineering/test work was September 17.
This is a continuation handoff, not a claim that the port is finished.

## Start here

Continue in `/Users/logan/Developer/melee`, the user's fork at
`https://github.com/AttilaTheFun/melee.git`, branch `master`.
Read `/Users/logan/Developer/AGENTS.md`, this document, then `native/STATUS.md`.
`native/README.md` is a large chronological engineering journal: old statements
such as “not connected,” “no physical iPhone,” or “unfinished character support”
may have been superseded. Prefer STATUS.md and this handoff for current claims.

The user wants an actual native ARM64 port of their Melee disc on macOS and
on iPhone/iPad, with customizable WASD/arrows/key bindings, game controllers,
touch controls, and opt-in MetalFX upscaling. This runs original recompiled game
logic with native SDK/asset adaptations and an Aurora GX renderer through
Dawn/Metal; it is not Dolphin CPU emulation. The full port remains unfinished.

The latest development request was to finish the proposed reliability/input
verification work and remove the frame copy for performance. That implementation
exists, but final device and post-Xcode-update Simulator verification remains.
This handoff request authorizes committing and pushing all port source work.

## Immediate next work, in order

1. Inspect `git status`, current Xcode/Simulator versions, connected devices and
   any running tests. Do not assume the September 17 device/process IDs still work.
2. Resolve the final Simulator startup failure. Before a toolchain update, direct
   Metal passed frame/lifecycle tests and touch navigation to Classic selection.
   During the last interactive session, CoreSimulator changed from expected
   service 1166 to 1171.7, interrupting Xcode. A fresh retry then stayed at frame
   sequence zero for 20 seconds. The subsequent diagnostic launch found the
   Simulator shut down. Root cause is unresolved; do not simply label it a
   renderer regression or an environment-only problem without further evidence.
3. Once the test iPhone is connected/unlocked, install the signed candidate,
   measure real gameplay/audio/pacing, and test direct Metal → MetalFX 2×/3× → Off,
   background/resume, and touch input. Preserve the known-good rollback bundles.
   No speedup has yet been measured on hardware for the new direct path.
4. Finish Mac window/resizing/settings/keyboard tests in the private VM, then
   physical game-controller tests. Model/synthetic-controller tests are not
   proof of real Bluetooth delivery.
5. Extend normal gameplay reliability and earned-unlock saves. Controlled campaign
   fixtures are useful regression tests but do not prove ordinary human completion
   of every mode. Keep fixes native-only where needed to preserve console matching.
6. Consider a GPU-only MetalFX path after the default path is verified; MetalFX
   currently retains the older CPU readback/upload path.

## Operating constraints and user preferences

- Never drive or capture the host Mac desktop. The user's parent AGENTS.md
  requires Mac GUI testing inside `~/Developer/macos_vm`; read its README.
  Use `guest.sh app`, `guest.sh windows`, `guest.sh cmd`, and `guest.sh shot` over
  SSH. Do not focus/click the VM window from the host. Windowless native GPU
  tests and Simulator-specific automation do not manipulate the host desktop.
- Run GPU/game/Simulator tests sequentially. Do not package/rebuild app bundles
  while one of those tests is running. Avoid leaving orphan test processes.
- Preserve real saves, other installed apps and the user's supplied disc.
  Only removal of UniversalUIUITestRunner was authorized previously; it was
  removed to free a development slot. Do not delete more apps by analogy.
- The user values autonomous progress, accurate evidence, and concise updates.
  Don't ask again for already-authorized testing/installing on the test phone.
  Ask for unlocking/reconnecting when actually needed.
- The user confirmed good audio and smoother motion on the PRE-direct-Metal phone
  build. Do not replace it casually or claim the new candidate was user-confirmed.
- The private VM was locked at last check. Its password was supplied earlier in
  chat; it is deliberately not placed in a Git document. Ask the user if needed.
- Do not publish disc assets, extracted assets, memory cards, signing keys or
  profiles. The source commit excludes ignored build data. Local evidence is
  retained on this machine; a fresh clone alone does not contain that evidence.

## Repository and local data

- Upstream source starting HEAD before this port checkpoint:
  `fa0bac8b12dd03e8b0a0389931fc5c398b7c07ca`.
- The port had 303 modified existing files plus 526 untracked source/tool/doc
  files before this handoff was added. These are the integrated port, not a
  small isolated patch. The handoff checkpoint captures them together.
- Source architecture: `native/src`, `native/include`, `native/aurora`,
  `native/apple`, `native/tests`, `native/tools`, plus conditional changes in
  `src/` and `extern/dolphin/`. `MELEE_NATIVE` selects native paths;
  `MELEE_AURORA` also affects GX SDK object sizes/ABI.
- User disc:
  `/Users/logan/Downloads/Super Smash Bros. Melee (USA) (En,Ja) (Rev 2).ciso`.
  Test copy: `native/build/simulator-test.ciso`.
- The app's original memory-card pipeline persists under Application Support,
  `MeleeNative/MemoryCardA.card`. On iPhone the disc is Documents/Melee.ciso.
  Keep container data across app upgrades. Regression save tests use temporary
  cards and do not establish persistence of legitimately earned unlocks.
- `native/build/` is ignored: CMake trees, downloaded dependencies, app bundles,
  extracted test assets/fonts, diagnostic logs, screenshots and xcresults live
  there. Do not delete/clean it during handoff.
- Known-good rollback: `native/build/baselines/2026-09-17-confirmed/` contains
  `source.tar.gz`, `manifest.json`, and mac/iphone/simulator app bundles.
  Source SHA-256:
  `b65218e0dee818832dd79de2027596832ed422013d04db17ab72137650fee122`.
  It was verified again during handoff. Extract into a separate directory rather
  than overwriting the working tree. Signing may expire; rebuild/re-sign if needed.
- Aurora dependency checkout: `native/build/deps/aurora`, pinned to
  `749d6ee7a22bdfab78c8ece9047bca5d79aa72ca` from encounter/aurora.
  Its local edits exactly match `native/aurora/aurora.patch`; the preparer checks
  this and refuses unexpected edits. Those edits are preserved by the patch,
  not by pushing the ignored nested Git checkout. Licenses accompany the code.
- `native/third_party/dolphin_dsp/dsp_coef.bin` is a 4096-byte pinned reconstructed
  Dolphin coefficient file, with provenance/license in that directory. It is
  not a game asset; `prepare_ax_coefficients.py --check` verifies it and the include.

## Implementation details the next agent needs

### Direct Metal candidate (September 17)

`native/aurora/metal_present.hpp` owns a Dawn `SurfaceSourceMetalLayer` surface.
`native/apple/DirectMetalSurface.swift` supplies a UIView/NSView-backed
CAMetalLayer. UI publishes retained layer/size snapshots; the game thread owns
surface creation/configuration, commands and presentation. A fullscreen triangle
samples the existing XFB GPU texture with nearest sampling and forces alpha=1.
The target is opaque BGRA8Unorm, FIFO presentation. Zero-alpha GX output must
not make the window transparent.

`game_runtime.cpp::publish_frame` takes this path before CPU readback. Successful
presentation advances sequence and `direct_presents`; transient unavailable
surfaces do not publish a frame; persistent failure falls back to CPU. The
existing required GX draw-done wait remains. This removes GPU→CPU pixel mapping,
CPU copies/CGImage construction, and CPU→GPU image upload; it still performs a
fullscreen GPU draw. Do not call it zero GPU work or claim a measured speedup.

`GameView.swift` uses direct presentation when MetalFX is Off and the path has
not failed. MetalFX 2×/3× still use CPU images. `MELEE_CPU_PRESENTATION=1` forces
the old path. UI sampling on the direct path polls sequence without constructing
Data/CGImage. Separate `pixel_sequence` prevents returning old CPU bytes as a
new frame when switching paths. Runtime APIs expose layer setting, direct state
(0 pending/off, 1 direct, -1 failed), sequence, and CPU-readback totals.

`MeleeRuntimeTiming` adds `cpu_readbacks` and `direct_presents`. Its legacy
`readback_ns` fields now measure the full presentation phase; Swift diagnostics
name these `presentationMs` and `maxPresentationMs`. Do not misread them as proof
of CPU copying. Counters establish that distinction.

Capture pitfall: XCTest's XCUIScreen images were stale during the interactive
Metal session even while rendering/input continued. `simctl io <id> screenshot`
produced fresh actual Simulator images. The legacy diagnostics `frame.png`
from `game.frame` is also stale or absent during direct mode. Always establish
image freshness before calling a screen frozen. The earlier apparent title freeze
was disproved by thread sampling, fresh captures and successful menu navigation.

### Crash fixed by the new reliability run

The first Classic next-round run crashed under ASan at ftanim.c:466. Item drop
restored a retained animation tree after a fighter entered no-animation state;
`anim_id == -1` indexed before the motion array. The native-only guard in
`ftAnim_80070CC4` now restores the tree only with a valid active animation;
otherwise it uses the existing joint-reset branch. The subsequent five-check
suite passed. See `playable-20260917/classic-round.log` for the failure and
`playable-20260917-fixed/` for the passing rerun. Matchups may vary; fixtures
are controlled procedures, not a claim of fully deterministic gameplay.

### Preserve the prior audio and pacing fixes

- The old 640-source-frame audio ceiling was smaller than a real 683-source-frame
  callback on the iPhone's 48 kHz route. Buffering is now 1600 frames (50 ms at
  32 kHz), adapting to largest request +640, rounded to160 and bounded by the ring.
  Real phone evidence: 10,990,934 delivered frames over343 seconds, zero missing
  frames/underruns. User confirmed sound good.
- Disabled ADPCM loop addresses are checked only if looping is enabled. Stream
  updates choose a running stereo channel; finished voices wait for cleanup
  instead of asserting. Do not undo these fixes while tuning latency.
- iOS uses a persistent GameFrameClock CADisplayLink at60 with weak main-actor
  target. Sample input/frame, then signal the VI worker. Coalesce delayed pulses;
  keep game callbacks off main. Phase accounting preserves 1000/1001 NTSC timing.
  Stopping display clock returns VI to autonomous timing; macOS uses a Timer.
- Prior phone baseline skipped328/2410 frames. Fixed sample delivered2398/2398
  over approximately40s (59.85 FPS), no audio underruns, nominal thermal state.
  It still had27 game intervals above20ms, none above33.37ms. This is frame/UI
  handoff evidence, not panel scanout latency or an isolated sustained fight
  benchmark. User confirmed “It feels a lot smoother now.”

## Build and test commands

Use existing configured build trees on this Mac first. During handoff,
`xcodebuild -version` reported Xcode27.0, build27A266a. Earlier test tooling
changed mid-run, so verify SDK/runtime state rather than assuming compatibility.
Tools include Xcode, Python3, CMake, Ninja and xcodegen. Network is needed to
fetch pinned dependencies on a clean checkout.

```sh
cd /Users/logan/Developer/melee
python3 native/tools/prepare_aurora.py
python3 native/tools/prepare_ax_coefficients.py --check
cmake --build native/build/aurora-integration --target melee_game_startup melee_runtime_probe -j 6
python3 native/tools/test_playable_baseline.py --disc native/build/simulator-test.ciso
```

The runner creates a unique report/log/image directory, runs Classic match,
next round, game-over, rematch, then disposable-card write/reload. It fails fast
and kills subprocess groups on timeout/interruption. `--campaigns` adds longer
controlled Classic/All-Star runs. Reports describe controlled scope explicitly.
For foundation/input checks use `make -C native all test` and
`python3 native/tools/test_apple.py`; these are not substitutes for real input.
The larger HSD/asset suite is configured in `native/aurora/CMakeLists.txt`;
inspect `ctest --test-dir native/build/aurora-integration -N` before selecting it.

Packaging (sequential, no running game tests):

```sh
python3 native/tools/build_runtime_app.py
python3 native/tools/build_ios_runtime_app.py --sdk iphonesimulator
python3 native/tools/build_ios_runtime_app.py --sdk iphoneos
python3 native/tools/prepare_ios_app_project.py
xcodebuild -project native/build/ios-device-project/MeleeNative.xcodeproj \
  -scheme MeleeNative -configuration Debug -destination 'generic/platform=iOS' \
  -derivedDataPath native/build/ios-device-project/DeviceDerivedData \
  DEVELOPMENT_TEAM=9CLW2BBDAG build
```

App locations:
- Mac: `native/build/macosx-game/MeleeNative.app`.
- Simulator: `native/build/iphonesimulator-game/MeleeNative.app`.
- Unsigned iPhone: `native/build/iphoneos-game/MeleeNative.app`.
- Signed candidate: `native/build/ios-device-project/DeviceDerivedData/Build/Products/Debug-iphoneos/MeleeNative.app`.

Clean configure recipes (expensive; don't throw away local trees unnecessarily):

```sh
cmake -S native/aurora -B native/build/aurora-integration -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=26.0 -DMELEE_AURORA_SANITIZE_GAME=ON
cmake -S native/aurora -B native/build/aurora-iphoneos -G Ninja \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
  -DAURORA_DAWN_PROVIDER=package -DAURORA_SDL3_PROVIDER=vendor \
  -DAURORA_CACHE_USE_ZSTD=OFF -DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew \
  -DMELEE_AURORA_SANITIZE_GAME=OFF
```

For Simulator use the iOS recipe with directory `aurora-iphonesimulator`,
SYSROOT `iphonesimulator`, `AURORA_DAWN_PROVIDER=vendor`,
`DAWN_BUILD_PROTOBUF=OFF`, `TINT_BUILD_IR_BINARY=OFF`. Do not reuse device binaries
in Simulator. Mac sanitizer build is a correctness build, not an iPhone speed proxy.
The full runtime requires the Aurora/Dawn dependencies; older README claims of
no downloaded dependencies apply only to foundation targets.

### Simulator workflow

Last test device UUID: `530C89B3-A8F0-4165-BD41-4A0C9E6D9666`.
Check `xcrun simctl list devices` first. XCTest game bundle is
`dev.melee.native.uitestgame`, separate from the normal app.

```sh
python3 native/tools/prepare_ios_ui_tests.py --session-dir native/build/new-control-session
xcodebuild -project native/build/ios-ui-tests/MeleeUITests.xcodeproj \
  -scheme MeleeUITests \
  -destination 'platform=iOS Simulator,id=530C89B3-A8F0-4165-BD41-4A0C9E6D9666' \
  -derivedDataPath native/build/ios-ui-tests/DerivedData \
  -resultBundlePath native/build/NEW-UNIQUE-NAME.xcresult \
  -parallel-testing-enabled NO \
  '-only-testing:MeleeGameUITests/GameUITests/testFrameClockResumesAfterBackgroundAndSettings' \
  '-only-testing:MeleeGameUITests/GameUITests/testGraphicsAndTouchAppearance' test
```

`testControlSession` instead enables file-driven bounded touch commands. Use a
fresh session directory and wait for state.json, then:

```sh
python3 native/tools/ios_control.py native/build/new-control-session press --button Start --duration .6
python3 native/tools/ios_control.py native/build/new-control-session stick --x .22 --y 1 --duration .55
python3 native/tools/ios_control.py native/build/new-control-session quit
```

Commands require acknowledgements; never overwrite a pending command. Typical
navigation: Start skips intro, Start at title enters menu (don't wait beyond its
attract-mode window), three A presses reach Classic selection, stick(.22,1) for
~.55s moves the initial cursor onto Mario, A selects, Start begins. Inspect
fresh screenshots between navigation steps; timing/starting screens can differ.
XCTest touches are within the Simulator, not host events. The session times out
after30min; quit it explicitly, wait for XCTest exit and verify app termination.

## Physical test device / diagnostics

Authorized old iPhone13 Pro, A15, iPhone14,2, last observed iOS18.7.8.
CoreDevice ID: `684AF0BF-87F2-5C64-99B7-4F5ADD7B04E5`.
UDID: `00008110-001E219A1E44801E`.
Bundle: `dev.melee.native.game`; personal signing team `9CLW2BBDAG`.
Paid team `B86E5YKCTK` required accepting an updated agreement; do not accept
legal terms on the user's behalf. Existing personal-team signing worked.

Last known state: old confirmed pacing/audio build remains installed. The new
September17 direct-Metal candidate was built/signed but NOT installed. Connection
first failed, then device tools reached the phone but reported it locked.
Recheck now; don't assume current lock state. Preserve app data during upgrade.

```sh
xcrun devicectl list devices
xcrun devicectl device info processes --device 684AF0BF-87F2-5C64-99B7-4F5ADD7B04E5
xcrun devicectl device install app --device 684AF0BF-87F2-5C64-99B7-4F5ADD7B04E5 \
  native/build/ios-device-project/DeviceDerivedData/Build/Products/Debug-iphoneos/MeleeNative.app
xcrun devicectl device process launch --device 684AF0BF-87F2-5C64-99B7-4F5ADD7B04E5 \
  --terminate-existing --console \
  --environment-variables '{"MELEE_DEVICE_DIAGNOSTICS":"1","MELEE_DEVICE_CONTROL":"1","MELEE_PERF_ONLY":"1"}' \
  dev.melee.native.game
```

Always terminate-existing for diagnostic launches: iOS may reopen the app after
install, and launching an existing process does not apply new environment flags.
That previously led to stale diagnostic files masquerading as a stuck process.
Normal launch uses terminate-existing without diagnostics env. Restore normal
launch and MetalFX Off after testing. Don't uninstall/reinstall to clear caches
because that risks real saves; inspect per-container cache paths deliberately.

`native/tools/device_control.py` uses bounded pad commands through the app's
container, with sequence/ack freshness. Example:
`python3 native/tools/device_control.py --device <CoreDeviceID> --output <unique-dir> --button Start --duration .6`.
It also supports analog axes and `--metalfx 2`; inspect its help. These are
synthetic pad commands, not physical touch or Bluetooth proof.

App diagnostics report frame timing, direct/readback counters, audio and memory.
Collect performance before attaching LLDB: debugger pauses contaminate intervals.
Own-app physical screenshots were previously captured via LLDB/UIKit into the
app's Documents and copied using devicectl. That method may omit CAMetalLayer
pixels; verify before trusting a black/stale screenshot. Never substitute a host
desktop capture. Detach LLDB and stop its session after capture.

## Evidence index and honest coverage

All paths below are relative to `native/build/` and remain local/ignored.

| Evidence | What it establishes |
| --- | --- |
| `playable-20260917-fixed/report.json` | Five native fixture checks exit0 after animation guard |
| `playable-20260917-fixed/classic-match.png`, `classic-round.png` | Inspected live match and next-round framebuffers |
| `playable-20260917-fixed/save-reload/` | Disposable card fresh-process rumble/unlock restore, nonzero PCM |
| `apple-input-20260917.log` | WASD/arrows, rebind/persistence, cancellation, focus, synthetic controller slots/disconnect pass |
| `direct-metal-simulator.xcresult` | Earlier two tests pass: advancing direct frames with no added CPU readbacks; background/resume/settings; unavailable MetalFX UI |
| `direct-metal-attachments/2428058B-43FA-4C17-8DDE-C787BC840B20.png` | Test header frame295, direct1, readbacks0 |
| `direct-metal-final-{mac,simulator,iphone,signed}.log` | Final candidate builds pass with animation fix |
| `direct-metal-navigation-{6,9,10}.png` | Fresh simctl captures: menu, Classic selection, Mario cursor via touch |
| `direct-metal-final-simulator.log` | Interrupted by CoreSimulator version change, NOT a completed pass |
| `direct-metal-service-retry.xcresult` / `.log` | Graphics UI passed; initial frame progress failed at0 after20s |
| `direct-metal-service-retry-attachments/` | Failure hierarchy and startup images |
| `direct-metal-stall.sample.txt` | Rendering continued during stale-XCTest-capture investigation; not the later zero-frame startup failure |
| `iphone13-performance-final.jsonl` | Prior confirmed pacing sample; not new direct-path performance |
| `iphone13-paced-results.png` | Prior physical Classic results capture,55,200 points |
| `iphone13-audio-fixed/gameplay/received/state.json` | Prior audio/no-underrun evidence |
| `baselines/2026-09-17-confirmed/` | Recovery source/apps predating direct presentation |

See STATUS.md for earlier broad tests: all294 HSD/asset checks; all13 All-Star
fights/12 portals/credits via controlled replay; controlled Classic campaign,
Master Hand/credits/unlock encounter; segmented Adventure checkpoints; Training
speed/items/CPU/resume; target-course clear checkpoints; Home-Run entry/timer/
return. These do not establish all normal gameplay, every record calculation,
all playable events, full Adventure continuity, or earned-unlock persistence.
Physical iPad is untested. Physical controller and final Mac GUI verification
remain outstanding. Long sustained thermal/memory performance remains unverified.

## Handoff verification / don't lose context

The September19 handoff operation checked the remote, source whitespace,
Aurora patch equivalence, coefficient provenance, and source inventory. The
whole-file staged whitespace checker flags unified-diff blank context lines in
`aurora.patch`; those spaces are required patch syntax, so the patch is retained
byte-for-byte and the source check excludes that one file. It does
not rerun the September17 GPU tests. No new iPhone install is part of the handoff.
The Downloads copy includes the resulting commit/push receipt appended after
commit creation; the repository copy is `native/HANDOFF.md` in that checkpoint.
Use the same working directory to retain all ignored evidence/dependencies.
A remote clone provides source and recipes only, not the user's game or test data.
