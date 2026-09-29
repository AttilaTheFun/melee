# Browser port milestones

The active goal is full original-game execution compiled to Wasm, first offline
with two local players, then online with WebRTC DataChannels and TURN fallback.
The character viewer remains a separate diagnostic; it is not a gameplay milestone.

## 1. Offline two-player

- [x] Compile the full shared game/backend source manifest for Wasm.
- [x] Run Aurora GX with browser WebGPU and correct asynchronous GPU completion.
- [x] Boot from a user-selected local disc without copying/uploading the whole disc.
- [x] Reach a real two-player match; independently control both players with
      keyboard and/or gamepads, finish a match, and rematch.
- [x] Add shared-ring Web Audio output; browser measurements show non-silent audio
      with no reported underruns during the menu/stage-selection test.
- [x] Verify durable importable/exportable browser memory cards: a nonempty card
      survives reload and import/export byte-for-byte, another tab cannot acquire
      the active card, and corrupt import preserves the prior save.
- [ ] Verify frame pacing, loading, memory, pause/resume, and a ten-minute match
      in an actual browser. Preserve native Apple builds and regression checks.

## 2. Online TURN multiplayer

- [x] Implement paired logic-tick inputs, shared match identity and compact
      checksums/desync handling; verify matching snapshots in the tested match.
- [x] Implement WebRTC DataChannel transport and room signaling with server-issued,
      short-lived TURN credentials; validate the Worker with a mocked credential API.
- [x] Two independent browser instances complete a synchronized match over a
      forced local TURN relay, including variable input delivery and cleanup.
- [x] Complete a full local TURN match with actual UDP packet loss and clock skew.
- [ ] Verify Cloudflare credentials/deployment and separate networks.
- [x] Assess rollback against measured delay and deterministic state restoration;
      document actual input-delay/rollback behavior rather than imply equivalence.
- [x] Provide reproducible hosting instructions and local tests. Hosted files
      contain the application only; each player selects their own game disc.

Current work: the latest Mario/Pikachu and Pikachu-mirror soaks each passed more
than eleven minutes and four matches/rematches. Earlier intermittent offline
freezes remain unexplained. Fixed online draw cadence passes a full local TURN
match with packet loss; Cloudflare deployment and separate-network tests remain.
After correcting strict callback signatures, the full game renders a live
Onett versus match with two fighters and an advancing timer. The stage-entry
scenario passed with no browser errors. A complete match (including sudden
death) reached results. Keyboard attacks/jumps, synthetic Gamepad API movement and rematch also pass.
Broader stage/character coverage and unexplained earlier freezes remain.
Both-player actions, pause/resume and multiple ten-minute soaks now pass.
The full offline milestone remains open for performance/stability verification;
the online milestone is not complete.

Foundation evidence: all 1,151 shared game/backend compilation units build with
Emscripten 6.0.9. Wasm alarm, VI, command and JPEG tests pass alongside native
regressions. Headless Chrome passes cross-worker WebGPU resource/completion
and Aurora GX triangle pixel checks (`test-gpu-bridge.mjs`, with
`MELEE_GPU_PROBE=melee_aurora_probe` for GX). These are renderer foundation tests,
not proof of playable gameplay.

Full-runtime checkpoint: `python3 native/web/build_game.py --game` links the
original game. `MELEE_GPU_PROBE=melee_browser MELEE_DISC=/path/to/game.ciso`
with `test-gpu-bridge.mjs` boots via actual local Blob range reads and waits for
120 game frames. `browser-boot.png` confirms the original memory-card prompt
on the canvas, through a GPU-only presentation pass. Wasm global data begins
at 16 MiB so host pointers cannot overlap the game’s separate ARAM offsets.
Two-port keyboard movement and synthetic standard Gamepad API movement pass.
The HTML is a development launcher, not a completed offline milestone.

GPU lifetime regression: the pinned Emdawn implementation released C++ handle
memory before removing its JavaScript registry entry, permitting another worker
to reuse the address prematurely. `tools/prepare_emdawn.py` creates a checksum-
validated local replacement that removes the entry after destruction but before
freeing the address. It leaves the shared SDK cache untouched. The browser GPU
probe passes 6,000 concurrent resource lifetimes and pixel/completion checks.
Repeated full-game boots and a complete match now pass without that handle error;
longer gameplay and device-loss handling remain to be tested.

