# Jev simulator shader coverage

This harness drives the original game's controller inputs in the dedicated iOS
simulator. It selects Adventure, Very Easy, Kirby and five stocks. Jev chooses
combat/platforming actions from structured, read-only observations. The harness
provides context-appropriate action choices and deterministic menu navigation.
Maze navigation tracks visits to the six marker locations and supplies collision-aware
corridor waypoints; it does not know which randomized marker is the exit. Screenshots
are saved locally for review; they are not sent to the model. No disc image,
assets, saves or API key are included in requests or decision logs.

The model is `typesafe/jev-1.13` through OpenRouter's `/api/alpha/decisions` API,
not the chat-completions API. See
<https://openrouter.ai/blog/tutorials/how-to-use-jev/>.

## Run

Export `OPEN_ROUTER_API_KEY` (or `OPENROUTER_API_KEY`) in the invoking shell.
Do not place the value in a command, repository file or run log.
The controller requires Python 3 and Pillow (`python3 -m pip install Pillow`),
plus Xcode with the iOS simulator runtime used by `melee_simulator.py`.

```sh
python3 native/tools/run_jev_simulator.py \
  --disc '/path/to/owned/game.ciso' \
  --run-dir native/build/jev-adventure \
  --max-cost 2 --max-calls 10000 --max-seconds 7200
```

The launcher builds the simulator app, boots only the Melee-owned simulator,
extracts six maze marker coordinates and collision segments from the owned disc (discarding the temporary
archive), installs and launches the test build, waits for shader preparation, runs the
controller, snapshots the learned SQLite database and shuts down its simulator.
Use `--skip-build` only after building the current code. The run directory holds
an isolated memory card under `saves/`; it does not touch phone saves.

By default the game waits between action batches while the model decides.
`--real-time` leaves the game running during network requests. Stepped mode is
for shader coverage, **not performance measurement**: pauses, observation-file
I/O, screenshots and simulator graphics all perturb timing. Recheck frame pacing
on a real device using the normal app and its profiling controls.

The bridge is compiled only into the simulator runtime, enabled by
`MELEE_AGENT_DIR`; stepping additionally requires `MELEE_AGENT_STEP`.
It exposes no network listener and executes no shell commands from model output.
The model can choose only listed controller actions; climb actions are sequences
of ordinary jumps and attack combinations wait for each animation to finish.
Attack combinations stop early when the player is knocked airborne or the scene changes.
Character, stage and combat state are never changed by the
harness. The native character-select helper is read-only.

## Evidence and limits

October 3 development run: Kirby completed Very Easy Adventure with continues,
reached the clear-ending mode, and rendered the staff credits. Policies were
refined between resumed controller processes; a fresh uninterrupted full replay
of the final policy has not been completed. The run made 4,481 model decisions,
reported $0.2517 in API charges, and conservatively reserved another $0.13 for
uncertain/interrupted requests. Local evidence is under `native/build/jev-adventure/`
(`report.json`, `completion.json`, decision logs and screenshots).
During development, an unrelated snapshot harness foregrounded Safari on the
Melee simulator and suspended the game. Its owner confirmed the interference
and moved that harness to a separate simulator. Foregrounding the same Melee
process preserved the playthrough; this interruption was not a game crash.
The status-bar override was cleared, but three Safari tabs and light appearance
remain on the dedicated test device. The device was shut down after testing.
The one-command launcher also passed a separate one-decision smoke run: archive
extraction, app installation, shader preparation, menu/character selection,
paid decision, controller acknowledgement, catalog backup and simulator shutdown.
Its resulting catalog still contained exactly 3,376 entries; no new shaders
were needed during that smoke run. Evidence: `native/build/jev-launcher-smoke/`.
The final simulator build also passed the normal direct-Metal startup and
background/Settings resume regression (`native/build/jev-seed-ui.xcresult`).
The business-signed phone build is at
`native/build/jev-device-derived/Build/Products/Release-iphoneos/MeleeNative.app`;
it contains the identical seed and excludes the simulator agent bridge.
Subsequently, the user requested installation on the iPhone 15 Pro: build 2,
named **Melee** with the new icon, was installed and its metadata verified.
The app was left closed for the user to test.

The learned catalog reached 2,263 entries. Merging with the earlier seed and
stage-transition captures produced 3,376 entries, all verified before the first
frame by `validate_shader_seed.py`; all eight discoveries from the latest phone
log are included. This is shader coverage, not evidence that every gameplay
hitch is fixed or that all characters/modes/graphics settings are covered.

- `decisions-*.jsonl`: model decisions, before/after state and charges. Costs from
  previous invocations in the same directory count toward the total limit.
- `frame-*.png`: simulator screenshots at scene transitions and periodically.
- `state.json`: current game observation and controller acknowledgement.
- `completion.json`: written only on reaching Adventure's clear-ending mode
  (`GM_ADVENTURE_GOVER`, 22), which the original game enters after recording a
  successful clear. Review the screenshot and scene history as well.
- `catalog-*.db`: consistent SQLite backup, including live WAL contents.

A call/time/budget limit, game-over, unknown menu or API error is not completion.
The model is not guaranteed to finish Adventure. Its route and recovery choices
need evaluation; coverage claims must come from captured catalogs/logs. Transient API failures have at most three attempts with backoff; uncertain
requests reserve $0.01 each before retrying. Authentication, billing and malformed
responses stop immediately. The budget is a conservative client-side limit, not a provider
billing cap. Only one controller process may own a run directory at a time.

## Bake learned configurations into the next build

For an already running manual simulator test, snapshot its catalog with:

```sh
python3 native/tools/collect_simulator_shaders.py \
  --device SIMULATOR_UUID --output native/build/learned-catalog.db
python3 native/tools/merge_shader_catalogs.py \
  --output native/build/expanded-seed.db \
  native/shaders/initial_pipeline_cache.db native/build/learned-catalog.db
```

Validate the merged seed using the windowless Metal probe:

```sh
python3 native/tools/validate_shader_seed.py --disc '/path/to/owned/game.ciso' \
  --seed native/build/expanded-seed.db --output native/build/seed-validation \
  --discovery-log '/path/to/exported/shader-events.jsonl'
```

The discovery log is optional; when supplied, every discovery in its latest
session must be present in the seed. The output directory must be new.
The probe must already be built (`native/build/aurora-integration/melee_runtime_probe`).
After validation, replace `native/shaders/initial_pipeline_cache.db`
and rebuild. Always include the previous seed when merging. Do not copy just a
live database file and omit its WAL. This copies portable shader configurations,
not simulator GPU binaries. Each Apple device compiles those configurations for
its own GPU during shader preparation. A warm-start validation should confirm
catalog entries are prepared before gameplay and report any new discoveries.

For a manual session, generate its marker file with `python3 native/tools/jev_maze_route.py native/build/GrNSr.dat RUN_DIRECTORY/maze-markers.json`. The extractor validates the point-table bounds and only accepts the identity transforms used by this disc revision.
