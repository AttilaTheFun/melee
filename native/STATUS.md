# Native Apple port status

Updated October 3, 2026. The full port is **unfinished**. The coverage below
includes historical passes; broad gameplay reliability and physical-device
performance coverage remain incomplete.

## October 3 expanded play-session diagnostics

- Automatic bounded local session history now records mode/scene/stage transitions,
  lifecycle and pause/resume, graphics settings, controller product categories,
  thermal/low-power state, audio/runtime errors and two-second frame summaries.
  Separate cumulative timing counters avoid consuming the opt-in profiler's data.
- Shader events carry last observed game context. Settings starts with Debugging:
  **Share debugging logs** includes session, shader and existing profiling files;
  **Mark a visual problem** records a timestamp and scene context without an image.
  Stack capture remains opt-in. Session disk I/O runs off the main/game threads.
- Simulator runtime/app compilation and physical-iPhone runtime compilation pass.
  The simulator UI test passed marker/share availability, direct Metal with zero
  readbacks, background/resume and Settings pause/resume. Exported JSON was checked
  for the marker, scene IDs, lifecycle events and advancing frame counters.
  Evidence: `build/session-diagnostics-evidence/`,
  `build/session-diagnostics-ui-retry.log`, `build/session-diagnostics-device-build.log`.
  Xcode stalled finalizing the result bundle after reporting the test passed;
  the retained text log and copied JSON are the validation evidence. The Xcode
  process was stopped and the dedicated simulator shut down.
  No new build was installed on a personal device for this change.
- Details and limits: [PERFORMANCE.md](PERFORMANCE.md). This is session history,
  not automatic texture diagnosis or a crash reporter.

## October 3 Link single-player follow-up

- User completed single-player with Link and confirmed the Xbox controller worked.
  Original single-player game logic disables C-stick attacks; the right stick is
  already mapped to GameCube substick input, so no mapping change was made.
- New export: 57 discoveries, all successfully compiled; 30 waits totaled
  2,469 ms, with a 296 ms maximum. Nine complete configurations were recovered
  from older local captures and added to the bundled seed, now **3,385** entries.
  The remaining 48 cannot be reconstructed from the exported hashes alone.
- Fresh real-Metal validation prepared all 3,385 entries before play, with no
  new discoveries or dropped events; nonblack rendering and pause/resume passed.
  Evidence: `build/link-shader-review/validation/`. This updated seed has not
  been installed on the phone; installed build 2 still bundles 3,376 entries.
- Black roof textures remain unresolved. A fresh Link/Onett windowless fixture
  rendered the roofs normally; the affected scene and device rendering path
  still need reproduction. Evidence: `build/link-shader-review/link-specials.png`.

## October 3 shader learning and Jev simulator coverage

- Shader preparation now includes a bundled portable catalog. The seed at this milestone
  contained 3,376 configurations, including all 40 discoveries in the first phone
  report and all eight in the follow-up. New discoveries still accumulate locally.
- Added a bounded OpenRouter Jev controller, simulator-only structured observation
  and input bridge, route assistance, isolated saves, per-call cost records,
  screenshots, a dedicated-simulator launcher and WAL-safe catalog collection.
  No game outcomes, fighter positions or difficulty are patched by the harness.
- A Jev-driven Kirby playthrough completed Very Easy Adventure with continues
  and reached the staff credits. Policies were refined between resumed runs;
  this is not a fresh uninterrupted replay of the final policy. The run collected
  2,263 configurations before merging earlier captures and reported $0.2517 in
  API charges. Evidence: `build/jev-adventure/report.json` and `completion.json`.
- The expanded seed passed windowless real-Metal validation: all 3,376 entries
  prepared before the first frame, all eight latest phone hashes covered, zero
  new discoveries, zero dropped events, nonblack rendering and pause/resume.
  Evidence: `build/jev-seed-final-verified/`. Simulator timing under stepped
  automation is not a physical-device frame-pacing measurement.
- The final simulator app passed normal startup, direct-Metal presentation with
  no additional CPU readbacks, background/resume and Settings pause/resume with
  the expanded seed (`build/jev-seed-ui.xcresult`). The one-command launcher
  passed setup, one paid gameplay decision, catalog backup and automatic shutdown
  (`build/jev-launcher-smoke/`). No new shaders were discovered in that smoke run.
- A business-signed Release iPhone build with the identical seed is ready at
  `build/jev-device-derived/Build/Products/Release-iphoneos/MeleeNative.app`.
  Signing verification passed and the simulator agent symbols are absent from
  its runtime. On the subsequent user request, build 2 was installed on the
  iPhone 15 Pro with Home Screen name **Melee** and a new app icon. Installed
  metadata confirmed the name/build; the app was not launched. Evidence:
  `build/melee-icon-install.json` and `build/melee-icon-installed-apps.json`.