Stage-entry diagnosis can be replayed with `MELEE_NAVIGATE=1`,
`MELEE_INPUT_FILE=native/web/scenarios/stage-entry.json`, and the full-game
probe variables above. This timing-based keyboard sequence is diagnostic,
not a portable gameplay assertion. `MELEE_EXPECT_MATCH=1` additionally requires
the final read-only snapshot to contain two live fighters in the versus scene.
Each navigation step logs its scene and fighter snapshot, and aborts navigation
on a game error. A boot-only pass does not satisfy the match requirement.

Stage-entry evidence (2026-09-29): `wasm-game-boot-test.log` reaches frame 1514,
mode 2 / scene 2, with Mario and Pikachu live in their idle motions. The canvas
capture `native/build/wasm-renderer/browser-boot.png` shows Onett, both players,
the HUD and a running timer. This is the first live-match rendering checkpoint,
not yet a completed offline two-player milestone. Callback fixes cover background
flash, camera/magnifier/special-display renderers, fighter/item material setup,
and Ready/Go completion; the status callback API now uses its concrete type.

Independent keyboard movement also passed (`wasm-game-controls-test.log`):
P1 moved from x=-47.70 to -42.49 while P2 stayed at x=52.20; P2 then moved
from x=52.20 to 41.26 using its separate key layout. Both fighters remained live,
with no browser errors. Repeat this check with `MELEE_TEST_CONTROLS=1`.
`MELEE_WAIT_RESULTS=360000` optionally waits up to six minutes for the versus
results scene, logging snapshots every ten seconds and failing on game errors.
Native regression after these shared callback fixes: the Mac startup target
built and completed its 1,200-submission `--versus` probe with a nonblack capture.

Full match evidence: `wasm-game-match-test.log` and its result snapshot reach
mode 2 / scene 4 at frame 5665, after stage hazards, KOs and sudden death.
No browser errors were reported; the results canvas rendered a winner.
`wasm-browser-save-test.log` passes a 16,340-byte container with an 8 KB test-file
payload through export/reload/import/reload, rejects a corrupt copy, and confirms
the prior container survives rejection and the following reload. It also verifies
exclusive card ownership across tabs. These tests use a private browser context.

Action coverage: `wasm-game-moves-test.log` observes attack motion 44 and jump
motions 24/25 independently for both players, using their real keyboard mappings.
The navigation fixture now waits for intro/title/main-menu/character-select
transitions rather than assuming loading finishes within fixed wall-clock delays.
A missed title input previously exposed a Great Bay attract-demo callback mismatch
(`Fighter_8006CB94` → the stage hazard callback). Great Bay, Mushroom Kingdom
adventure route and target-test Mario callbacks now accept the third callback
argument; this additional fix builds but needs runtime regression verification.

Rematch diagnosis: results require Start from each human player. The first
rematch test supplied Start only for P1 and therefore correctly remained in
results. The test now supplies both Enter and numpad Enter. Its current full
match/rematch rerun is logged in `wasm-game-rematch-test.log`; inspect that result
before marking the offline match/rematch criterion complete.

Rematch evidence: `wasm-game-rematch-test.log` passes both players’ attacks,
jumps, keyboard movement and synthetic gamepad movement, reaches match results,
then returns through CSS/SSS and starts a second match at frame 6799 with two
live fighters. This establishes the match/rematch criterion, not broad roster
coverage or long-session stability.

Online transport foundation: `netplay/transport.mjs` uses ordered, reliable
WebRTC DataChannels with bounded messages/queues, setup/disconnect timeouts,
SDP/ICE validation and optional relay-only ICE policy. `test-net-transport.mjs`
passed 1,000 packets each direction between isolated real browser contexts,
verified exact ordering/content, rejected a malformed peer packet and cleaned up
both peers. This transport test used host candidates, not TURN. Engine
integration evidence is recorded below.

Lifecycle evidence: `wasm-game-lifecycle-test.log` passes three explicit pause/
resume cycles during a live match. Game frame and fighter snapshots remain
unchanged during each pause, AudioContext is suspended, and simulation/audio
resume. Both players’ attacks and jumps pass afterward. The launcher now offers
Pause/Resume and requests pause when the document becomes hidden; real background
visibility transitions still need browser verification.

