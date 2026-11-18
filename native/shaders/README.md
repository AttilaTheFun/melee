# Learned pipeline seed

`initial_pipeline_cache.db` contains 3,385 unique Aurora shader/pipeline
configurations captured during development, the user's October 3 iPhone runs,
and a Jev-driven Very Easy Adventure playthrough in the iOS simulator.
It includes all 40 hashes from the first reported run and all eight from the
earlier follow-up run. The subsequent Link single-player report contained 57
additional discoveries; nine were recovered from compatible local catalogs
and added to the seed. The other 48 are not yet bundled. It contains no
game assets, device identifiers, save data, or compiled GPU binaries.

Apple packagers include this database as a resource. Before runtime startup the
app atomically copies it to the GPU cache as `initial_pipeline_cache.db`.
Aurora merges its rows into the existing learned catalog, filters compatible
configuration versions/sizes, and prepares the resulting catalog before play.
The user's learned database and WAL are not replaced. New discoveries continue
to be saved locally. Restarting the app prepares newly learned entries.

To update the seed, capture complete catalog databases (including any WAL), then:

```
python3 native/tools/merge_shader_catalogs.py --output /tmp/new-seed.db \
  native/shaders/initial_pipeline_cache.db /path/to/captured/pipeline_cache.db
```

Review the output, validate it with the real runtime, then replace the bundled
seed. The merger checks database integrity/schema, row sizes and duplicate
configuration conflicts; the renderer checks shader configuration versions.
A shader event log alone does not include enough data to reconstruct a shader.

October 3 expanded-seed validation: a fresh local catalog loaded the bundled
seed, compiled all 3,376 entries before its first frame, and passed nonblack
rendering and pause/resume. There were no new discoveries or dropped log events.
The verified barrier wait was 9.84 seconds on the test Mac; the first probe
of the same seed took 14.18 seconds. These are not iPhone timing claims.
Local evidence is in `native/build/jev-seed-final-verified/`.

The simulator Adventure run reached the clear ending and staff credits using
normal controller input, Kirby, Very Easy and continues. Its learned catalog
contained 2,263 configurations; merging it with the previous seed and an earlier
stage-transition catalog produced the 3,376-entry seed. Policies were refined
between resumed runs; this does not establish unattended success for every
randomized route. See [JEV_TESTING.md](../JEV_TESTING.md) for the reusable harness
and validation commands. Other characters, modes, difficulties, items and
graphics settings can still discover configurations not exercised here.

The expanded seed also passed the normal simulator startup, direct Metal,
zero-additional-readback, background/resume and Settings pause/resume test
(`native/build/jev-seed-ui.xcresult`). The dedicated launcher smoke run reached
gameplay, collected the same 3,376-entry catalog and shut down its simulator.

Earlier 1,333-entry seeded iOS Simulator startup, direct rendering, background/resume and
settings pause/resume regression passed (`shader-seed-ui-retry.xcresult`).
The first run exceeded the former 20-second initial-frame timeout while
compiling; the test now allows 180 seconds only for initial preparation with
the expanded seed.
Subsequent frame-progress waits remain 20 seconds. The business-signed iPhone
build was installed without launching it, preserving the existing app container.

October 3 Link follow-up: the exported log recorded 57 discoveries, all of which
compiled successfully, and 30 waits totaling 2,469 ms (maximum 296 ms). The log
contains hashes, not complete configurations. Nine matching configurations were
recovered from older compatible local captures. A fresh real-Metal probe prepared
all 3,385 entries in 7.54 seconds on the Mac, with zero discoveries or dropped
events, and passed nonblack rendering and pause/resume. Evidence:
`native/build/link-shader-review/validation/`. This seed has not yet been
installed on the phone. Its learned local catalog should prepare all 57 reported
discoveries on the next launch, independently of this bundled seed.

The user also reported black roof textures. A fresh windowless Link/Onett
fixture rendered normal roof colors and discovered no additional configurations.
This does not rule out an iPhone-specific or other-scene issue; the affected
scene is still unidentified and no texture fix has been established.