- Reusable commands and limitations: [JEV_TESTING.md](JEV_TESTING.md) and
  [shaders/README.md](shaders/README.md). Other characters, modes, difficulties,
  random items and graphics settings still need coverage.

## September 28 opt-in hitch profiler

- Added persistent, default-off Performance profiling in Settings, local share
  and clear actions, and rotating logs capped at approximately 10 MB.
- A serial background watchdog samples the still-stalled game thread after a
  50.05 ms frame gap, no more than once per second. Startup/pause/background
  exclusion, safe bounded frame-pointer capture/resume, ARM pointer-auth stripping,
  native timing counters, and UUID-based offline symbolication are implemented.
- Mac windowless sampling tests and Mac/iOS builds pass. Simulator UI testing
  captured injected stalls at the actual stalled function; pause/background and
  disable flows pass. The physical iPhone produced a successful 14-frame stack
  (both Mach capture and resume returned success). Final signed Release installed.
- Details, limitations, commands, and log locations: [PERFORMANCE.md](PERFORMANCE.md).
  Evidence under `native/build/hitch-profile/profiler-*`, `simulator-profile*`,
  and `iphone-profile.jsonl`. The exact final device runtime is retained locally
  as `native/build/hitch-profile/installed-profiler-runtime.dylib`.

## September 28 performance work

- Profiled stage loading and gameplay, optimized sorted archive relocation
  lookups, and changed Metal shader scheduling to defer waits until encoding
  with two compiler workers. Required draws still wait; none are skipped.
- Repeated Mac stage decode/playback checks improved by 5.5–12.1×. Controlled
  shader comparisons reduced total measured shader waits by 45–61%; first-use
  hitches remain and these are not physical iPhone speedup claims.
- Added reproducible profiling and regression checks. See [PERFORMANCE.md](PERFORMANCE.md)
  for measurements, artifacts, limitations, and remaining physical-device testing.

## September 27 iPhone 15 Pro / Backbone installation

- Built and installed on Logan’s iPhone 15 Pro using the requested
  development team and bundle `com.LoganShire.Melee.Port`. Automatic provisioning
  succeeded. The supplied disc was copied into this app’s Documents/Melee.ciso;
  other apps and the older iPhone 13 Pro installation were not modified.
- The game view now hides touch controls and removes their horizontal margins
  when an extended controller occupies player one. Disconnect restores the
  controls, and changing between controller/touch clears held touch input.
  Input-model tests cover connection, disconnection and stale-touch clearing.
- Actual device launch reports `Backbone One; touch controls hidden`. Live
  diagnostics confirm Backbone detection, active audio, direct Metal rendering
  with zero CPU readbacks, and approximately60 FPS in a short startup sample.
  That sample had zero audio underruns; it is not sustained combat evidence or
  a completed physical-button gameplay test.
- The device runtime includes the September20 shader-cache warmup fix. Build,
  signing, installation and startup succeeded. The app was relaunched normally
  without diagnostic flags and left available for the user to play.
- Evidence: `build/iphone15-backbone-{build,sign,launch}.log`,
  `build/backbone-input-test.log`, and `build/iphone15-backbone-evidence/`.
  Signed app: `build/iphone15-backbone-derived/Build/Products/Debug-iphoneos/MeleeNative.app`.
- Project generation now supports `--bundle-id` and `--team-id`; the defaults
  preserve the earlier development identifier. Reproduce this configuration with
  `python3 native/tools/prepare_ios_app_project.py --bundle-id com.LoganShire.Melee.Port --team-id "$MELEE_SIGNING_TEAM"`.
  Device/team values are stored only in ignored `build/local-signing.json`.

### Fullscreen layout and controller settings

- iOS gameplay now occupies the full display with a centered, aspect-preserving
  4:3 image at the maximum fitting size. The header/footer are removed. A settings
  gear lives in the side margin; touch controls still depend on player-one
  controller presence. Actual phone capture: `build/iphone15-fullscreen.png`.
- Settings pause gameplay and expose connected controllers, wireless discovery,
  Bluetooth pairing guidance, saved controller button/trigger mappings and reset,
  and MetalFX Off/2×/3×. Sticks retain movement/C-stick behavior. Mapping preferences
  apply to all extended controllers; profiles per individual controller are not
  implemented. Pairing/forgetting that requires system UI stays in iOS Settings.
- Controller-model tests pass remapping, persistence, analog shield mapping,
  disconnect/touch fallback, and suppression of gameplay input while settings are
  open (`build/fullscreen-input-test.log`). Device signing/install/normal launch
  passed (`build/fullscreen-sign.log`). Wireless pairing itself was not tested.