Input synchronization foundation: `netplay/inputs.mjs` serializes canonical
float32 controller samples and provides a bounded two-player input-delay queue.
Missing peer input stalls; conflicting duplicates and excessive lookahead fail.
`node native/web/test-net-inputs.mjs` passes 10,000 paired ticks plus invalid,
duplicate and missing-packet cases. The engine integration below uses this
queue at the logic-tick boundary.

Engine synchronization checkpoint: online mode now replaces the raw-input queue
at `lb_800198E0`, once per simulation tick, with the two committed peer samples.
`lb_80019894` initially requested one logic tick per render; the catch-up
checkpoint below supersedes that policy. The game receives a shared seed. Offline alarm-driven input remains unchanged.
The WebRTC session checks build/disc identities, seed, delay and opposite player
slots before starting. `test-net-session.mjs` covers rejected mismatches, paired
ticks, fingerprinting and deliberate desynchronization rejection.

`wasm-net-game-match-test.log` passed two actual Wasm game instances over direct
WebRTC with 2,200 equal per-tick snapshots: menus, Mario/Fox selection, Onett
loading, movement, attacks, jumps and hazard damage. The comparison includes
scene, RNG, cursor/stage selection and fighter state. Runtime checks now exchange
a compact simulation signature every 60 ticks and fail on mismatches or missing
checks; these are diagnostics, not a complete rollback-state serialization.
The expanded signature adds stocks, facing, velocity and grounded state.
The full-match test passed 9,600 matching snapshots through the results screen
with the expanded signature (`wasm-net-full-match-test.log`). Forced TURN,
variable-network tests and performance work remain. That initial implementation advanced one simulation tick per render; the
catch-up checkpoint below addresses simulation pacing. Rendering and wider
validation still need work before release.


Room/launcher checkpoint: `game/index.html` now supports offline, host and join
modes. Online startup fingerprints the local disc, checks the staged build
identity, creates or joins a private room, and completes the peer handshake
before booting the game. Both players start without persistent cards. Leaving
the tab closes the session; independent online pausing is disabled.
`test-room-launcher.mjs` passes the host invitation → join → real WebRTC →
paired inputs flow in two isolated browsers, using a mocked signaling boundary
and stub game entry point. The actual two-engine match test is separate.
`test-room-server.mjs` passes room auth, slot ownership, signaling limits, TURN
credential single-flight/cache, cleanup and headers using boundary doubles.
Neither test establishes deployed Cloudflare or forced-TURN operation.
`stage_site.py` stages nine allowlisted public files, including a build identity;
it excludes game assets, memory cards and test output.

Browser render-queue optimization: four scalar-only WebGPU calls (draw,
indexed draw, viewport, scissor) now use async proxy dispatch. Resource/descriptor
operations and pass End remain synchronous. The GPU probe passes 6,000 handle
lifetimes and 1,000 queued scissored draw sequences with exact pixel readback.
The two-engine test passes 2,200 matching tick snapshots after this change
(`wasm-net-async-profile-test.log`). In sequential headless Chrome 154 runs on
the same Mac, sampled 60-frame averages from match frames 1260–2100 show mean
render-queue waiting falling from 21.891 ms to 17.483 ms (about 20%). Completion
waiting was 4.857 ms before, 5.052 ms after. Samples are wall-clock phase timings
from two concurrent game instances, not GPU timestamps or a broad benchmark;
this does not establish 60 Hz online performance. Detailed data are in
`native/build/wasm-async-profile-comparison.json`.

The optimized build also passes offline two-player keyboard movement, attacks,
jumps, two synthetic standard Gamepad API slots, and three pause/resume cycles
with frozen snapshots and suspended/resumed audio
(`wasm-async-offline-test.log`). The captured Onett frame was visually inspected:
two fighters, stage textures, timer and HUD render correctly. Physical controller
and additional browser/device coverage remain separate checks.

Further browser dispatch work queues pipeline/index-buffer/vertex-buffer setters
as well. These carry only numeric values and owned handles; synchronous registry
deletion and pass End drain their preceding queued uses. The updated GPU probe
uses 1,000 indexed draw sequences and releases pipeline/index-buffer C++ owners
before End, then checks exact pixels. That probe and another 2,200-tick direct
WebRTC match pass. In the same sequential headless comparison, the sampled mean
render-queue wait is now 11.787 ms versus the original 21.891 ms; completion wait
is 4.839 ms. This remains a limited two-instance wall-clock comparison, not a
60 Hz performance claim (`wasm-net-resource-profile-test.log`).

