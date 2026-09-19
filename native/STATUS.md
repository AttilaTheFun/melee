# Native Apple port status

Updated September 17, 2026. The full port is **unfinished**. Mac runtime and iPhone/iPad Simulator verification is complete for the
coverage below. Physical iPhone verification is in progress; broader mode coverage remains incomplete.

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