- Optional experimental 16:9 uses an anamorphic 640×480 framebuffer. The renderer
  scales the complete horizontal projection row by 0.75 before GPU uniform upload;
  the view then presents at 16:9. This expands perspective scene framing without
  stretching characters and centers orthographic content at its original size.
  Raw GX projection registers/camera/game logic remain unchanged, so projection
  save/restore cannot accumulate scaling. Offscreen targets are excluded and
  uniform caches are invalidated when the correction changes across passes.
  This is not a higher-resolution EFB or a complete HUD/camera rewrite: edge
  culling, backgrounds, screen effects, indicators and every stage remain to be
  audited. Settings label the feature experimental and it defaults to Off.
- Fixed a reproduced device settings bug: redundant `@Published` assignments on
  every controller poll rebuilt settings during touches; MetalFX segments were
  enabled but taps failed to select them. Controller names and PAD status now
  publish only on actual changes. Model regression checks 120 identical polls;
  `build/settings-input-test.log` passed. The physical-device test reproduced the
  failure before the fix and passed 2× selection/rendering and return to direct
  Metal afterward (`build/upscale-repro2.xcresult`, `build/upscale-fix.xcresult`).
  Fresh completed 2× output was retrieved from the app's diagnostics container.
- `prepare_ios_app_project.py --ui-tests` includes the opt-in physical-device
  `DeviceGraphicsTests` suite. Supply signing identifiers locally; the user's
  disc must already be installed in this app's Documents directory. The suite
  changes both graphics settings, checks frame progress, verifies zero additional
  readbacks with MetalFX Off, and verifies preferences after relaunch.
- Final physical-device graphics matrix passed (`build/graphics-matrix2.xcresult`):
  2×/3× in 4:3 and 16:9, Off in 16:9, unchanged CPU-readback count with Off,
  saved 16:9+2× after termination/relaunch, then restore 4:3+Off. Both 1280×960
  and 1920×1440 completed GPU images were retrieved; screenshots are in
  `build/graphics-matrix2-attachments/`. Simulator runtime/app compilation passed
  (`build/widescreen-simulator-build.log`); no new Simulator UI run or physical
  iPad run was made for this change. The signed build installed successfully.
- Live device visual check reached a Link-versus-Kirby match on Fountain of
  Dreams in 16:9 using bounded diagnostic pad input; the timer and damage advanced.
  Actual display capture: `build/wide-gameplay.png`. This is a smoke check, not
  exhaustive stage/culling validation. The final app was relaunched normally
  without diagnostic environment or launch-argument overrides; defaults remain
  4:3 and MetalFX Off, with both preferences available in Settings → Graphics.

### Follow-up: reported black screen

- User reported a black screen after the normal launch. The process was alive;
  its game thread was waiting in the runtime pause gate when inspected. That
  observation does not establish the original cause (the app may have been
  backgrounded before inspection). LLDB was detached afterward.
- A temporary CPU-presentation launch produced a valid game framebuffer and
  actual phone screenshot with the Backbone label and no touch controls.
  A subsequent direct-Metal launch also produced a visible title-screen capture.
- The final normal launch, without diagnostic flags or CPU override, visibly
  reached the main menu (`build/iphone15-normal-screen.png`). Actual captures use
  `devicectl device capture screenshot`; no host desktop capture was used.
  `build/iphone15-{fallback,direct}-screen.png` preserve the comparison.
- No source fix was made during this investigation. The original black-screen
  cause remains unresolved; the current successful launch is not proof that an
  intermittent startup/resume problem is fixed. The normal app remains running.

## September 20 continuation

- Reproduced the zero-frame Simulator startup timeout on September 19, under
  Xcode 27.0 / iOS 27.0. The sampled game thread was inside Aurora pipeline-cache
  initialization while the worker compiled Metal shaders. The UI remained live.
  Evidence: `build/direct-metal-handoff-20260919.{log,xcresult,png,sample.txt}`.
- Found a concrete startup issue in the native Aurora patch: its synchronous
  pipeline setting also made background cache loads blocking. Restricting that
  setting to normal draw requests allows cache warmup to run in the background;
  required draws and explicitly blocking requests still wait for their shaders.
- A windowless Mac runtime probe using 1,245 copied pipeline configurations
  completed startup, 180 nonblack frames and pause/resume in 22.76 seconds before
  the fix and 3.91 seconds after. Both started with an empty per-run Dawn disk
  cache; OS/driver caches were not reset. Evidence:
  `build/cache-startup-20260920-{before,after}/`. This is a diagnostic comparison,
  not an iPhone performance result or verification of direct surface presentation.
- `tools/test_cached_startup.py` reproduces the populated-cache runtime check
  without touching the source cache or real saves. Supply `--pipeline-cache`
  pointing to an existing populated `pipeline_cache.db`. Run sequentially with
  other GPU tests. Its timeout is configurable; the refreshed run passed at 20s.