`MELEE_NET_JITTER=1` passes 2,200 matching actual-game tick snapshots with ordered
incoming delivery delayed asymmetrically by 10–65 ms. It models delivery jitter,
not packet loss or real wide-area routing (`wasm-net-jitter-test.log`).
`test-local-turn.py` passes 1,000 packets each way through a real coturn relay
bound to loopback, with relay-only policy and relay candidates asserted on both
peers. Disconnect cleanup passes; the process and temporary credentials are
removed afterward (`wasm-local-turn-test.log`). Cloudflare and separate-network
verification remain outstanding.


Forced-relay full-match evidence: `test-local-turn.py --game --ticks 9600` passes
9,600 matching actual-game logic snapshots through results, with local and
remote candidate types both asserted `relay` for both peers
(`wasm-local-turn-full-match-test.log`). This is loopback coturn, not Cloudflare
or separate physical networks. The 2,200-tick relay scenario also passes.

Cloudflare tooling: Wrangler 4.143.0 accepts the example configuration in deploy
`--dry-run`, including Durable Object, rate-limit and asset bindings. The actual
workerd runtime with SQLite DO storage passes asset/isolation headers, two-slot
WebSocket ownership, signaling, TURN-credential cache single-flight and room
cleanup. The credential API is mocked at the outbound network boundary; the
Worker/DO implementation itself is real (`wasm-room-runtime-test.log`). No
Cloudflare deployment or account-specific configuration has occurred.


Online pacing checkpoint: simulation now follows a stable NTSC deadline and can
use up to three logic updates before the next draw through the original game
loop. Long loading stalls and input waits over one tick rebase the local pacing
clock; they do not accumulate a permanent rendering backlog. Every logic tick
still waits for the canonical paired input. Snapshots/checksums now run at the
end of each logic update, including updates without a draw, rather than at the
render boundary. Offline input timing and native Apple behavior are unchanged.

The first catch-up build passes 2,200 equal snapshots with unequal render counts.
A deliberately asymmetric 1/3 catch-up test also passes all 2,200 snapshots
(`wasm-net-asymmetric-render-test.log`). That older variant lets a blocked peer
retain clock debt; the final clock-rebase variant is tested separately below.
A two-tick cap measured 54.68 ticks/s and failed the requested 55 ticks/s pacing
gate. The three-tick version measured 59.81–59.96 ticks/s, but only 20.27/28.28
draws/s for the two peers. Rebasing after blocked input measured 58.73–58.90
ticks/s with 27.70–28.00 draws/s, passing the 55 ticks/s gate and all 2,200
snapshots (`wasm-net-clock-rebase-test.log`). These are two concurrent headless
Chrome instances on one Mac, not a claim of 60 rendered FPS or broad device
performance. The native macOS startup target still builds after the guarded
logic-boundary hook change.

The final clock-rebase variant also passes 2,200 matching snapshots with
asymmetric 1/3 render catch-up limits (`wasm-net-rebase-asymmetric-test.log`).
That intentionally constrained run is limited by the one-tick peer and is a
render-independence check, not a normal-speed benchmark. Full-match/TURN and
jitter evidence above predates this pacing change and must be repeated against
the new build before treating the updated online milestone as complete.

Input-delay settings: the host can choose 2, 3 (default), 4 or 6 ticks. Room
creation validates a bounded JSON body and stores the delay; both welcome
messages carry it and the peer handshake checks it. Joiners use the host's
value. Launcher and actual workerd tests pass with four ticks, including shared
identity and server validation. Settings lock once startup begins, and the
connected player/delay information remains visible during play.

Pacing/TURN revalidation: the first post-catch-up full-match run stayed matched
but hit its fixed 9,600-tick limit during sudden death, so it failed its results
assertion. The scenario now uses ordinary P1 movement to finish sudden death and
allows at most 6,000 extra ticks, still requiring results. A subsequent full
forced-TURN + ordered 10–65 ms jitter run reached results with all compared
states equal, but failed its 55 ticks/s gate: four ticks of input delay measured
48.84 ticks/s (`wasm-turn-catchup-jitter-full-test.log`). This is not a green
full-performance pass. A six-tick attempt was inconclusive after a long gap in
observations; its timeout is recorded rather than counted as game evidence.