- All five refreshed windowless checks pass with the cache fix: Classic match,
  next round, game-over, rematch and disposable-card fresh-process save/reload.
  The next-round framebuffer was visually inspected. Evidence:
  `build/playable-20260920-cache-fix/report.json`. These are controlled fixtures,
  not ordinary human campaign completion or earned-unlock persistence.
- Mac runtime/probe targets and the Simulator app package build successfully
  with the cache fix (`build/cache-startup-build-20260920.log` and
  `build/cache-startup-simulator-build-20260920.log`). The updated Simulator
  package has not been launched. The signed iPhone candidate still predates this
  cache fix and needs rebuilding before the next device installation.
- PosterBoard crash reports identify the same Simulator, with an Apple
  `PRPosterDescriptor` initialization trap. This is a separate unresolved system
  process failure; fixing game shader warmup does not establish its resolution.
  Simulator testing is paused to avoid disturbing the user's desktop. No device
  was erased. The Simulator was found booted again during the September 20
  inventory and final check, and shut down each time. This continuation issued
  no boot/launch commands; the source of the unexpected restarts is unresolved.
- The test iPhone 13 Pro remains unavailable and the private Mac VM is not on
  the network. The confirmed phone build and rollback bundles remain intact.
- [Browser/Wasm feasibility](WASM_FEASIBILITY.md): substantial reuse is plausible,
  with concrete ABI, scheduling, storage and memory work identified. A bounded
  desktop WebGPU prototype is now implemented as described below; no browser
  gameplay has been implemented or validated.

## September 20 isolated Simulator and browser prototype

- Created `Melee Dedicated iPhone 17 Pro`, iOS 27.0, UUID
  `ECC92EA9-C4F5-47F1-8FF0-0127214DBFFF`. Booted it and installed the refreshed
  native game package; gameplay was not launched in this turn. All Simulator
  operations in this turn used this exact UUID. Other agents' devices were not
  operated. The dedicated device was shut down after testing.
  `tools/melee_simulator.py` selects/creates only this named device.
- [Browser prototype](web/README.md) builds with pinned local Emscripten 6.0.9.
  Native disc/DAT/animation/skinning/texture/visibility code runs in a dedicated
  browser worker. WORKERFS reads ranges of a user-selected File without uploading
  or copying the whole disc. The asset-free static UI implements fighter
  selection, animation playback/scrubbing, orbit and zoom.
- This is an animated Mario/Fox/Peach viewer, **not playable Melee**. The small JS
  WebGPU renderer approximates the native geometry diagnostic; Aurora, full GX,
  gameplay, audio and saves have not been ported. Omitted texture layers are
  disclosed in the UI.
- Wasm tests pass on the actual CISO: finite geometry, valid indices, changed
  animation, all three fighters and return to Mario. Windowless WebGPU tests
  pass with Dawn Node 0.6.1/Metal: shader compilation, all material pipelines,
  pixel readback and distinct animated frames. Rendered Mario, Fox and Peach
  frames were visually inspected. Evidence: `build/web/scene-test.log`,
  `renderer-test.json`, `fighter-*.{ppm,png}`.
- WebKit on the dedicated Simulator passes module worker, WORKERFS File input,
  Wasm animation and character reloads. WebGPU exists but requestAdapter returns
  null; this is **not a GPU/browser rendering pass**. Evidence:
  `build/webkit-test/result.json`. `web/test-webkit.py` reproduces this check
  with private extracted fixtures and closes its temporary server/probe app.
- The prototype's first test harness hit iOS 27's required scene lifecycle trap;
  it was corrected to use UIScene. This was the disposable WebKitProbe app,
  separate from the earlier PosterBoard system-process issue.
- Made the heap header explicitly 32-byte aligned on wasm32 and excluded POSIX
  execinfo/backtrace on Emscripten. Native heap and diagnostics tests pass with
  sanitizers. The Wasm model unit test also passes.
- Desktop Safari remains unverified: the private Mac VM was started headlessly
  but is not reachable over SSH. Host desktop automation is prohibited by the
  Developer AGENTS.md. Use `python3 native/web/serve.py` to try the built
  prototype manually; its server serves only the seven asset-free app files.

## September 17 presentation candidate

- Preserved the previously confirmed source and Mac/iPhone/Simulator bundles in
  `build/baselines/2026-09-17-confirmed/`, with a SHA-256 manifest. The source
  archive excludes ignored build assets, disc data and saves. Extract into a
  separate directory for recovery; do not overwrite the active working tree.
- Default presentation now draws the existing GPU framebuffer directly into a
  Dawn Metal surface backed by CAMetalLayer. This removes CPU pixel readback,
  CGImage construction and image re-upload while direct presentation is active.
  A fullscreen GPU draw and the game's required draw-done synchronization remain.