With idle sleep inhibited and monotonic test deadlines, the six-tick forced-TURN
jitter test passes 2,200 matching states and the pacing gate: 59.88/59.96 ticks/s,
29.94/29.72 rendered FPS (`wasm-turn-jitter-delay-six-awake-test.log`). This shorter
run does not replace full-match performance validation with six ticks. The
local relay stopped and temporary credentials were removed after each attempt.

`MELEE_SOAK_MS=600000` now repeats offline complete matches/rematches, exercising
controller input during play and recording Wasm/JS heap sizes after each rematch.
It checks progress, browser errors and excessive Wasm heap growth after warm-up.
The ten-minute run is still pending; adding this option is not test evidence.

Startup-stall diagnosis (2026-09-29): a full run intermittently stopped while
leaving the opening movie. Disassembly of the actual Wasm artifact showed that
`lbMthp_8001F800` loaded the pending-read flag once, then entered an unconditional
infinite loop if it was set. Native movie cleanup now stops new reads and checks
completion under the host interrupt gate, yielding via VI retrace while a read
is pending. `test_movie_stop.c` forces an outstanding read, including entry with
the interrupt gate held. The pre-fix optimized native build hangs in that test;
the fix passes native ASan/UBSan and Wasm, alongside the other five Wasm
foundation tests. Native and browser game builds pass. The fix is committed as
`7e5a3624b`. The browser integration is a development checkpoint, not a completed
or deployed release.

The diagnostic pre-fix build passed five staggered-start relay tests and a full
six-tick-delay, forced-TURN, 10–65 ms ordered-jitter match: 9,600 matching snapshots
through results, 59.64 simulation ticks/s and 30.69–30.79 renders/s on this Mac
(`wasm-turn-phase-full.log`). This establishes the catch-up pacing result but
does not erase the intermittent startup failure. Post-fix full-match and longer
offline validation are tracked separately. Relay processes and credentials are
removed by the harness after every run.

Post-fix startup checks: five more 600-tick real WebRTC runs with ordered jitter
and peer boot offsets 0/250/750/1,250/0 ms all pass against the movie cleanup fix
(`wasm-fixed-startup-1.log` through `-5.log`). The first offline soak completes
one match/rematch and three pause/resume cycles, with zero reported audio
underruns, but then stops advancing at render frame 8,858 during its second match.
It is a failed soak, not ten-minute stability evidence. A renderer process sample
was saved; the next run reports execution phase, heap, visibility and pause state
and fails after 30 seconds without frame progress.

Hosting correction: the previous browser artifact embedded the locally extracted
text atlas. Browser font arrays now begin empty and load from the selected disc's
DOL section table before game startup. Wrong revision, missing/overlapping or
undersized sections, invalid offsets and failed reads are rejected. Synthetic
parser checks pass natively with ASan/UBSan and in Wasm; both atlases read from the
local CISO match the independently extracted DOL bytes exactly. The new artifact
contains none of 4,551 distinct text-atlas byte signatures present in the prior
artifact. The debug atlas was already eliminated from the old linked artifact,
but its browser definition is also runtime-loaded. Browser compilation no longer
includes the generated-font directory. Full-game validation of this new startup
path is in progress; no Cloudflare deployment has occurred.

Clean-checkout verification: detached commit `42b57b3db` builds the full browser
target from a clean worktree with no generated fonts. It reuses only the pinned
Emscripten/Aurora dependencies, not the working tree's uncommitted Apple changes.
The font-loading build also reaches a live match and passes both players'
attack/jump controls after three pause/resume cycles.

The instrumented offline soak completed two matches/rematches, with stable
996,343,808-byte Wasm memory after the first rematch and zero reported audio
underruns. It then stalled at frame 15,026 during the third match and failed the
30-second progress watchdog (`wasm-offline-soak-diagnostic.log`). Native phase 2
places the stall in the input-availability path; the tab was visible and unpaused.
Finer input-alarm and interrupt-state diagnostics are now under test. This is
still an unresolved stability issue, not a ten-minute pass.