- MetalFX 2×/3× currently use the existing CPU-backed presentation path. Off
  selects direct Metal. Surface failure falls back to the old path;
  `MELEE_CPU_PRESENTATION=1` explicitly selects it for diagnosis.
- The refreshed windowless Apple input-model regression passes WASD/arrows,
  rebinding/persistence, cancellation, controller slots/disconnect and focus
  cleanup (`build/apple-input-20260917.log`).
- Mac, iPhone and Simulator builds pass. The Simulator frame-clock/lifecycle test
  verifies direct presentation, frame advancement with no additional CPU
  readbacks, background/resume and reopening graphics settings. The graphics
  unavailable-feature test also passes (2 tests, zero failures).
- Evidence: `build/direct-metal-simulator.xcresult`; the actual screen attachment
  `build/direct-metal-attachments/2428058B-43FA-4C17-8DDE-C787BC840B20.png`
  shows frame 295, direct state 1 and zero CPU readbacks.
- Final Mac, Simulator and signed iPhone rebuilds include the animation fix
  (`build/direct-metal-final-{mac,simulator,iphone,signed}.log`). Fresh Simulator
  captures show touch navigation through Main Menu, Classic selection and Mario
  cursor movement (`build/direct-metal-navigation-{6,9,10}.png`). The subsequent
  final UI run was interrupted by a CoreSimulator service version change
  (1171.7 versus expected 1166); it is not a completed gameplay/lifecycle pass.
  A restarted-service retry (`build/direct-metal-service-retry.xcresult`)
  passed the graphics UI test but failed initial frame advancement (sequence
  stayed zero for 20 seconds). A subsequent diagnostic launch found the
  Simulator shut down. Final post-update Simulator validation remains unresolved;
  the earlier two-test direct-frame/lifecycle result is historical passing evidence.
- Direct-Metal XCTest screenshots can be stale even while the game advances.
  Use fresh `simctl io <device> screenshot <path>` captures for visual evidence;
  the app-container legacy framebuffer PNG is also stale in direct mode.
- The signed iPhone candidate has not been installed: the paired test phone's
  connection initially failed; a retry reached it but reported it locked. The
  previously confirmed phone build remains the baseline.
  No physical-device speedup is claimed. Mac window/resize verification awaits
  the locked private VM, and physical controller testing remains outstanding.
- The new round regression exposed an AddressSanitizer out-of-bounds read in
  held-item animation restoration when `anim_id == -1`. The native item-drop
  path now resets the joint instead of restoring a stale tree in that state.
  The failing evidence is `build/playable-20260917/classic-round.log`. The
  fixed rerun passes all five checks: `build/playable-20260917-fixed/report.json`
  (match, round, game-over, rematch and fresh-process save/reload). Match and
  next-round framebuffers were inspected. The save fixture restores rumble=0
  and a fixture-set Luigi unlock; it does not prove earned-unlock persistence.
- `tools/test_playable_baseline.py` provides sequential controlled Classic match,
  round, game-over, rematch and disposable-card fresh-process save checks. These
  are regression fixtures, not certification of normal play through all modes.

## Working and verified

- Native ARM64 macOS runtime renders the original game from the user's disc.
  GPU tests run windowlessly; screenshots below are real framebuffers.
- Configurable WASD/arrows, keyboard rebinding/persistence, cancellation and
  focus cleanup pass input-model tests. Synthetic controller tests cover slots
  and disconnects. Physical keyboard/Bluetooth input remains unverified.
- iPhone and iPad Simulator touch sessions completed combat, pause/resume,
  results and rematch on Onett. Earlier iPhone runs used Ness vs Zelda/Sheik; the refreshed run used Ness vs
  Samus. iPad used Ness vs Fox. These were silent tests preceding the final
  character-mapping fix; updated single-player checks are recorded below.
- Classic: controlled campaign passed every round, Master Hand, credits,
  Congratulations and an unlock encounter. Event previews cover all 51 entries;
  this does not establish completion of the playable events.
- Adventure: continuous controlled progression passed through the Brinstar
  escape/explosion and first Kirby battle. Later initialized checkpoints passed
  Kirby, Star Fox, F-Zero, Onett, Icicle, wireframes, Metal Mario, Bowser and
  Giga Bowser, ending at 600 frames of credits. These use controlled KOs,
  stocks and course placement, not manual traversal or one uninterrupted run.
- Saves: original memory-card pipeline persists in Application Support under
  `MeleeNative/MemoryCardA.card`. A disposable-card, fresh-process regression
  restored rumble=0 and a fixture-set Luigi unlock. Invalid/locked cards report
  an error instead of being replaced. Earned-unlock persistence is unverified.
- MetalFX spatial upscaling: Off (default), 2× and 3×. Actual Mac GPU checks
  passed output size, channels, orientation, fresh frames and completion.
  Simulator's unavailable-feature UI and original-image fallback passed.
  Mac window presentation/latency remains unverified.