Actual packet-loss transport test: `test-local-turn.py --loss-percent 5` forwards
TURN UDP through a loopback impairment proxy, dropping datagrams in both
directions before SCTP recovery. Both peers receive all 1,000 ordered packets
with relay candidates asserted; disconnect cleanup passes. The run dropped
193 of 3,781 datagrams (`wasm-turn-loss-test.log`). All sockets and the relay were
closed afterward. Full-game behavior under packet loss still needs validation.

Native preservation check: the macOS startup target completes 1,200 original-game
GPU submissions in its windowless versus probe and captures a nonblack framebuffer
(`wasm-native-checkpoint.log`, `wasm-native-checkpoint.png`). Native disc/card
regressions also pass. This does not revalidate all Apple device UI features.

TURN loss uncovered a title-screen desync: `gmTitle_801A165C` advanced the random
generator using the local calendar second. Online now derives that entropy from
the agreed session seed; offline behavior is unchanged. A deliberately skewed
peer clock must apply inside pthreads as well as the page. The clean pre-fix
artifact fails at tick 240 with a 17-second worker-clock offset; the fixed build
passes the 600-tick scenario (`wasm-title-worker-skew-before.log`,
`wasm-title-worker-skew-fixed.log`). The earlier page-only offset did not reach
the native clock and is not counted as a skew test.

The fixed game also passes 2,200 matching snapshots through a live match over
forced TURN with 5% configured UDP loss: 898/18,053 datagrams dropped, all sockets
closed afterward (`wasm-turn-loss-game-fixed.log`). The measured short gameplay
window was 55.42/57.29 ticks/s; this is not full-match or wide-area performance
evidence. Another offline diagnostic attempt failed its timed rematch navigation
before reproducing the stall. Rematch now follows live Onett cursor guidance;
the input-alarm investigation continues with that corrected test navigation.

Full post-fix relay/loss/skew validation: both game instances reach results with
9,600 matching snapshots, with a 17-second worker-clock offset and 5% configured
UDP loss. The proxy drops 3,996 of 78,358 datagrams; the relay and proxy sockets
close afterward (`wasm-turn-loss-full-fixed.log`). Simulation measures
57.21–57.23 ticks/s and rendering 27.60–27.77 FPS while an independent offline
soak runs on the same Mac. This is a full-match correctness pass under local
packet loss, not separate-network or Cloudflare evidence. The offline stall
still prevents declaring overall stability complete.

Rollback assessment: this milestone uses input-delay lockstep. A six-tick buffer
adds about 100 ms of scheduled input delay, plus device polling/render latency;
missing reliable-channel packets can stop simulation until retransmission. The
relay/loss tests verify recovery and matching logic, not rollback responsiveness.
Copying the live Wasm heap is not a valid rollback implementation: it also holds
allocator, mutex, worker and resource-handle state, while GPU objects, audio and
file operations live outside it. Rollback needs a deterministic gameplay-state
boundary and restoration tests that reconcile those external systems. The current
compact checksums neither serialize nor prove equality of the complete game state.

The next diagnostic soak completed two matches but failed its second rematch
navigation: full-axis keyboard pulses oscillated around the stage target. This
is a test-driver failure, distinct from the input stall. Selection now uses
small synthetic standard-Gamepad API pulses, still through the normal input path.
The subsequent stack-capture soak has passed two rematches with this driver;
extended stability remains under validation.

The subsequent analog-navigation soak froze before its first rematch, at frame
2,329, phase 6. The input alarm continued completing callbacks while the game
frame stayed fixed (`wasm-input-alarm-soak-analog.log`). This rules out treating
the older phase-2 observation as proof that the input alarm stopped. The watchdog
now captures worker stacks through the existing headless Chrome CDP pipe before
closing the failed test; a busy-worker smoke check verified capture and resume.
The next real-game run is validating that failure diagnostic.

Restart follow-up: `wasm-stack-soak.log` passed 690,635 ms of gameplay, completing
four matches and four rematches with Mario/Pikachu on Onett. Both attack/jump
checks and three pause/resume cycles passed. Wasm memory held at 1,066,139,648
bytes after the first rematch, and audio reported zero underruns. The archived
report is `wasm-stack-soak-result.json`. The earlier intermittent freezes are
still unresolved; this pass does not establish their cause. A targeted Pikachu
mirror test now uses live icon-center deltas because locked-character layouts
reorder icons. Its first attempt used incorrect static offsets and was a driver
failure, not a gameplay failure.