- Match-coin crash fixed; bronze/silver/gold articles passed live spawning.
- All-Star: all 13 fights, 12 rest portals and 600 frames of credits passed in
  one controlled replay (25,062 submissions). Two Jigglypuff costume crashes
  are fixed: a Kirby-field alias and four omitted physics descriptors. Both
  affected costumes also pass separate native matches with three live chains.
- Training menu models/animations and both 83-entry language text banks pass
  source-disposal tests. Native Training gameplay, pause menu, speed change and resume now pass.
- All 26 target-course owners pass asset/animation/source-disposal checks.
  Mario's live course clears all ten targets and advances to the next fight.
  Mewtwo also passes the live ten-target clear and next-fight checkpoint. Mario's linear
  path preserves repeated control points and equal cumulative lengths.

- Home-Run Contest: both language stage archives, all 11 model hierarchies,
  text, Sandbag normal-match ownership and both result HUDs pass CPU checks.
  Live human/Sandbag entry, timer expiry and return to character selection
  pass under AddressSanitizer after fixing HUD conversion, adjacent-global
  storage and truncated distance-marker pointers. Record scoring is unverified.

- iPad Classic crash fixed: player lookups now use the explicit character table
  instead of reading past unrelated strings. Every character/secondary-fighter
  mapping passes regression. Real touch navigation passed Mario vs Ice Climbers,
  first-round completion, next team battle, pause and resume on the updated build.

- Updated iPhone Simulator Training passed touch movement/attacks, speed change,
  food spawning, adding a second CPU, resume and Finish back to character select.
  Its MetalFX-unavailable settings and original-image fallback also passed.

## Physical iPhone 13 Pro verification

- Installed on the authorized iPhone 13 Pro (A15, iOS 18.7.8) using personal-team
  development signing. Removed only UniversalUIUITestRunner with explicit user
  approval to free its development slot. The user's other apps were retained.
- The supplied disc is in Melee's own Documents/Melee.ciso. Normal app launch
  discovers it. The original memory-card creation flow and persistence across
  relaunch passed.
- Real device rendering reaches Classic Mario versus Fox, with
  pause/resume. Tethered synthetic pad commands were used; physical touch and
  controller delivery are not established by these checks.
- Fixed excessive item-script memory retention: each state previously retained
  an archive-sized native command arena. It now retains only reachable words,
  preserving branches and fallthrough. The original physical-device Classic
  assertion no longer occurs in the tested match. Full item archive ownership
  and native command-flow regressions pass, including sparse branch graphs.
- Early gameplay samples ranged from 28–55 FPS depending on the scene and
  loading. The latest Classic movement/special/pause samples were about 60 FPS.
  Sustained performance across modes and thermal conditions is not yet verified.
- Found a live audio failure at a disabled loop target installed by Synth at
  stream completion. The decoder now validates the loop address only when
  looping is enabled. CPU regression passes; audio engine and mixer remained active through several minutes of physical-device Classic gameplay. The user subsequently confirmed audible output but reported static/choppy playback; engine activity alone did not establish audio quality.
- Fixed the reported choppy audio: the 48 kHz iPhone route requested up to
  683 source frames per callback, exceeding the old 640-frame queue ceiling.
  Baseline: 39,979 of 639,659 frames missing (937 underruns). The queue now
  buffers 50 ms and adapts upward for larger requests. The same phone/route
  then delivered 10,990,934 frames (343 seconds) with zero missing frames or
  underruns, through intro/menu transitions and Classic Mario versus Ice
  Climbers on Icicle Mountain. Latest gameplay sample: 59.76 FPS. See
  `iphone13-audio-fixed/gameplay/received/state.json`. Sanitized mixer/transport
  and offline Apple conversion/lifecycle tests pass. The signed fixed app is
  installed and reopened normally; the user confirmed that sound is good after this fix.
- Shared immutable archive ownership now avoids repeating serialized bytes for
  each animation state. All 294 asset/animation tests and explicit shared-owner
  lifetime tests pass. Post-change live phone memory footprints were about 1.5–1.6 GB; an
  earlier different-stage run was about 2.22 GB, so this is not a controlled
  performance comparison.
- MetalFX 2× and 3× now produce fresh completed GPU images at 1280×960
  and 1920×1440 on the phone. Fixed early drawable requests before view layout
  and used MTKView's draw callback to prevent duplicate presentation. The updated
  run has no drawable/presentation errors. Persistent allocation failure falls
  back to the original image. Settings are restored to Off after testing.
- Fixed a second stream-completion crash: the stream updater now uses a running
  stereo channel and waits for normal cleanup when both voices have stopped.
  CPU tests cover staggered channel completion, and the physical phone completed
  the full intro into attract-mode gameplay with audio active.
- A capture of the actual iPhone app window includes the touch overlay and
  paused Mario/Fox match (`native/build/iphone13-gameplay-controls.png`).
- Opt-in app-container diagnostics capture actual framebuffer PNGs, completed
  MetalFX output, audio state and memory footprint. The bounded pad fixture
  rejects stale commands after relaunch and requires a fresh post-input frame.

## Refreshed cross-platform regression

- Mac Classic passed 3,800 native GPU submissions with two fighters and a
  nonblack framebuffer after the device fixes (`mac-device-fixes-classic.log`).
- Seven native player/stage checks and all 294 asset/animation checks pass.
- Updated iPhone Simulator passed two UI tests: touch navigation into Classic,
  Fox versus Kirby gameplay, movement/special, pause/resume, and MetalFX
  unavailable settings with original-image fallback (`iphone-device-fixes.xcresult`).
- The updated signed iPhone app is installed and was reopened without test
  environment flags. MetalFX was restored to Off; physical multi-touch and
  speaker audio quality was confirmed good by the user.

## Frame pacing update

- User confirmed audio is good, then reported ongoing uneven motion.
- Profiling found 328 of 2,410 generated frames skipped by UI sampling in a
  40-second baseline, with nominal thermal state. Rendering already uses Metal.
- Replaced the view-owned Combine timer with a persistent display link on iOS.
  The display link now paces the VI worker after input sampling/frame handoff,
  preserving NTSC's 1000/1001 ratio and coalescing delayed pulses. Native game
  callbacks stay off the UI thread. macOS uses a persistent timer.
- Sanitized VI timing/concurrency tests pass, including clock handoff and
  shutdown. Updated macOS, Simulator and iPhone builds succeed.
- Simulator frame-progress checks pass after background/foreground and after
  Graphics settings, alongside the graphics/touch-overlay test:
  `build/iphone-display-paced.xcresult` (two tests, zero failures).
- Final physical iPhone Classic-session sample: 2,398 generated frames, all 2,398
  delivered to the UI across approximately 40 seconds (59.85 FPS), zero skipped
  source frames, no intervals above 33.37 ms, no audio underruns, nominal
  thermal state. This is UI handoff evidence, not measurement of panel scanout.
  `build/iphone13-performance-final.jsonl` preserves the sample before debugger
  attachment; individual game-frame intervals still vary (27 above 20 ms).
  A later actual app-window capture shows first-round results at 55,200 points
  (`build/iphone13-paced-results.png`); the sample is not an isolated combat
  benchmark. The final signed build is installed and reopened normally.
- That installed pacing baseline still uses CPU readback to SwiftUI. The newer
  September 17 direct-Metal candidate is described above.

## Current work / limitations

- Broader Training, Stadium and playable event coverage remains incomplete.
- Complete Adventure/All-Star ending-to-menu and earned-unlock save checks.
- Test actual Mac window resizing/settings, keyboard and controller behavior.
- Physical iPhone multitouch/controllers, sustained performance
  and sustained MetalFX performance across modes remain unverified. Physical iPad is untested.

The isolated Mac VM still shows `loginwindow` (rechecked this session). Its guest
session must be unlocked for Mac window/input tests. The host desktop is not
being driven or captured. Windowless native Metal tests remain available.

## Build locations

- macOS: `build/macosx-game/MeleeNative.app`
- Simulator: `build/iphonesimulator-game/MeleeNative.app`
- iPhone/iPad package: `build/iphoneos-game/MeleeNative.app` (unsigned)
- Signed test iPhone app: `build/ios-device-project/DeviceDerivedData/Build/Products/Debug-iphoneos/MeleeNative.app`
- Device Xcode project: `build/ios-device-project/MeleeNative.xcodeproj`, generated
  by `python3 native/tools/prepare_ios_app_project.py`. Choose a development team
  and connected device for signing. No disc image is bundled.

**Packages refreshed:** `mac-stream-end-build.log`,
`simulator-stream-end-build.log`, and `iphone13-stream-end-build.log` passed.
`iphone13-stream-end-signed.log` passed personal-team device signing, and that
build is installed on the authorized iPhone 13 Pro. Refreshed Mac Classic and
Simulator UI results are recorded above. No disc image is bundled.

Earlier title-only tests sent Start during attract-mode loading. Native tracing
confirmed correct input delivery, but a roughly 26-second loading gap in the
Simulator. The interactive harness now records the initial screen before navigation.
Startup/loading responsiveness remains a limitation.

## Reproducible checks

Run GPU/gameplay/Simulator tests sequentially. Build the matching targets first.

- `python3 native/tools/test_adventure.py icicle` (also `kirby`, `starfox`,
  `fzero`, `onett`, `wireframes`, `finale`).
- `python3 native/tools/test_allstar.py` runs seed 1 and verifies the campaign
  success marker, rather than treating arbitrary process exit as completion.