The concurrent direct-WebRTC stress test completed results but failed exact
snapshot comparison for ticks 4,490–4,499 (`wasm-stack-concurrent-online.log`).
One peer's offscreen Mario took an extra damage point and advanced RNG earlier.
`ifMagnify_802FBBDC`, a draw callback, updates `is_offscreen`; fighter logic reads
that flag for offscreen damage. Adaptive catch-up therefore cannot safely omit
draws independently on each peer. Default catch-up is now limited to one logic
tick per draw; larger `MELEE_NET_CATCHUP` values remain diagnostic-only. A full
one-tick comparison passed all 9,600 snapshots through results with the 17-second
clock offset (`wasm-single-tick-render-test.log`), at 30.52 simulation ticks and
draws/s under concurrent offline load. This replaces the earlier pacing claim as the
production policy; performance and broader determinism remain open.

An explicit `MELEE_ASYNC_DRAW=1` diagnostic now tests overlapping game CPU work
with GPU completion instead of unconditionally waiting inside every frame
submission. Original HSD draw-done fences and XFB ownership remain in use; pause
drains outstanding work. The option is disabled by default while validation is
pending. Asynchronous profile output reports render-queue/completion timing as
null rather than attributing a previous frame's timestamp to the current one.

The asynchronous experiment passed 9,600 matching snapshots but only reached
24.8 ticks/draws per second under stress (`wasm-async-draw-net-test.log`); its
code was removed. A fixed two-tick cadence also passed all 9,600 snapshots,
reaching 53.7 simulation ticks and 26.9 draws/s while the asynchronous pair and
offline soak ran concurrently (`wasm-fixed-cadence-net-test.log`). The production
default now uses this fixed cadence, with synchronous GPU submission retained.
It targets 60 simulation ticks/30 draws per second; it does not adapt the draw
schedule independently per peer. `MELEE_NET_DRAW_TICKS=1` retains the slower
one-tick comparison. A new full TURN/loss test of the final default follows.

Final-default relay evidence: `wasm-fixed-cadence-turn-loss.log` passed all 9,600
snapshots through results with a 17-second peer-clock offset and 4,003/78,447
actual UDP datagrams dropped. Simulation measured 57.19–57.22 ticks/s and
rendering 28.59–28.61 draws/s; the offline mirror soak overlapped part of this
test. Relay and impairment sockets closed. The archived full histories are in
`wasm-fixed-cadence-turn-loss-result.json` (ignored build output).

The targeted Pikachu mirror soak passed 691,517 ms, four matches/rematches and
both action/lifecycle checks (`wasm-pikachu-guided-soak.log`). Wasm memory stayed
at 995,753,984 bytes after its first rematch. Audio recorded five underruns while
multiple game tests ran concurrently; it was not a zero-underrun pass. No offline
freeze reproduced in either completed soak. Keep the watchdog capture and the
earlier failures visible rather than declare their cause fixed.

Session protocol 2 folds every observed logic-tick signature into a rolling
checksum, sending it every 60 ticks. The old endpoint-only checksum missed the
ten-tick divergence because states reconverged before the next checkpoint.
Tests now prove detection of a one-tick mismatch followed by matching states,
and reject missing/repeated observations. This remains a compact diagnostic,
not full state serialization or proof of complete game-state equality. Session,
input and room-boundary tests and the real-WebRTC launcher handshake pass; a
two-engine integration check of the new rolling protocol also passed 2,200
matching ticks through live versus gameplay at about 57.7 ticks/s, with skewed
peer clocks (`wasm-rolling-state-net-test.log`).

AddressSanitizer follow-up: `--game --asan` now builds independently with separate
DWARF. Its first successful boot found a global-buffer-overflow in stage setup
(`wasm-asan-split-soak.log`): `it_8026CD50` addressed `it_804A0E50` as one object
past `it_804A0E30`, relying on original BSS adjacency. The native/Wasm path now
uses the actual table symbol; the GameCube path is preserved. A regression using
the real builder, independent globals (`-fno-common`) and native ASan/UBSan fails
before the fix and passes afterward, validating selected item kinds, cumulative
weights and preservation of unrelated globals. It also passes compiled to Wasm.
The full sanitized browser has passed the previously failing stage load and is
running the Pikachu mirror soak. Do not yet equate this defect with the earlier
intermittent freezes; its longer validation is still pending.