- `python3 native/tools/test_runtime_save.py` uses and removes a disposable card.
- `python3 native/tools/test_metalfx.py --image PATH_TO_FRAMEBUFFER`
- `python3 native/tools/prepare_ios_ui_tests.py --session-dir DIRECTORY`, followed
  by Xcode UI tests; `ios_control.py` sends simulator-only touch commands.

## Evidence

- `build/mac-player-mapping-classic.log`: final native Mac Classic run passed
  3,800 GPU submissions with live fighters and a nonblack framebuffer; inspected.
- `build/final-diff-check.log`: `git diff --check` passed.

- `build/iphone-player-mapping.xcresult`: both tests passed. Frame 14 is Training,
  20 changed speed, 25 spawned food/two CPUs, 27 resumed gameplay and 33 character
  select after Finish. Graphics fallback attachment inspected.

- `build/ipad-player-mapping.xcresult`: passed, 316.7 seconds. Frame 14 is
  live Classic, 19 first-round completion, 22 pause, 24 resumed team battle.
  `ipad-native-input-trace.xcresult` is the earlier reproduced crash, not a pass.
- `build/native-final-regression.log`: all seven native player/stage checks passed.

- `build/iphone-touch-diagnostic.xcresult`: touch session passed on refreshed
  runtime; frame 14 pause, frame 19 results, frame 25 rematch, all inspected.
  `iphone-final.xcresult` exited cleanly but did not advance title input and
  is **not** counted as a gameplay pass.

- `build/hsd-final-regression.log`: all 294 asset tests passed, 341.11 seconds.

- `build/homerun-current.log`: live entry, timer expiry, CSS return and 5,000
  submissions passed. Earlier missing-HUD/global/pointer crash logs are retained.
- `build/training-current.log`: 5,000 submissions; menu, speed change and resume
  passed. Actual Training-menu framebuffer inspected.
- `build/target-course-mario.log`, `target-course-mewtwo.log`: ten targets cleared
  with original callbacks, next fight and 5,000 submissions passed; frames inspected.
- `build/apple-final-input.log`: keyboard/controller/input model regression passed.

- `build/training-owner-regression.log`: both Training menu archives passed.
  `training-text-{dat,usd}-fixed.log`: both text banks passed.
- `build/hsd-current-regression.log`: 259/259 asset/animation tests passed before
  the additional target-course and spline work; 145.77 seconds.
- `build/target-purin-fixed-regression.log`: 27/27 target/Purin tests passed.
  `hsd-target-spline-regression.log` subsequently passed all 285 tests in
  157.20 seconds, including exact costume parameter comparisons and finite
  matrices over Mario's full 1,800-frame path cycle.
- `build/purin-hat-2.log`, `purin-hat-3.log`: actual native matches passed through
  submission 2800, three live physics chains; both framebuffers inspected.
- `build/allstar-campaign-hat-fixed.log`: failed on omitted costume descriptors;
  this is not a passing campaign. `allstar-seed-1.log` subsequently passed all
  rounds/portals through credits entry; exit 0, actual framebuffer inspected.
- `build/adventure-escape-complete.log`: 15,125 submissions, escape/explosion
  through first Kirby, exit 0; framebuffer inspected.
- `build/adventure-kirby-checkpoint.log`, `adventure-starfox.log`: exit 0;
  reached Fox and Pokémon battle respectively, framebuffers inspected.
- `build/adventure-icicle-entry.log`: historical misname; F-Zero through **Onett**,
  exit 0. Native car pointer/counter integrity verified.
- `build/adventure-onett-icicle-fixed.log`: Onett through 600 frames in Icicle,
  exit 0. `adventure-checkpoint-icicle.log`: timed mountain/encounter through
  wireframe fight, exit 0; both framebuffers inspected.
- `build/adventure-checkpoint-wireframes.log`: wireframes/Metal Mario through
  Bowser entry; `adventure-checkpoint-finale.log`: Giga finale through credits.
  Both exit 0, actual framebuffers inspected.
- `build/ios-gameplay-retry.xcresult`, `ipad-gameplay-retry.xcresult`: complete
  touch sessions passed on earlier packages. `ui-session-retry/frame-29.png`
  and `ipad-session-retry/frame-30.png` show live rematches.
- `build/save-unlock-roundtrip.log`: fresh-process setting/unlock restore and
  nonzero PCM passed. The guarded wrapper rerun `save-final-roundtrip.log` also passes.
- `build/metalfx-current.log`: 2×/3× GPU wrapper passed; image inspected.
- `build/bigblue-route-archive.log`: native F-Zero adapter retains all 38 models;
  unused model 33's invalid PATH animation is deliberately omitted. Generic raw
  playback of that dormant animation still fails and is not counted as passed.