The diagnostic build also retains PPC debug-console code optimized away in the
release build. Native printing now omits the SDK's lazy PPC floating-point context
switch while preserving the reports and interrupt gate. It does not fabricate
host CPU contexts. Native startup and browser release builds pass this change.

Post-fix native runtime verification completed 1,200 GPU submissions and a
non-black framebuffer capture (`wasm-item-fix-native-runtime.log`). The fixed
release also completed the full TURN/loss test with protocol-2 rolling checksums:
9,600 matching snapshots, 3,992/78,315 datagrams dropped, about 56.6 ticks/s and
28.3 draws/s under overlapping diagnostic load (`wasm-item-fix-turn-loss.log`).
The ASan run reached match results without another invalid-access report but
exhausted its 2 GiB limit allocating a 64 MiB WebGPU mapped range. The diagnostic
now keeps a 64 MiB quarantine instead of the default 256 MiB; release allocation
is unchanged. Its rerun and a fresh release soak remain in progress.


The bounded-quarantine ASan rerun passed 610,633 ms and three matches/rematches
with zero reported audio underruns (`wasm-asan-bounded-soak.log`). Its heap
reached 1,828,913,152 bytes by the final sample. The release run passed 680,733 ms
and four matches/rematches, stable at 993,394,688 bytes after the first rematch,
with zero reported underruns (`wasm-item-fix-release-soak.log`). Both precede
the following upload optimization; neither establishes the cause of earlier
intermittent freezes.

The browser upload path now uses stable CPU storage per frame slot and queue
writes of used ranges, preserving original GPU capacities. Emdawn previously
copied all 119 MiB of mapped capacity each draw; the tested fight uses roughly
16–17 MiB. The native mapped path is unchanged. Twelve changing-color pixel
readbacks across reused slots and the 6,000-handle/ordered-draw probe pass.
A full offline Pikachu mirror passes both players' attacks/jumps and lifecycle
checks, with a visually inspected game capture (`wasm-upload-offline-check.log`).
A one-draw-per-tick comparison passes 9,600 matching snapshots with 17-second
clock skew, measuring 35.80 ticks/draws per second (`wasm-upload-single-draw-net.log`).
This remains below 60; the production two-tick cadence is retained.

Final upload-path checks pass: the sanitized browser boots into the guided
Pikachu mirror and completes both players' attacks/jumps and pause/resume
(`wasm-upload-asan-check.log`). Native startup completes 1,200 submissions with
a valid framebuffer (`wasm-upload-native-runtime.log`). The production fixed
two-tick cadence passes 9,600 matching snapshots through results over forced
TURN, with 17-second peer-clock skew and 3,992/78,321 datagrams dropped. It
measures 57.91–57.92 simulation ticks/s and 28.95–28.96 draws/s while the
sanitized check overlaps (`wasm-upload-turn-loss.log`). Relay/proxy sockets
closed and temporary credentials were removed. The staged site was refreshed;
Cloudflare deployment and separate-network verification remain outstanding.


Broader stage entry coverage now includes guided Pikachu mirrors on Venom,
Fountain of Dreams and Great Bay. All three pass each player's attack and jump
motion checks plus three pause/resume cycles; captures show the expected stages
(`wasm-stage-{venom,fountain,greatbay}-frame-check.log` and matching PNGs).
These checks are not full matches, extended soaks or complete visual comparisons.

The first Venom action attempt ran during an entry animation. A later attempt
held Player 2's jump key while all 12 samples reported the same game frame.
Worker capture showed the game waiting in `aurora::gx::fifo::wait_draw_done`,
with the completion worker polling its GPU future. This demonstrates a render
wait, not a missing keyboard event or proof of the earlier long-soak freeze's
cause. Action checks now require idle before input and allow simulation to
advance (up to 36 rendered frames / ten seconds), preserving failures with
held-key/frame/fighter samples, worker stacks and a screenshot. The subsequent
Venom check passes; hitching remains a separate performance issue.

Fountain also exposed a false `oioi...` diagnostic on each draw: its original
GameCube pointer check requires the high address bit, which valid Wasm offsets
lack. That diagnostic is now restricted to the original GameCube path. The
actual pointer dereference and rendering logic are unchanged. Native and browser
builds pass. The final Fountain rerun passes both-player action/lifecycle checks
with zero occurrences of the false diagnostic (`wasm-stage-fountain-fixed.log`).
