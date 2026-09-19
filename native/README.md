# Native Apple ARM port

**Status: partial native ARM64 game runtime; the full port is unfinished.**
See [STATUS.md](STATUS.md) for current verified behavior, app build locations,
remaining work and test evidence. The chronological notes below include earlier
limitations that have since been resolved; they are not the current feature list.

The original GameCube build remains the default; native paths use `MELEE_NATIVE`.

Current game app packaging commands (after building the matching Aurora runtime):

```sh
python3 native/tools/build_runtime_app.py
python3 native/tools/build_ios_runtime_app.py --sdk iphonesimulator
python3 native/tools/build_ios_runtime_app.py --sdk iphoneos
```

Outputs are `native/build/macosx-game/MeleeNative.app`,
`native/build/iphonesimulator-game/MeleeNative.app` and
`native/build/iphoneos-game/MeleeNative.app`. The iPhone device app is unsigned
and needs development signing/provisioning. The older build commands below
also cover foundation tests and preview/input apps; those are separate from
the game runtime apps.

## Presentation and repeatable gameplay checks

The default game view presents the GPU framebuffer directly through Metal.
MetalFX remains opt-in and currently uses the older CPU-backed path. For diagnosis,
launch with `MELEE_CPU_PRESENTATION=1` to force that path. See STATUS.md for the
platforms actually tested and the preserved pre-change baseline.

After building `melee_game_startup` and `melee_runtime_probe`, run:

```sh
python3 native/tools/test_playable_baseline.py --disc /path/to/Melee.ciso
```

The runner writes a separate report/log/image directory and uses a disposable
memory card. Run it without other GPU/game/Simulator tests or app packaging in
progress. `--campaigns` additionally runs controlled Classic and All-Star
campaign fixtures; these do not replace manual gameplay coverage.

## Build and validate

Requires Apple Silicon, Xcode, and Python 3. No downloaded third-party build
dependencies are needed for these targets. Run from the repository root:

```sh
make -C native all test
python3 native/tools/test_apple.py
python3 native/tools/build_apple.py
python3 native/tools/build_apple.py --sdk iphonesimulator
python3 native/tools/build_apple.py --sdk iphoneos
```

Apps are written to `native/build/<sdk>/MeleeNative.app`. Mac and simulator
bundles are ad-hoc signed. The iPhone/iPad device bundle is **unsigned** and
requires development signing and provisioning before it can be installed.
It has not been tested on physical iOS hardware.

Validation on this machine: Mac bundle launched in the existing macOS test
VM; iPhone simulator bundle installed and launched on iOS 27 with its landscape
control layout checked by screenshot. The VM's screenshot command failed,
so the Mac window has not been visually verified. Physical keyboard events,
Bluetooth controllers, and simultaneous touchscreen gestures still need
end-to-end testing. The simulator screenshot is generated at
`native/build/iphone-input.png` and is not checked into Git.

The C tests use AddressSanitizer and UndefinedBehaviorSanitizer for archive
parsing and input processing. The game math test links the original
`src/melee/lb/lbvector.c`, using dead stripping to omit functions that still
depend on the unported engine. It does not supply fake engine implementations.
Apple model tests use synthetic GameController profiles to test multiple
controllers and hot unplug; they do not establish compatibility with a real
Bluetooth controller or validate touch gestures.

The HSD integration test executes the original controller queue, button-edge
detection, repeat, stick normalization and rumble command interpreter against
the native `PADRead` backend. It verifies disconnect cleanup, reset-switch
handling, whole-sample synchronization and nested OS critical sections under
ASan/UBSan. The Apple model test additionally sends keyboard and synthetic
GameController values through that same HSD pipeline. The application shows
both raw PAD readings and normalized HSD readings.

The native SDK math backend exports the 28 `PSMTX*`/`PSVEC*` entry points from
the matrix/vector headers. Twenty-three reuse the SDK's existing scalar C
implementations (translation is constructed directly); the five reordered,
skinning and signed-16 array routines implement their assembly's mathematical
operations in C. PowerPC function bodies remain in the repository behind
`#ifndef MELEE_NATIVE`, preserving the original build path. The scalar camera,
projection and light-matrix builders are also linked in the Apple targets.

`test-mtx` checks transform composition, in-place operations, singular/in-place
inversion, inverse transpose, axis/quaternion rotations, camera positioning,
GameCube near/far depth mapping, reordered arrays, two-matrix skinning and
signed-16 conversion under ASan/UBSan. It also executes the original Melee
normalized cross-product routine through the native SDK. This establishes
mathematical behavior, not bit-exact PowerPC floating-point fidelity. Packed
disk components must still be decoded to host endian before calling these APIs.
The native world-to-screen projection scratch matrix now allocates four rows,
matching the SDK's writes; the full world-to-screen path still needs GX support.

The original HSD matrix module is also linked, including SRT construction,
parent-scale compensation, quaternion transforms, inverse operations and its
matrix/vector pools. `test-hsd-mtx` runs the original functions at `-O2` with
ASan/UBSan. It checks a known transformed point, quaternion/Euler agreement,
parent-scale compensation, rotation extraction/reconstruction, inverse-concat
aliasing, inverse-transpose agreement, singular fallbacks and pool accounting.
Native bitwise absolute value uses `memcpy` to preserve signed-zero/NaN bits
without violating C aliasing rules. Joint graph loading and animation
application are still separate unfinished work.

Apple C object files now retain their source directory paths. This keeps
`src/sysdolphin/baselib/mtx.c` distinct from the SDK's `mtx.c` instead of
silently overwriting one module's object during a build.

Game headers preserve the original MSL `bool` ABI as a 32-bit integer through
`Runtime/platform.h`. Host interfaces use `MeleeHostBool` (native `_Bool`) so
including game headers cannot widen the keyboard state or change the Swift/C
boundary. The ABI test checks field sizes, callback compatibility, integer
boolean semantics, and host input calls with game headers included first.

Native effects use a 32-entry `HSD_JObj*` animation queue and an independently
addressed parameter table. They no longer depend on the GameCube linker's
adjacent-global placement or step through pointers in four-byte increments.
The effect parameter test executes the original setters against all eight
slots, updates, and full-table handling under ASan/UBSan. The dispatcher uses
standard native varargs; full effect spawning still requires the game runtime
and assets and has not been exercised.

The stage animation-object search uses a native `jmp_buf`, a full-width
callback argument, and a volatile result that survives `longjmp`. Its search
context is per-call rather than the original global PowerPC register buffer.
`test-animation-search` runs the actual HSD traversal at `-O2` with ASan/UBSan,
checking first-match early exit, child/sibling search, missing matches and
repeated calls. It does not load a stage from an archive.

Particle sort buckets now assert their two-pointer structure rather than a
fixed eight-byte size. `test-particle-sort` links the original particle list
and sorter, checking stable bucket ordering, list boundaries, cached results,
empty lists and singleton lists. Particle asset relocation and drawing remain
unported. Native OS/HSD fatal diagnostics log to stderr and abort; the
GameCube panic display and reset UI are not implemented. Native OSReport
callbacks and host backtraces are described in the diagnostics section below.

The native heap backend manages caller-owned arenas with 32-byte-aligned
allocations, block splitting/coalescing, multiple nonoverlapping heaps and a
mutex around heap operations. The original HSD memory wrappers and object pool
allocator are linked into the Apple builds. Native object pools preserve full
pointer addresses, ensure pointer-sized/aligned free-list entries and reject
zero/overflowing batch counts. The allocator test exercises exhaustion, reuse,
concurrent SDK allocations, alignment through 128 bytes, object count limits,
fixed arena exhaustion, and the original `HSD_AObjAlloc`/`HSD_AObjFree` functions
under ASan/UBSan.

A future game boot must provide a live arena to `OSInitAlloc`, create its heaps,
and select the HSD heap before allocating objects. Resetting/destroying a heap
invalidates its outstanding allocations; object pool descriptors must also be
reinitialized. The platform UI does not yet boot this game heap. The native
backend supplies `HSD_GetHeap`/`HSD_SetHeap` in place of the unported initialization
module; it must not be linked alongside that module or the original SDK heap.
Unused SDK heap extension, fixed allocation, visitation and dump APIs remain
unimplemented. This does not yet load or animate game assets.

A typed animation loader now converts linked, big-endian `HSD_FObjDesc` records
from a validated DAT archive into owned native descriptors. It distinguishes
null pointers from relocated references to offset zero, rejects unresolved
external pointers and descriptor cycles, and copies the compressed command
bytes without swapping them: the original format stores those values in
little-endian order. The stream validator checks opcode/count/value/wait bounds
and the interpreter's 16-bit count and duration limits before loading tracks.
Descriptors and copied streams must outlive any HSD animation objects that
reference them. This loader handles FObj lists, not whole fighter/stage graphs.

`test-animation` passes synthetic archive tracks through the original HSD
allocator, loader, request and interpolation functions with ASan/UBSan. It
checks linear and Hermite interpolation, replay, all five scalar encodings,
negative values, copied-data lifetime, empty tracks and malformed input.
Native scalar parsing avoids undefined signed shifts while retaining the
original signed divisor semantics; empty tracks emit no uninitialized value.
The original FObj and spline modules are linked into all Apple builds.

The typed fighter-motion loader decodes `FigaTree` records, signed per-joint
track counts terminated by -1, 12-byte disk track records, and owned bytecode.
It preserves native pointers and validates every stream before handing it to
HSD. `test-animation` covers conversion, interpolation and malformed fighter
records. `verify-motion` additionally exercises real assets through the
original HSD track interpreter with ASan/UBSan. Fox's `PlFxAJ.dat` passed all
221 motions (16,049 joint records), producing 95,334 finite callbacks at start,
midpoint and end, with no outstanding HSD objects. This is not a visual or
frame-by-frame comparison to GameCube output; applying tracks to scene joints,
rendering and the full fighter runtime remain unfinished. The same sanitized
check subsequently passed all 33 fighter motion bundles: 6,245 motions,
439,224 joint records and 2,846,511 finite callbacks, with no decode failures
or outstanding HSD objects.

```sh
make -C native build/verify-motion
native/build/verify-motion native/build/PlFxAJ.dat
```

The native texture decoder converts all eleven GX sampled color formats to
packed, straight-alpha RGBA8: I4, I8, IA4, IA8, RGB565, RGB5A3, RGBA8, C4,
C8, C14X2 and CMPR. It validates complete tile
sizes, output bounds and palette indices, clips partial edge tiles, preserves
intensity alpha, and handles RGBA8's two planes. One call decodes one mip level.
CMPR uses GX's 5/8–3/8 interpolation and retains averaged RGB for transparent
selectors, following the documented behavior in Dolphin's
[texture decoder](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/TextureDecoder_Generic.cpp)
and [blend helper](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/TextureDecoder_Util.h).

`test-texture` checks format values, palettes, CMPR sub-block ordering,
transparency and clipped tiles under ASan/UBSan. `verify-textures` traverses
joint/drawable/material/texture descriptors in an archive and decodes all mip
levels. It passed Fox (77 material texture references), Mario (60), Kirby (50)
and Peach (81), including actual CMPR, C8 and I4 assets. Game & Watch's tested
costume graph had zero texture references. Shared descriptors are counted once.
This verifies decoding, not material appearance. The model preview below now
uploads and samples these pixels; full TEV emulation and scene rendering remain
unfinished.

```sh
make -C native build/verify-textures
native/build/verify-textures native/build/PlFxNr.dat PlyFox5K_Share_joint
```

The original HSD class, base-object and hash-search modules are linked into
native builds. Class allocation table growth now clears full native pointers
instead of four bytes per entry. `test-class` runs at `-O2` with ASan/UBSan
using poisoned heap backing to expose partially initialized tables. It checks
lazy class initialization, inherited methods, descendant queries, large object
allocation across table growth, object reuse, failed initialization cleanup,
delete callbacks and allocation counters. This establishes base-object
allocation; converting full scene descriptors and loading renderable joints
remain unfinished.



Runtime scene IDs now use a native pointer-width `HSD_IDKey`; the GameCube
build retains 32-bit IDs. The original ID table is linked into the Apple apps.
Joint and animation-reference fields, descriptor registration, envelope/constraint
lookups, and fighter/stage registration preserve the full key instead of
truncating addresses. On-disk descriptors still need explicit conversion.
`test-id` uses same-low-32-bit keys that collide in the original hash buckets,
checking independent lookup, replacement, head/middle/tail removal, missing
entries, stored null values, private tables and pointer-backed object IDs with
ASan/UBSan. Full joint loading and reference resolution remain unverified.

## Controls in the platform app

| GameCube input | Default Mac keys |
| --- | --- |
| Main stick | WASD or arrow keys |
| C stick | H / L / M / O (left / right / down / up) |
| A / B | J / K |
| X / Y | U / I |
| L / R | Q / E |
| Z | Space |
| Start | Return |

Click an action to replace its keys, then press a key. Escape cancels.
Bindings persist in the app's preferences. Restore defaults restores both
WASD and arrow aliases. Key mapping uses Mac virtual key codes, so defaults
describe physical US keyboard positions. Modifier-only keys are not captured.

Paired controllers with Apple's extended gamepad profile are polled at 60 Hz.
The four face buttons map to A/B/X/Y, the right shoulder maps to Z, and triggers
map to analog L/R with a digital press near full travel. Up to four controllers
have stable slots during a session. Keyboard and touch merge with player one.
The iOS test surface provides independent multitouch sticks and buttons;
release, cancellation, backgrounding, and layout changes clear touch state.
The original rumble interpreter produces motor commands through the native
PAD backend. Physical haptic output is not implemented, so the Apple app
advertises no rumble-capable ports. The backend's motor bitmask represents
actual host capabilities, not merely connected controllers.

Host triggers map to Melee's calibrated 0–140 raw range and sticks to ±80,
matching the game boot configuration. This avoids reaching full analog shield
halfway through a host trigger's travel. The original HSD layer performs
normalization and button-edge detection; input is not clamped a second time
through the separate SDK `PADClamp` function.

`melee_pad_publish` atomically supplies four ports to `PADRead`, allowing a
future game thread to consume snapshots independently from the Apple UI.
`melee_hsd_input_frame` currently advances the original HSD processing at the
test app's 60 Hz timer rate. It is a port integration entry point, not a game
frame scheduler. Native `OSDisableInterrupts`/`OSRestoreInterrupts` serialize
cooperating critical sections with the SDK's previous-state nesting semantics;
they do not suspend arbitrary host threads or implement the GameCube scheduler.

The native ARAM backend supplies 16 MiB of separate byte storage with the
original 0x4000 allocation base and LIFO allocator. Game-visible ARAM addresses
remain offsets; main-memory addresses use full native pointers. The original
SDK ARQ scheduler is linked unchanged in its scheduling logic, with a typed
native interrupt adapter and full-width transfer fields. DMA completion is
deferred to a worker under the cooperative interrupt gate, so callers can
finish their critical sections before callbacks run. Reset requires idle DMA;
reset/reinitialize the ARQ scheduler along with its backing memory.

`test-aram` exercises original ARQ chunking, priority preemption between chunks,
callback chaining, ARAM allocation/free and a full-width pointer round trip
under ASan/UBSan. The backend does not emulate the DSP or produce sound. The original HSD DevCom queue is now linked too. Native callback arguments use
`intptr_t`, its relay buffers are aligned to 32 bytes, and queue descriptors
come from the configured HSD heap until the separate audio heap is ported.
Idle DVD handles are closed so an idle queue does not retain the disc mount.
CPU-backed transfers use coherent host memory with ordering fences for the
original cache invalidation/store calls; GPU synchronization is separate.

`test-devcom` passes all eight transfer modes through the original queue:
disc-to-host, disc-to-relay, disc-to-ARAM, host-to-ARAM, ARAM-to-host,
ARAM-to-relay, ARAM-to-ARAM, and clearing ARAM. It covers multi-chunk transfers,
callback chaining and full-width callback data under ASan/UBSan and
ThreadSanitizer. With a disc argument, it also loads and validates Fox's real
costume archive through HSD DevCom and the native DVD/CISO backend.
Cancellation and I/O-error recovery at the HSD queue layer remain unverified.

```sh
native/build/test-devcom /absolute/path/to/game.ciso
```

## Game assets

The user supplied a CISO containing `GALE01`, revision 2. The extracted
`orig/GALE01/sys/main.dol` matches the repository's expected SHA-1,
`08e0bf20134dfcb260699671004527b2d6bb1a45`. Original data stays in ignored
local paths; it is not included in the port's source or application bundle.

The native read-only disc reader supports ISO/GCM and GameCube CISO directly.
It validates the filesystem hierarchy, preserves original entry numbers,
resolves case-insensitive paths and relative directories, and reads across
CISO block boundaries with zero-filled omitted blocks. File reads allow the
original DVD API's trailing padding. The native DVD backend provides open-by-path/entry, synchronous reads, status
queries, and a serial priority queue for asynchronous reads. Callbacks run under
the native interrupt gate after transfer and can chain reads or close a file.
Mounting/unmounting requires no open handles or outstanding requests. Buffers
and file-info storage must remain alive through callbacks. Cancellation is not
yet implemented: closing an active handle returns false instead of canceling
its request. Disc audio streaming, raw drive commands, extracted-directory
mounting and app disc selection are not connected yet.

`test-dvd` checks alignment and range rules, callback deferral through the
interrupt gate, chained requests, priority order, I/O failure and handle
lifetime. It also calls the original `lbFile_8001634C` game function. Passing a
disc path adds a real-data check that reads Fox's costume through the DVD APIs
and validates its archive and joint symbol:

```sh
native/build/test-dvd /absolute/path/to/game.ciso
```

This passes against the supplied CISO under ASan/UBSan. The synthetic
concurrency tests also pass ThreadSanitizer. The HSD queue is tested separately below; the complete game loading/scene loop
still does not boot. Tests cover sparse blocks, padded reads, path lookup,
invalid hierarchies, invalid block maps and truncated images with ASan/UBSan.
CISO container semantics were checked against Dolphin's
[format reader](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CISOBlob.cpp).

```sh
native/build/inspect-disc /absolute/path/to/game.ciso
native/build/inspect-disc /absolute/path/to/game.ciso PlFxNr.dat > native/build/PlFxNr.dat
native/build/verify-disc /absolute/path/to/game.ciso
```

The supplied disc has 1,212 filesystem entries. The scan validates all 861
standalone DAT/USD archives and 33 bundles containing 6,245 archives aligned
to 32-byte boundaries. Real menu assets established that relocation slots can
be unaligned and targets can equal the data section's end. The reader accepts
these forms without casting disk bytes to pointers; scalar reads still require
bytes inside the data section. Regression tests cover both accepted forms,
out-of-range targets and truncated slots.
These results verify container/table parsing, not native object conversion,
rendering or animation playback.

To inspect an extracted HSD archive:

```sh
native/build/inspect-archive /absolute/path/to/file.dat
```

The reader checks table bounds, symbol termination, relocation slots/targets,
and external chains. It retains immutable big-endian disk bytes and exposes
explicit scalar reads and offsets. **It is not a replacement for
`HSD_ArchiveParse` yet.** Game objects still require schema-aware conversion
from 32-bit disk layouts into 64-bit native structures. Reinterpreting disk
data as host structs or writing 64-bit pointers into four-byte relocation
slots would corrupt it. Both synthetic fixtures and the real archive scan above exercise this reader.
Complete game object graphs remain unported.

## Compiler audit

```sh
python3 native/tools/audit.py
python3 native/tools/audit.py --sdk iphoneos
```

The audit compiles all 984 C translation units under `src/melee` and
`src/sysdolphin` for ARM64. It exits nonzero while any fail. Per-file commands,
diagnostics, objects, and a JSON report are saved under
`native/build/audit-<sdk>/`. It excludes the GameCube SDK/runtime, which need
native implementations rather than execution of hardware-register code.

Current results with native scalar and game-boolean ABI handling: **984/984
compile on both macOS and iOS**. That measures only object compilation, not link
completeness, runtime correctness, or port completion. Many compiled objects
still contain pointer truncation warnings and GameCube memory assumptions.

## Owned model assembly and textured Metal preview

`melee_model_decode` assembles joints, polygon streams, skin bindings, materials and
per-drawable part records into one owner. Static costume bytes can be released
after decoding. Animation binding borrows the decoded motion owner; subsequent
steps update poses and skin the existing meshes without reparsing the costume.
Per-part records retain source offsets, polygon flags, joint visibility and
owned material references. Shared polygon chains under separate drawables are retained
as separate parts; cycles in drawable/polygon lists fail cleanly.

Sanitized tests cover ownership, repeated animation updates, shared geometry,
cycle/truncation rejection, unsupported skin types, and cleanup. The optional
real-asset verifier runs 60 updates through the model API for each of the five
base costumes:

```sh
make -C native build/verify-model
native/build/verify-model native/build/PlFxNr.dat PlyFox5K_Share_joint native/build/PlFxAJ.dat
```

`melee_material_decode` owns material colors, texture pixels and mip chains,
sampler settings, and the original texture matrix computation. It preserves
pixel-engine and TEV descriptor fields. Sampler decoding
includes HSD's default minification filter, indexed-texture filter adjustment,
and non-mipmapped fallback. Sanitized tests cover ownership, mip levels, sampler
fields, texture transforms, cycles and malformed descriptors. All five base
costumes pass the 60-frame owned-model verifier with material decoding enabled.

`MetalGeometryRenderer.swift` supplies an offscreen Metal geometry pass with GPU
vertex transformation, depth testing, UV texture sampling, approximate
directional shading and CPU readback. Uploads retain all decoded mip levels and
use the material's wrapping, filtering, LOD and anisotropy settings. The CLI
applies the first supported UV texture per material with the basic HSD color and
alpha mapping operations. It counts unsupported additional layers, reflection
mapping and custom TEV operations rather than claiming to reproduce them.
It is compiled in the macOS and iOS builds but is not connected to the platform
test app's UI or a game scene. The command below opens no windows and writes a
PNG; it requires access to the Metal GPU. A restricted process without that
access reports `unavailable`.

```sh
python3 native/tools/build_renderer.py
native/build/renderer/render-model native/build/PlFxNr.dat PlyFox5K_Share_joint native/build/PlFxAJ.dat 5 native/build/renderer/fox-textured-frame5.png
```

The CLI converts quad/fan topology for Metal, preserving the decomposition used
in Dolphin's [index generator](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/IndexGenerator.cpp).
It waits for successful GPU completion before reading the texture, and rejects
an effectively blank frame. On the Apple M4 Max, Fox's frame 5 produced 35,896
foreground pixels across 418 draws, with 75 uploaded UV textures and two
unsupported reflection-mapped layers. The textured Fox image was inspected;
fur, face, clothing and equipment are recognizable. Earlier untextured checks
also rendered Mario and Kirby; Kirby visibly includes unwanted alternate parts.

These images are **model diagnostics**. They still use approximate lighting and
no face culling. Full TEV behavior, reflection mapping, fighter-specific part
selection, scene transparency ordering and normal render passes are unfinished.
Seeing a posed mesh is not proof of a running game or visual fidelity. No playable
match exists yet.

Synthetic GPU checks open no windows and require no game data:

```sh
python3 native/tools/build_metal_tests.py
native/build/metal-tests/test-metal
```

They check supported color/alpha operations, UV orientation, nearest/linear
sampling, wrapping, mip uploads, LOD clamping and the untextured fallback by
reading actual rendered pixels. Pixel-engine tests also cover all eight alpha
and depth comparisons, the four alpha Boolean operations, all 64 blend-factor
pairs, reverse subtraction, color/alpha write masks, and depth writes before
versus after a rejected alpha test. GPU access is required; these checks are
separate from the default sanitized C tests.

## Original HSD pixel setup and native GX registers

`gx_pixel.c` implements the GX setters used by the original `HSD_SetupPEMode`.
They retain blend factors, color/alpha write masks, depth testing and write
control, early/late depth location, alpha comparisons, destination alpha and
dithering in a native register snapshot. `melee_gx_pixel_material` validates an
optional on-disk PE descriptor, serializes access to HSD's state cache, forces
register updates, and runs that original function. The default sanitized test
checks all eight combinations of HSD's transparency/depth flags, custom PE
fields and stale-cache invalidation. Future game-thread GX calls can use the same
register backend; this is not yet a full GX command processor.

The Metal preview applies these snapshots per draw. It supports disabled,
additive-factor and reverse-subtractive blending; independent RGB/alpha writes;
all GX comparison functions and alpha Boolean operations; and explicit early
fragment tests where HSD requests depth before textures. Blend-factor values
2/3 are interpreted as destination color for the source term and source color
for the destination term, with the corresponding alpha-channel interpretation.
This mapping is cross-checked against Dolphin's
[render-state implementation](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/RenderState.cpp)
and synthetic GPU pixel results. Animated alpha references are shader inputs and
do not create additional pipeline objects.

Fox, Mario and Peach were rendered and inspected with their original HSD pixel
settings: 77, 59 and 85 materials respectively, with one, one and two distinct
pixel states. Mario's textures and Peach's dress details are visible; alternate
parts (including Peach's weapons during idle) still overlap. The preview target
is RGBA8/depth32, not the GameCube EFB: EFB quantization and framebuffer-format
behavior remain unimplemented. The preview explicitly rejects destination-alpha
override, logic blend operations and EFB dithering. Alpha testing quantizes the
current floating-point preview alpha to eight bits; this does not establish
bit-exact TEV arithmetic or original-console visual fidelity.

## Fighter drawable indices and original visibility selection

Owned models now enumerate drawables in the same depth-first joint order as
`ftParts_SetupParts`, including drawables without polygon streams. A part's
`drawable_index` identifies the enclosing DObj rather than an individual PObj.
`melee_model_set_drawable_hidden` combines explicit drawable visibility with
animated joint visibility; animation steps preserve the explicit choice. Tests
cover a branching joint hierarchy, empty drawables, shared geometry, and joint
visibility that cannot be overridden by showing a drawable.

`melee_visibility_decode` converts one costume visibility table from a
`FtPartsDesc` into owned native lookup records. It resolves the original
costume-zero fallback, validates every drawable index, and retains group/choice
ordering. It runs the original `ftParts_80074D7C` and `ftParts_80074B6C` against
owned drawable flag records and native fighter selection fields. These records
are used for selection only; they are not fully loaded HSD scene objects. Table
2 requires its separate low-poly drawable count. A signed-byte selector absent
from the selected table hides that group's entries, matching the original code.

Sanitized tests check archive independence, fallback, repeated selection, absent
choices, index bounds, malformed lists, and unchanged state after rejected API
inputs. Real base-costume table 0 checks pass for Fox (77 drawables/1 group),
Mario (59/1), Kirby (42/2), Peach (85/7), and Game & Watch (116/11). Every available
choice was exercised through the original selector: 43 transitions total,
followed by a return to the initial hidden state.

```sh
make -C native build/verify-visibility
native/build/verify-visibility native/build/PlPe.dat ftDataPeach 85
```

`melee_visibility_command` now unpacks commands 31/32/33 and executes the original
`ftAction` set/restore/clear handlers. Default choices are set through the original
`ftParts` setter. Tests drive these handlers from an owned action program across
timer boundaries and verify the resulting drawable flags, including negative
selectors and invalid indices. `melee_visibility_controls` retains the table's
immutable membership mask so multiple tables can affect a model without showing
unrelated drawables.

The optional fighter-data arguments connect this path to the offscreen preview:

```sh
native/build/renderer/render-model native/build/PlPeNr.dat PlyPeach5K_Share_joint native/build/motion-checks/PlPeAJ.dat 5 native/build/renderer/peach-selected-frame5.png native/build/PlPe.dat ftDataPeach
```

For Mario, Fox, Peach and Game & Watch, the preview runs the original fighter
reset callback, restores its model defaults, and advances the first idle script
to the requested frame. It verifies that the motion name matches the fighter
table's idle entry. Tables 1 and 3 remain hidden while table 0 selects the normal
model, following the normal fighter draw path. The preview reports non-model
events whose effects are not executed. This mode currently accepts base costumes
and frames 0–3600; it does not simulate animation wrapping or game state changes.
Kirby's reset callback needs additional player/copy-ability services and is not
enabled here.

At idle frame 5, this hides 32/77 Fox drawables, 16/59 Mario drawables, 48/85 Peach
drawables and 64/116 Game & Watch drawables. Peach renders without the overlapping
weapons seen in the earlier all-parts diagnostic. Game & Watch executes an actual
model-selection script event, but his rendered shape and appearance remain
visibly incorrect; event execution does not prove rendering fidelity. These are
still offscreen model checks, not a playable scene or an app gameplay loop.

The sanitized owned-model verifier can exercise the same initialization path:

```sh
native/build/verify-model native/build/PlPeNr.dat PlyPeach5K_Share_joint native/build/motion-checks/PlPeAJ.dat native/build/PlPe.dat ftDataPeach
```

## Owned action programs and original script control flow

`melee_action_decode` converts reachable fighter commands into owned native
storage and relocates local call/goto targets. Command lengths come from the
original `ftAction` table. Base timer and control commands are expanded into
native command unions; other event payloads remain explicitly decoded 32-bit
words, rather than being cast into host bitfields. Bad opcodes, truncated
commands, unresolved branches and branches into operands fail decoding.

`melee_action_step` executes commands 0–8 through the original `lbcommand`
routines. The native loop handler no longer addresses pointer slots with
32-bit header arithmetic, and the native return stack retains all five slots
reserved by the original console record. Typed stack checks reject mismatched
returns, underflow and overflow before entering those routines. A per-step
instruction budget detects scripts that never yield. Timer/animation-wrap,
nested-loop, subroutine, pointer-width and counter-wrap tests pass under
ASan/UBSan.

Commands 9–58 require an explicit event callback. An absent or rejecting callback
stops with `MELEE_ACTION_UNHANDLED`; the production API does not silently skip
game effects. The preview connects model commands 31–33 to their original
handlers. The fighter state machine, attacks, other effects and audio remain
unconnected. Default model choices also come from fighter initialization
callbacks; they cannot be inferred solely from the idle script.

```sh
make -C native build/verify-action
native/build/verify-action native/build/PlPe.dat ftDataPeach 2
```

The verifier observes events without executing their game effects. The first
idle scripts (animation index 2) from Fox, Mario, Kirby, Peach and Game & Watch
pass 180 frame advances through their actual timer/call/loop data. This checks
owned command traversal and timing under the supplied frame sequence, not
gameplay or synchronization with a running animation. Peach remains waiting at
the end of that sequence; the other four scripts finish.

## Native skin binding and CPU skinning

`melee_skin_decode` reads a PObj's rigid/shared-joint or envelope references and
weights into owned bindings tied to a native pose. Archive bytes can be released
after decoding. The palette builder follows the original PObj full-weight and
weighted paths, uses the original envelope-node correction and matrix routines,
and produces position and inverse-transpose normal matrices. CPU skinning
transforms the decoded vertices while preserving UVs, colors, and draw order.
It does not normalize or rescale the stored weights.

Tests cover weighted palettes, the full-weight fast path, rigid/shared joints,
node correction, normal transforms, relocated references to offset zero, bad
indices, unsupported shape animation, and unresolved external references.
Animation, joint, and skin decoders now share `melee_archive_pointer` for nullable
pointer resolution instead of maintaining separate copies of that logic.

```sh
make -C native build/verify-skin
native/build/verify-skin native/build/PlFxNr.dat PlyFox5K_Share_joint native/build/PlFxAJ.dat 5
```

The verifier decodes and skins all 46,594 vertices across the five base costumes
at frames 0 and 5 of their first idle motion. Positions and normals remain
finite, with empty AObj/FObj/Vec pools after cleanup. It reports bounds for
inspection; all model parts are included, so these are not visible-character
bounds. No visual equivalence or gameplay result is established by this check.
Camera-facing billboard matrices, shape animation, fighter-specific Z scaling,
reflection texture matrices and part selection remain to be connected. The owned
model and Metal preview above consume these skinned vertices and their materials.

## Native vertex decoding

`melee_vertex_decode` converts fixed-format GX display-list draws into owned
vertex records and draw ranges. It handles direct, 8-bit-indexed, and
16-bit-indexed attributes; positions; XYZ/NBT/NBT3 normals; two color channels;
eight texture coordinates; and nine matrix indices. Big-endian scalars become
native floats/bytes. Attribute declarations are canonicalized into GX order,
and a presence mask is retained for the renderer. Primitive types and draw
boundaries are preserved rather than implicitly triangulated.

Normal integer scaling and the three-index NBT addressing were checked against
Dolphin's [normal vertex loader](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/VideoCommon/VertexLoader_Normal.cpp).
Tests cover signed fixed-point values, packed colors, array bounds, byte order,
attribute ordering, NBT3 offsets, matrix indices, malformed/truncated commands,
and nonfinite floats. Unsupported state commands, nested display lists, VAT
mismatches, and position sentinel indices fail decoding explicitly.

```sh
make -C native build/verify-vertices
native/build/verify-vertices native/build/PlFxNr.dat PlyFox5K_Share_joint
```

The verifier walks the joint/DObj/PObj references and converts the disk vertex
attribute descriptors using the original PObj layout. On the five decoded base
costumes, sanitized decoding passes for 504 polygon objects, 1,719 draws, and
46,594 vertices (Fox 7,602; Mario 6,982; Kirby 8,860; Peach 9,080;
Mr. Game & Watch 14,070). These are unskinned vertex streams, including geometry
for alternate visible parts. The model preview connects material decoding,
envelope binding, primitive conversion and Metal rendering. Shape animation and
fighter-specific part selection remain unfinished.

## Native pose evaluation using the original joint functions

`melee_pose_create` builds owned HSD joint transform records from a decoded joint
graph, in the depth-first order used by `ftParts_SetupParts`. These records are
not registered scene-class objects: use `melee_pose_free`, not HSD scene-object
destruction. Pose evaluation calls the original `JObjUpdateFunc` and
`HSD_JObjMakeMatrix`, including hierarchy propagation and parent scale handling.
Inverse-bind matrices are copied for subsequent skinning. Materials and rendering
payloads remain in the source graph/archive.

`melee_pose_bind` accepts an explicit animation-node-to-pose-index mapping, or an
equal-count preorder mapping. It validates all channels and indices before
replacing a binding. The decoded animation must remain alive while bound.
Supported channels are Euler rotation, translation, scale, and node/branch
visibility. Constraints, custom classes, instance graphs, quaternion animation,
user matrices, matrix independence, and event/path tracks are rejected until
their runtime dependencies are connected. Constraint expression modules are
linked as dependencies of the original joint code; they are not exercised or
claimed native-correct by these tests.

Tests check parent transforms, seeking and fractional playback, explicit mapping,
failed binding preservation, original visibility propagation and small-scale
clamping, unsupported features, and allocation cleanup. Real base-costume pose
verification covers Fox (217 motions), Mario (191), Kirby (300), Peach (197), and
Mr. Game & Watch (192): 1,097 motions over 44,818 frames have finite matrices and
empty AObj/FObj/Vec pools after cleanup. Another 46 entries are rejected because
they use throw-victim or Kirby copy-ability skeletons with different node counts.
They still need their corresponding skeletons and fighter-part mappings.

```sh
make -C native build/verify-poses
native/build/verify-poses native/build/PlFxNr.dat PlyFox5K_Share_joint native/build/PlFxAJ.dat
```

The verifier returns nonzero when any entry is unsupported, and reports accepted
and rejected counts separately. Finite matrix checks establish executable pose
evaluation, not visually correct skinning or original-console equivalence.
There is still no rendered fighter or running match.

## Original animation timeline

The Apple source list includes the original `aobj.c` animation controller and
`lbanim.c` fighter track loader. Native `lbAnim_LoadAObj` exposes the same track
construction and ordering used by `lbAnim_8001E6D8`, independently of joint
attachment. It borrows the decoded FigaTree streams and returns an owned AObj
with no attached HSD object. Track-only users release its tracks using
`HSD_AObjSetFObj(controller, NULL)` before `HSD_AObjFree`; full joint-owning
`HSD_AObjRemove` still requires the joint runtime.

Sanitized tests cover first-frame handling, normal and fractional stepping,
seeking, stopped controllers, loop rewind offsets, jumps spanning multiple
loops, zero speed, update suppression, special track ordering, signed start
frames, and allocator cleanup. The original AObj/FObj implementations perform
the timeline and interpolation calculations.

```sh
make -C native build/verify-motion
native/build/verify-motion native/build/PlFxAJ.dat --timeline
```

The timeline verifier advances every joint controller one frame at a time
through each motion's end and one extra frame. All 33 supplied fighter bundles
pass: 6,245 motions, 439,224 joint records, and 39,297,435 finite update callbacks,
with no outstanding FObj/AObj allocations. This exhaustive mode limits motions
to 100,000 frames to bound verification work. It checks native playback of the
streams, not visual poses, animation-to-skeleton mapping, original-console
numerical equivalence, or gameplay. Joint transforms and the game frame loop
still need to consume these updates.

## Native joint graph decoding

`melee_joint_decode` converts the 64-byte big-endian joint records into owned
native nodes: flags, rotation/scale/translation, class names, optional 3-by-4
inverse-bind matrices, and child/sibling references. Shared references retain
identity. Material/spline/particle payload and constraint references are
preserved as archive offsets for their subsequent schema decoders; these are
not pointers and this graph is not yet an `HSD_JObjLoadJoint` input. Keep the
archive available when resolving those payloads.

The decoder distinguishes relocated offset zero from null, rejects unresolved
external references and malformed/nonfinite transform data, and uses an explicit
stack for cycle validation. Tests exercise shared instance references, owned
storage, truncated records/matrices, cycles, and a 2,048-joint chain that forces
node-array growth without invalidating native references.

Real costume data passes under AddressSanitizer and UndefinedBehaviorSanitizer:
Fox has 73 joints/65 inverse-bind matrices; Mario 61/53; Kirby 46/37;
Peach 114/99; Mr. Game & Watch 53/46. These checks validate decoding, not visible
poses, skinning, material rendering, or numerical fidelity during gameplay.

```sh
make -C native build/verify-joints
native/build/verify-joints native/build/PlFxNr.dat PlyFox5K_Share_joint
```

## Original game-object scheduler

The native source list now includes the original GObj allocation, process-link,
process, graphics-link dispatch, object attachment, and user-data modules.
`make -C native test` runs their ARM64 integration test with optimization and
AddressSanitizer/UndefinedBehaviorSanitizer. It checks process priority/link
ordering, both pause flags, all 64 link-mask bits including bit 63, frame-phase
skips, self-deletion, deletion of the next object, callback insertion, deferred
priority changes, destructor invocation, graphics callback order, and empty
object/process pools after cleanup. The process mask uses an unsigned shift to
avoid undefined behavior at link 63.

`HSD_GObjInitWithHandlers` initializes a fresh scheduler using only explicitly
supplied object destructors. It leaves camera/light/joint/fog kinds unregistered.
The original `HSD_GObj_80391304` still registers those real scene classes; it
requires their implementations before it can be used. Apple builds now strip
unreferenced code so unported graphics callbacks do not force unresolved symbols
into the platform test app. No graphics callback or destructor is replaced with
a no-op. The app has not started executing game scenes; this verifies the
scheduler independently of the unfinished renderer and game loop.

## Remaining work before a playable release

1. Convert remaining game/engine code to native ABI: typed callbacks, varargs,
   pointer arithmetic, command bitfields, static layouts, and libc integration.
2. Decode actual HSD object graphs and animation/command streams into native
   structures, retaining shared references and external symbols.
3. Implement remaining native OS/time/threading, filesystem/DVD, memory/card,
   and audio services. The native matrix/vector backend, PAD snapshots and original
   HSD input path are connected; replace the test timer with the game frame loop.
4. Implement GX rendering semantics on Metal, including vertex formats,
   textures, TEV materials, skinning, display lists, and framebuffer behavior.
5. Link and boot the original scene loop with disc assets, reach menus, load
   fighters/stages, and complete a versus match with graphics and sound.
6. Validate numerical differences against original-game behavior (the native
   reciprocal-square-root implementation is mathematical, not a bit-exact
   PowerPC estimate), then test frame pacing and sustained gameplay.
7. Verify keyboard customization, physical controllers, multitouch, lifecycle,
   and signed builds on real Mac, iPhone, and iPad hardware.

The acceptance criterion is a real Melee match driven by the original game
logic on native ARM, not the platform test app or a substitute fighting game.

## Experimental Aurora GX integration

`native/aurora` is a separate source-level GX compatibility experiment using
[Aurora](https://github.com/encounter/aurora), pinned to
`749d6ee7a22bdfab78c8ece9047bca5d79aa72ca`. Aurora is MIT licensed; its license
remains at `native/build/deps/aurora/LICENSE`. It has not yet been connected to
the Apple app or the model preview. Original HSD scene loading is exercised
separately by the probe below. Existing
builds above do not download or link Aurora.

```sh
python3 native/tools/prepare_aurora.py
cmake -S native/aurora -B native/build/aurora-integration -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build native/build/aurora-integration --target gx_abi_probe hsd_scene_probe --parallel 8
ctest --test-dir native/build/aurora-integration -R '^(gx_abi|hsd_scene)$' --output-on-failure
```

Configuration downloads Aurora's pinned build dependencies, including SDL3 and
Dawn. The probe runs without initializing a window or GPU. On ARM64 macOS it
checks SDK object sizes, 64-bit texture/palette/user-data pointers, canaries,
light/color values and 576 combinations of filters, anisotropy, bias, clamp,
and edge-LOD settings. It does not establish GPU rendering correctness or iOS
backend support.

The repository keeps its SDK headers: `MELEE_NATIVE` plus `MELEE_AURORA`
selects 64-byte texture objects and 40-byte palette objects with 8-byte
alignment. This is necessary because Aurora stores native pointers in these
opaque records. Every translation unit sharing these objects must use the
same defines. Ordinary native builds retain the original 32/12-byte sizes.
The experimental define also directs SDK vertex helper calls to Aurora's
functions. Remaining direct `GXWGFifo` stores in game/HSD sources still need
porting before those paths can execute.

`gx_getters.cpp` supplies missing texture/palette queries using Aurora's own
internal object definitions. `aurora.patch` contains backend fixes and an opt-in offscreen initialization path:
converting signed LOD bias through `int` before `u8`, and correcting the inverse
hardware-filter lookup for the two mixed nearest/linear mip filters. Preparation
accepts only the pinned clean checkout or that exact patch, and configuration
requires the patched revision. The sampler regression checks pass with both
fixes. Do not link `native/src/gx_pixel.c` into an Aurora host, since both provide
GX pixel-state functions.

`hsd_scene_probe` links the original JObj/DObj/PObj/MObj/TObj class methods and
TEV compiler against Aurora. It performs 64 synthetic scene lifecycles, checks
parent transforms and polygon-to-joint reference resolution, and verifies that
all five class instance counts and ID/matrix/vector pool counts return to zero.
The original HSD C code is built with AddressSanitizer and UndefinedBehaviorSanitizer
by default. A native-only fix avoids forming a member pointer through NULL when
the TEV descriptor list is initially empty. The probe does not initialize GX or draw a frame. With no arguments it uses a
synthetic display list that is never submitted; the archive mode below loads
actual fighter data.

The material archive decoder now reads the original 16-byte LOD descriptor:
byte flags at offsets 8/9 and the anisotropy enum at offset 12. Its earlier
20-byte interpretation read incorrect fields. The fixture now checks nonzero
padding and unrelated bytes after the record, independently of the values being
decoded.

### Original scene objects from archives

`native/src/scene.c` owns a copy of the archive and constructs host descriptors
for ordinary joint trees, drawable/polygon chains, rigid and envelope references,
materials, textures, palettes, LOD and TEV data. It then calls the original
`HSD_JObjLoadJoint`, including all class constructors and reference resolution.
`melee_scene_bind` owns decoded FigaTree streams; updates use the original
`HSD_JObjAnimAll`, and destruction releases original objects before their borrowed
descriptors/streams. This path is currently linked only by the Aurora scene probe.

```sh
native/build/aurora-integration/hsd_scene_probe native/build/PlFxNr.dat \
  PlyFox5K_Share_joint native/build/motion-checks/PlFxAJ.dat
```

On ARM64 macOS, Fox (73 joints/77 drawables/91 polygons), Mario (61/59/68),
Kirby (46/42/111), Peach (114/85/105), and Game & Watch (53/116/129) load their
base costumes and run 60 first-idle animation updates under ASan/UBSan. Each
material passes through the original TEV compiler. The probe checks finite
matrices, changed poses and empty ID/matrix/vector/animation pools on teardown.
The caller's input buffers are freed before animation runs.

The decoder validates vertex arrays and display lists, rejects cycles, shared
ordinary joints and excessive recursive depth, and leaves unsupported instance,
constraint, spline, particle, shape-animation, quaternion and user-matrix schemas
explicitly unsupported. Raw texture, vertex and display-list bytes retain console
byte order. Draw recording and offscreen GPU execution are described below;
fighter appearance and the complete game still require further integration.
The existing Apple app and offscreen model renderer still use their earlier paths.

When the five local costume/motion pairs above are present, CMake also registers
`hsd_asset_Fx`, `hsd_asset_Mr`, `hsd_asset_Kb`, `hsd_asset_Pe`, and `hsd_asset_Gw`.
Run `ctest --test-dir native/build/aurora-integration -R '^hsd_' --output-on-failure`
to exercise both synthetic malformed-input checks and real asset lifecycles.

### Original GX draw submission

The scene probe accepts `--record` after its motion-bundle argument. It calls
`GXInit` and the original `HSD_JObjDispAll`, then inspects the queued GX bytes.
It never starts an Aurora frame, drains the FIFO, creates a window, or initializes
a GPU. The five optional asset tests now exercise this recording mode too.
Fox/Mario/Kirby/Peach/Game & Watch emitted respectively
123050/98201/162763/132733/91024 bytes after 60 animation updates. These are draw
submission checks, not rendered frames or visibility-correct fighter previews.

Aurora's `GXSetArray` ABI includes buffer size and endian arguments. The native
SDK declaration and `GXSETARRAY` call sites now match that ABI. Native polygon
vertex descriptors carry the validated indexed-array extent and byte order;
console asset arrays and the game's static byte tables use big-endian mode.
The vertex decoder reports the largest referenced byte offset, including NBT3
vector offsets, so uploads can use the necessary prefix instead of copying the
remainder of an archive for every vertex attribute. The ABI test verifies both
endian modes and all five arguments against the actual serialized GX commands.
The default GameCube/native SDK call remains three arguments.

Sanitizer checks on original draw submission also required a native-only fix
in `HSD_TExpSetReg`: an empty constant list and the final list entry must produce
NULL directly, rather than taking a union member address through NULL.

### Offscreen Metal execution

```sh
native/build/aurora-integration/hsd_scene_probe native/build/PlFxNr.dat \
  PlyFox5K_Share_joint native/build/motion-checks/PlFxAJ.dat \
  --render native/build/aurora-Fx.png
```

`headless_gpu.cpp` initializes a real Metal/Dawn adapter, runs Aurora's GX command
processor and shaders, and reads a 640×480 RGBA texture to PNG. It does not call
`aurora_initialize`, create an SDL window, acquire a presentation surface, or
control the desktop. GPU access may require running outside the restricted
sandbox. Cache files stay in `native/build/aurora-cache`.

The local Aurora patch adds optional offscreen dimensions to the internal GPU
initializer, preserves the ordinary windowed path, and enables synchronous shader
pipeline creation for this deterministic capture mode. The first asynchronous
capture was blank while pipelines were still pending. The harness uses a fixed
perspective camera and an original HSD ambient light. Without a light, the
original diffuse materials produced black pixels. `MELEE_GX_DIAGNOSTIC_TRIANGLE=1`
adds a red test triangle to isolate backend setup; that output is diagnostic,
not a fighter rendering acceptance test.

All five base costumes rendered with their original HSD geometry, TEV materials,
and textures after 60 idle updates on the M4 Max. Actual GPU draw counts were
91/68/111/105/129 for Fox/Mario/Kirby/Peach/Game & Watch. PNGs are
`native/build/aurora-{Fx,Mr,Kb,Pe,Gw}.png`. Fox and Mario are recognizable. Peach
still includes alternate equipment; Kirby's overlapping costume/copy parts and
Game & Watch's white silhouette/extra parts are visibly wrong. This scene path
uses all parts by default; selected-part mode is described below. Material/color
animation remains unfinished. It is
not integrated with the Apple app or a playable game loop, and has not been
executed on iOS hardware.

### Part selection in original scenes

Append `--fighter native/build/PlPe.dat ftDataPeach` to the scene probe's
`--record` or `--render output.png` command to apply fighter model defaults and
first-idle model-selection events. The optional `hsd_selected_*` tests exercise
this path when fighter data is present. Animation names must match the fighter's
idle descriptor; costume zero is currently the only supported diagnostic costume.

`melee_visibility_bind` borrows the actual scene DObjs so the original ftParts
and ftAction model handlers change their hidden flags directly. It preserves
transparency/pass flags and unrelated drawables, checks list size/uniqueness,
and snapshots flags before rebinding to handle reordered pointers safely.
Visibility owners must be destroyed before their borrowed scene objects.

Fox/Mario/Kirby/Peach/Game & Watch hide 32/16/35/48/64 drawables respectively in
this 60-frame diagnostic. Their selected Metal render draw counts are
59/52/33/54/63. Peach's alternate equipment is gone and Kirby's copy parts are
hidden. Kirby's face/material appearance and Game & Watch's white silhouette and
extra geometry still need work. PNGs are `native/build/aurora-selected-*.png`.
The diagnostic explicitly counts every non-model action event it does not
execute; it does not run fighter physics, combat or other gameplay effects.

Kirby now shares a native-only model-default helper with its original OnDeath
routine. The diagnostic calls only that portion (the original two ftParts
setters); it does not pretend that Kirby's player-state or copy-ability reset
has run. The original GameCube OnDeath code remains unchanged.

### Costume expression descriptors

The selected original-HSD scene probe now attaches the costume's
`*_matanim_joint` tree when present. `melee_scene_bind_materials` builds owned
native material/texture animation descriptors, image tables, palette tables,
and copied FObj streams from the scene's archive. It currently accepts image
and palette selection channels, rejecting other material animation channels
and render-animation descriptors. It validates tree/list bounds and texture
IDs before calling `HSD_JObjAddAnimAll`; failed attachment rolls back its
allocations without changing the live scene. Only one attachment is supported.
Native HSD texture updates check animated table indices before dereferencing.

The diagnostic reads the costume-zero expression-control indices from fighter
data and sets their AObj rates to zero, matching `ftAnim_80070200`. It still
counts and skips non-model action commands, including expression changes; this
is not a full fighter animation controller. The integration tests exercise
all integer expression frames through original HSD after freeing input buffers,
then reset to frame zero. Fox/Mario/Kirby/Peach each have two controls; Game &
Watch has no costume material-animation tree. Repeated malformed-tree rejection
and subsequent valid attachment are also checked under sanitizers.

Windowless Metal renders passed for all four textured costumes and are saved
as `native/build/aurora-expressions-{Fx,Mr,Kb,Pe}.png`. Kirby is still shown from
the side, so these renders do not establish correct facial appearance. Game &
Watch's special width/color setup remains missing. The Apple app still needs
the original rendering path and gameplay loop integrated; these checks do not
establish playability or physical iOS support.

### Game & Watch base appearance

The selected-scene diagnostic now decodes Game & Watch's fifth visibility
lookup from `ftData->x48_items[10]`. The original OnLoad uses this as table 4;
normal fighter drawing hides it. Its 39 meshes were previously left visible,
which caused the extra objects and sleep letters. The explicit lookup API
`melee_visibility_decode_lookup` shares the standard table decoder and its
bounds checks. Unit tests cover selection and invalid table dimensions/indices.

For this diagnostic, an owned Fighter context supplies the decoded costume-zero
diffuse color, width, and common model scale to original `ftMaterial_800BFB4C`
and `Fighter_UpdateModelScale`. The latter is reapplied after motion updates,
as the width changes the root's X scale. It does not initialize a complete
fighter or run OnLoad's item setup. Outline color is decoded but the original
outline/material draw passes are not yet connected.

The resulting windowless Metal image is
`native/build/aurora-gamewatch-appearance.png`: a recognizable black Game &
Watch silhouette without the extra meshes. The scene hides 103/116 drawables
and the readback contains 7,164 non-background pixels. This supersedes the
earlier Game & Watch white-silhouette and extra-geometry diagnostics, but is
still a single isolated idle render rather than gameplay.

### Native stock HUD state addressing

`ifstock.c` now uses its typed `x204[player]` animation array in native builds.
The GameCube path's fixed `0x204` base offset is invalid after preceding HSD
pointers widen. Native assertions preserve the scalar animation record's byte
layout while the console keeps its original whole-object size assertions.

The `hsd_stock` integration test executes the original stock-steal function
with original `Player_SetStocks`/`Player_GetStocks` and real HSD joint instances.
It checks both animation slots, occupied-slot and zero-stock rejection,
trajectory coordinates, neighboring player state, and preservation of joint
pointers above 4 GiB. It also verifies the typed record access used by the HUD
update callback. It does not execute the full HUD update/draw callback or a
match. The test accesses the module's private state by including the original
source in a test translation unit, without a production test accessor.

Both macOS and iOS compiler audits now compile 945/984 modules; 39 still fail.
This count is not a full-game link or playability result. The original-HSD/GX
integration suite now contains 13 passing tests, including the stock test.

### Memory-card ABI

Native public CARD declarations use `s32`/`u32` for the twelve entry points
whose console declarations used `long`/`unsigned long`. In particular,
`CARDWriteAsync` now accepts `CARDCallback`, matching the original HSD caller's
32-bit channel/result signature on ARM64. Console declarations are retained
under the non-native branch. `test-game-abi` checks the exact native function
pointer types and the scalar `CARDFileInfo`/`CARDStat` layouts.

This corrects interfaces for a future native storage backend; it does not
implement memory-card persistence or make the console CARD hardware driver
safe to execute. The macOS and iOS compiler audits now compile 946/984 modules,
with 38 still failing, and the native regression suite passes.

### Training HUD host state and original fonts

Training mode's `CssSubStruct` is live state containing native HSD pointers;
its console-only `0x204` size check is retained for GameCube, while native
builds check the actual array cardinalities. All its accesses already use
named fields. The `hsd_training` test runs original `fn_80188D3C` on real HSD
joints/AObjs for values 0 through 999, checking digit/blank-frame selection and
preservation of all 39 native joint pointers and animation-state entries.
The original-HSD/GX integration suite now contains 14 passing tests. This does
not run the complete training menu or a match.

`python3 native/tools/prepare_fonts.py` extracts the text and debug atlases
from the user's `orig/GALE01/sys/main.dol`, requiring the known GALE01 revision-2
SHA-1. It translates the addresses in the checked-in symbol map through the
DOL section table and writes atomic generated includes, raw files, and a hash
manifest under ignored `native/build/generated`. No game data is downloaded
or added to tracked source. Compiler audits prepare these files automatically
when the DOL is present.

`python3 native/tools/verify_fonts.py` compiles the original HSD font objects
and compares their 154,112 bytes against the DOL: 287 text glyphs and 128 debug
glyphs. The native test target includes section-boundary/malformed-section
tests and, when the DOL exists, the original-font byte check. Font rendering
and UI integration remain to be verified separately.

With training host-state checks and generated fonts available, both full ARM64
compiler audits pass 949/984 modules (35 still fail). Native regression tests,
font byte verification, and all 14 original-HSD/GX integration tests pass.

### Mute City effect bookkeeping

The native `grMuteCity_801F2AB0` now returns an explicit zero/nonzero marker.
The verified DOL shows allocation failures leaving zero in r3, while the
successful path calls `Ground_801C0498` at 0x801F2B14 and leaves its r3 value
unchanged through return. That function loads `stage_info.param` into r3 at
0x801C04A0 and only changes f1 afterwards. Both callers test the marker for
zero; neither dereferences it. The host therefore returns zero for allocation
failure or missing stage parameters and one otherwise. The console path is
unchanged. This replaces undefined native C behavior with the observed use of
the original result, without trying to return a truncated host pointer.

The car state also stores its `x24` game-object reference as `HSD_GObj*` in
native builds, removing integer truncation on storage and presence checks.
`test-mutecity` runs the original setup function under ASan/UBSan with controlled
allocation/scale test doubles, exercising failure, existing-transform, scaling,
flag, and marker behavior. It checks full-width car object storage separately.
It does not execute actual particle spawning or a playable Mute City stage.

After the Mute City repair, both ARM64 audits compile 950/984 modules (34
still fail), and the native regression suite passes. This remains a compiler
and component-runtime milestone, not a full-game link or playability result.

### Shared trophy archive host layout

`ToyED8Data` and its `ToyGlobalsS_`, `tyLightData`, and `TyArchiveData` overlays
now share verified native offsets for their archive/object fields. The native
background-archive field is a full-width pointer. Relational assertions check
that the views agree, while the original console offset assertions remain in
the GameCube path. Native views explicitly use Clang's aliasing attribute.

The owner now allocates and zeros `ToyArchiveStorage`, a union large enough
for every view. A sanitizer test caught that aligning the prefixes alone was
insufficient: accessing a larger view over the old smaller allocation violates
native object-size requirements. The cleanup path also uses the named archive
field instead of the console's `void**[0x14]` index.

`test-toy-layout` checks shared pointer writes, preservation of neighboring
joint/object fields, and a boundary canary under ASan/UBSan at `-O3` with strict
aliasing. This repairs the shared archive state; other trophy globals and
pseudo-object/raw-offset accesses still need runtime work. It does not prove
that trophy menus work. Both ARM64 audits now compile 980/984 modules; the four
remaining failures are console diagnostics and JPEG error-handling modules.

### Host JPEG error handling and output bounds

The three original JPEG modules now use `HSD_SETJMP`/`HSD_LONGJMP`. On native
Apple targets these execute the host C library operations directly at the
call site; they never save or restore PowerPC registers. The jump-state union
retains the original 248-byte reserved prefix, with compile-time checks that
the host state fits. The console macros retain the original runtime calls.

The native encoder workspace has one consistent union definition. The decoder
workspace is explicitly aligned and includes the host structure's tail padding;
its scalar work area remains at offset `0x118`. The output byte/block writers
use full-width cursor arithmetic with checked subtraction, reject invalid
capacities and oversized writes, and retain the original block writer's
reserved final byte. A zero-length write avoids passing NULL to memcpy.

`test-jpeg-runtime` exercises real host jumps and original output-writer code
under ASan/UBSan, checking overflow recovery, exact-capacity behavior, invalid
inputs, pointer preservation above 4 GiB, and untouched workspace/output guards.
This is not a full JPEG codec validation: the remaining endian, raw-access,
and codec behavior still need runtime work before use with image data.

Both ARM64 audits now compile 983/984 modules. The remaining failure is the
original console diagnostics module; the current native targets use their
separate fatal-diagnostics backend. This count still does not prove a full-game
link or playable runtime. Native regression tests pass.


## Native report callbacks and fatal backtraces

The original `debug.c` now has a native branch, linked by the Apple host,
component tests, and Aurora scene probe. `HSD_LogInit` enables callbacks for
formatted `OSReport` bytes without accessing the console C library's private
`FILE` fields. It does not intercept arbitrary host stdout writes. Reports are
also written to stderr; long messages use allocated storage, and allocation
failure emits the bounded stack-buffer prefix. Callback dispatch is serialized
and suppresses recursive callback delivery while allowing nested stderr output.

Native panic callbacks receive an explicit `HSD_NativePanicContext` containing
source location, message, and host stack addresses, never a fabricated PowerPC
`OSContext`. Returning from a panic callback still aborts. Panic text is passed
as data to avoid interpreting percent specifiers. The native `dberror.c` branch
clears host floating-point exception flags and installs the original HSD log
ring plus a host backtrace reporter. It does not install GameCube exception
handlers or display the console crash screen.

`make -C native test-diagnostics` checks formatted and long reports, embedded
null bytes, repeated initialization, callback disabling and recursion, the real
HSD ring buffer, floating-point exception clearing, full-width stack addresses,
and fatal termination after a returning panic callback. It runs with ASan and
UBSan. Both full ARM64 compiler audits now pass 984/984 modules; the macOS app
and unsigned iOS app build, and the component regressions pass. These are
porting milestones, not evidence of a linked or playable full game.

## Full-game link audit and native startup arena

After running `audit.py` and `build_apple.py` for the same SDK, run
`python3 native/tools/audit_link.py` (or add `--sdk iphoneos`). This attempts a
real executable link using the original game entry point, all 984 audited
objects, and the existing native/portable SDK backends. It does not run the
result. The command exits nonzero for an incomplete link and writes the actual
linker output and command to `native/build/link-<sdk>/`. Missing or source-stale
objects are rejected; rerun both prerequisite builds after header changes too.
No placeholder symbols are supplied. Aurora is not part of this audit because
its extended GX ABI requires compiling the full game with that configuration.

The original SDK `OSArena.c` now supplies a native implementation of the six
arena get/set/allocation functions. The host supplies caller-owned bounds;
initialization of the host's overall game memory remains separate work. Low
allocations round their start and end upward, high allocations round downward,
and both preserve full host pointers. Invalid alignment, arithmetic overflow,
and insufficient space return NULL without changing the bounds. Mutexes
serialize reservations. Setting the two initial bounds must precede consumers.

`make -C native test-arena` checks two-ended reservations above 4 GiB, alignments
from 1 to 1024 bytes, exact exhaustion, insufficient alignment padding, untouched
boundary canaries, and handing the remaining region to the existing native heap
for a real allocation/free. It uses ASan/UBSan. Both Apple app builds and the
component regressions pass with this SDK module included.

The first real link attempt reported 229 unresolved symbols on each SDK after
including the arena backend. Fifteen were file-local inline helpers whose C11
external-inline definitions supplied no out-of-line body. Their definitions now
have native-only internal linkage; each helper's references were checked to be
confined to its defining source file. Both audits still compile 984/984 modules,
both Apple builds and component regressions pass, and the new link attempts
report 214 unresolved symbols on each SDK with those fifteen helpers resolved.
These remaining dependencies include graphics, audio, clocks/alarms, video,
CARD storage, movie decoding, and other SDK/runtime services. This link attempt
does not include Aurora and must not be interpreted as the total amount of
remaining porting work; runtime data layouts and hardware assumptions remain
beyond the missing symbols.

## Native tick clock and original calendar conversion

`time_backend.c` supplies `OSGetTime` and the wrapping 32-bit `OSGetTick`.
It samples the host UTC date once at initialization and advances that value
using the host monotonic clock, expressed in ticks since 2000-01-01. Later wall
clock corrections therefore do not reverse game timers. This is a clock source;
frame scheduling, suspend/resume policy, and OS alarm delivery remain separate
integration work.

Native clock macros no longer dereference GameCube low memory. They retain the
162 MHz bus / 486 MHz CPU rates and bus/4 tick unit, consistent with
[Dolphin's hardware clock definitions](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/HW/SystemTimers.h).
These constants describe the original game's units, not the ARM CPU frequency.
The SDK's original Gregorian calendar conversion code is compiled separately
from its PowerPC timebase instructions. Native guards reject unsupported dates
before year zero and conversion overflow; wide intermediates prevent signed
arithmetic overflow when processing calendar fields.

`make -C native test-time` exercises the original game `lbTime` date helpers,
2000 epoch, dates before the epoch, century and 400-year leap rules, month
normalization, millisecond/microsecond conversion, year zero and maximum tick
round-trips, a current UTC wall-time comparison, monotonic reads, low-word tick
behavior, and elapsed time. It runs with ASan/UBSan. Both Apple builds and both
984/984 compiler audits pass. The time functions resolve in both full-game link
audits; the game still has unresolved platform services and is not playable.

## Explicit compiler-runtime conversion and floating constants

The original training-mode speed selection explicitly calls `__cvt_dbl_usll`.
`runtime_backend.c` now implements that helper from the PowerPC body in
`src/Runtime/runtime.c`. Despite its unsigned return type, the original routine
truncates toward zero and saturates to signed 64-bit extrema on overflow, with
negative results represented in two's complement. NaNs and infinities follow
the original sign-dependent saturation. The native implementation decodes the
double bits and uses bounded integer shifts, avoiding undefined out-of-range
host casts. This ports that helper only; it does not link empty bodies for the
remaining compiler runtime assembly routines.

The native branch of `src/MSL/float.c` defines float objects with the original
`0x7FFFFFFF` NaN and `0x7F800000` infinity bits, matching the game callers' types.
Both modules are included in the Apple builds and Aurora integration. The full
link audit now includes configured sources outside the game/engine audit, so
these MSL definitions are checked too.

`make -C native test-runtime` checks exact constant payloads, the original game
`fn_801855BC` on negative/zero/positive inputs, conversion boundary cases,
infinities, signed NaNs, subnormals, and more than 40,000 finite conversions
against a host-cast oracle restricted to its defined range. ASan/UBSan pass.
Both Apple builds and the component regressions pass. Both full-game link
attempts now report 208 unresolved symbols, with all three runtime/constant
symbols resolved. Full gameplay is still neither linked nor verified.

## Native OS alarms

`alarm_backend.c` supplies alarm initialization, creation, relative/absolute and
periodic scheduling, cancellation, and queue validation. A condition-variable
worker waits on the native tick clock. Queue mutations and callback delivery
use the existing cooperative interrupt gate; the queue mutex is released before
calling game code. Equal deadlines retain insertion order. Periodic deadlines
stay aligned to their original start and skip elapsed periods, following the
SDK behavior. A periodic alarm is reinserted before its callback so that the
callback can cancel or reschedule it. One-shot callbacks can recreate their
alarm without the worker touching it afterward.

Callbacks receive a NULL context, not fabricated PowerPC registers. The three
original game alarm callbacks were checked to ignore that argument. The input
sampling callback now has a native wrapper with the exact OSAlarmHandler
signature, avoiding a call through an incompatible function pointer. This does
not yet run the full game's frame or movie loop. The interrupt gate serializes
cooperating critical sections; unprotected shared state in original callers
still needs runtime review before those subsystems are enabled.

`melee_native_alarm_shutdown()` cancels queued alarms and joins the worker for
host teardown. Call it only after stopping game work, outside callbacks and
interrupt-protected sections. Initialization can then restart the worker.

`make -C native test-alarm` checks equal-deadline order, delayed delivery while
interrupts are disabled, cancellation before delivery and during an in-flight
callback, periodic phase alignment/self-cancellation, one-shot rearming,
shutdown, and restart. Both ASan/UBSan and a separate ThreadSanitizer build pass.
The full component regressions, both Apple builds, and both 984/984 compiler
audits pass. Both full-game link attempts now report 203 unresolved symbols,
with the five previously unresolved alarm entry points resolved. Playability
remains unverified and the full game is still not linked.

## Original alarm-driven input timer integration

The native PAD backend now accepts the specification selected by the game's
startup (`PAD_SPEC_5`) and reports it through `PADGetSpec`. The host supplies
already-decoded specification-5 PADStatus values; unsupported historical wire
formats fail explicitly. `PADSetSamplingRate` records the original clamped
0..11 SI interval request. It does not throttle host events or emulate the SI
bus: host events update the latest four-port snapshot, and the game's own
alarm determines when HSD reads it. The requested value is available through
`melee_pad_sampling_rate` for integration checks.

`make -C native test-input-timer` links and executes the original
`lb_80019AAC` / `lb_80019628` timer setup, its alarm callback, the original
controller/rumble code, and the original card/snapshot polling functions. A
single explicit test double for CARDProbe reports no attached card; no native
save implementation is implied. The test publishes decoded input, waits for
real native alarm delivery into the original HSD raw queue, and advances the
original master/game-status updates. It checks button press/hold/release,
positive and negative stick endpoints, changing the game timer from its
normal interval to 8 ms, cancellation, and the PAD configuration contract.

Both ASan/UBSan and a separate ThreadSanitizer build pass this integration test.
Both Apple builds and the full component regressions pass. Full-game link
audits now report 201 unresolved symbols on each SDK, with all referenced PAD
symbols resolved. The Apple test app still uses its existing direct input
preview loop; full original game-loop integration, saved-card storage, and
playability remain unfinished.

## Native startup arena and original HSD heap initialization

`os_boot_backend.c` implements idempotent `OSInit` with a cleared, 32-byte-aligned
64 MiB host allocation and initializes the native clock. The allocation has
process lifetime. Physical/simulated memory-size queries report this software
arena's actual capacity; they do not describe the Mac's RAM or map console
physical addresses. The larger arena allows room for native pointer growth,
but does not by itself validate the game's memory requirements.

The native `OSRoundUp32B` and `OSRoundDown32B` macros now preserve uintptr_t
addresses. The original HSD memory stage uses the native heap backend's selected
heap instead of maintaining competing HSD_GetHeap/HSD_SetHeap definitions.
Native framebuffer reservation rejects invalid counts/dimensions, computes
sizes without signed multiplication overflow, and the FIFO allocator falls
back to the current heap when the arena has been consumed.

`make -C native test-boot` calls the private original HSD_OSInit memory stage
through a test translation unit including `initialize.c`; it does not call the
full HSD_InitComponent graphics/video initialization. It checks host addresses
above 4 GiB, alignment, idempotent OS initialization, original two-framebuffer
and FIFO reservations, separate audio/game heaps, memory accounting, allocations
from both heaps, and the post-arena FIFO allocation path. The real synth module
supplies its audio-heap global; no fake OS or heap functions are linked.

ASan/UBSan and the full component regressions pass, as do both Apple builds and
both 984/984 compiler audits. Full-game link attempts now report 198 unresolved
symbols per SDK, with OSInit and both memory-size queries resolved. The full
engine startup, rendering/audio integration, and gameplay remain unfinished.

## Full original game linked against Aurora: macOS audit

`python3 native/tools/audit_aurora_link.py` builds the explicitly requested
`melee_game_link_audit` CMake target in the configured Aurora integration build.
It compiles every game/engine translation unit with `MELEE_NATIVE` and
`MELEE_AURORA`, plus native/portable backends, and attempts an executable link
against the pinned Aurora GX/VI libraries. It excludes the isolated CPU
`gx_pixel.c` register-capture backend because Aurora supplies those GX symbols.
Generated fonts come from the verified local DOL. No placeholder functions or
ignore-unresolved-symbol linker options are supplied.

The target is excluded from normal builds and is not a CTest test. The audit
never executes it: the original entry point still lacks native engine/renderer
lifecycle integration and would not be a verified game even after linking.
The tool saves `full-game-link.log` and `full-game-link.json` in the build
directory, including the command, unresolved symbols, and explicit untested
runtime status. It removes any stale executable before attempting the link.
This configuration currently covers macOS, not iOS.

All original game/engine files compile with this graphics ABI. The first link
attempt reported 107 unresolved symbols, compared with 198 in the separate
non-Aurora audit. This difference reflects inclusion of the actual graphics
backend, not implementation of 91 new services. The compatibility layer now
supplies `GXInitFogAdjTable` using the original SDK float algorithm with native
input/overflow guards. The GX ABI test checks its table size against Aurora and
verifies orthographic/perspective geometric fixtures. All 14 GX/HSD scene tests
pass with the updated compatibility library.

The Aurora link now reports 106 unresolved symbols. Five are GX services:
GXNtsc480Prog, GXSetCopyClamp, GXSetMisc, GXSetTevClampMode, and GXWaitDrawDone.
Other missing services include audio, card storage, video retrace/presentation,
movie decoding, settings/reset, and debug I/O. Runtime correctness of resolved
symbols, full game data relocation, and actual gameplay remain separate,
unverified requirements.

## GX miscellaneous state and progressive configuration

The Aurora compatibility layer now implements GXSetMisc against the pinned
backend's actual shadow state. XF flush configuration updates its vertex-count,
BP, and dirty-state fields; display-list context configuration controls the
existing Aurora save/restore implementation. Changing that setting during list
recording and unknown tokens fail explicitly. The backend already handles the
console dummy-primitive flush without sending unnecessary native draws.

GXNtsc480Prog is restored from the original SDK configuration. Its complete
60-byte big-endian representation was compared with the user's verified DOL at
0x804011E0, including sampling positions and vertical filter. GXSetTevClampMode
preserves the release game's empty behavior: the DOL's entire four-byte routine
at 0x80340518 is 0x4e800020 (PowerPC blr), consistent with the SDK's debug-only
unsupported-hardware assertion. This is verified original behavior, not a
placeholder for a missing native effect.

The new `gx_misc` test exercises actual Aurora display-list recording and
checks that state is restored with context preservation enabled and retained
when disabled. It also checks XF configuration, the original no-effect TEV
operation, and every progressive-mode field. It creates no window or GPU.
Run `ctest --test-dir native/build/aurora-integration -R '^(gx_abi|gx_misc|hsd_)'
--output-on-failure` for the 15 port integration tests. A broader `gx_` pattern
also selects optional upstream tests that are not built by these probe targets.

The full macOS Aurora link audit now reports 103 unresolved symbols. Two GX
entry points remain: GXSetCopyClamp and GXWaitDrawDone. Draw waiting is not
replaced with a no-op or a FIFO-only drain: Aurora dispatched its
callback on command processing at the time of this audit. The later
GPU draw-done section below replaces that callback path; the game's presentation
path remains incomplete. Full gameplay remains unlinked/unverified.

## Submitted-work GPU completion and frame-resource reuse

`gpu_completion.cpp` adds an explicit Dawn queue-completion fence. It first
drains the CPU render worker so that queued submissions exist, then waits for
OnSubmittedWorkDone through WaitAny. Aurora's existing gpu_synchronize only
waits for its CPU worker; it is not treated as GPU completion. Callback status
is heap-owned so that timeout/cancellation cannot retain a caller stack
reference. The readback callback now follows the same lifetime rule.

The windowless Metal harness uses this fence before readback. Setting
`MELEE_GPU_REPEAT_FRAMES=4` with the existing `--render` scene command submits
four frames of the same owned pose, checks GPU completion between submissions,
and reuses Aurora's frame slots. This is a frame-resource test, not an extra
animation loop or the full game's simulation. The test exposed a first-frame
versus later-frame clear-color mismatch, now fixed by setting consistent host
and GX clear state.

The selected Fox asset passed one-frame and four-frame Metal runs with 59 draws
per frame and 12,916 non-background pixels. Their PNGs are byte-identical
(SHA1 36cbd063f48ea213b6626ac7eda055fd2bc0e337), and the rendered model was
visually inspected. Outputs are `native/build/gpu-completion-fox-1frames.png`
and `native/build/gpu-completion-fox-4frames.png`; logs record confirmed queue
completion. All 15 existing port integration tests also pass. These tests
create no window, acquire no presentation surface, and send no input events.

This helper waits for submitted work only. It does not close an active frame,
implement GXWaitDrawDone, or defer the original HSD callback until presentation
work is submitted and finished. Those lifecycle connections remain necessary;
the full-game link and gameplay remain incomplete.

## Native HSD framebuffer copy addresses and handoff tests

The original HSD video module's bottom-half antialias copy now uses uintptr_t
for its padded-row offset and destination address in native builds. It no
longer discards the high 32 bits of an XFB pointer. Native guards reject missing
buffers, zero dimensions, antialias heights without room for the overlap,
widths beyond the fixed overlap scratch buffer, and address addition overflow.
The width bound is checked before the SDK's 16-bit padding macro, which would
otherwise wrap a width of 65535 to zero.

`make -C native test-video` includes the original video.c and captures its GX/VI
calls with explicit test doubles. It checks whole-frame, top-half, and
bottom-half copies at addresses above 4 GiB, including a 623-pixel image whose
row stride pads to 624 pixels. It checks overlap scratch copies and rejection
of invalid width/height. It also executes original HSD initialization,
draw-enable selection, WAITDONE/NEXT/DRAWDONE transitions, and pre/post-retrace
handoff through two buffers. Completion callbacks are manually delivered by
the test; it verifies HSD state logic, not real GPU/VI presentation.

ASan/UBSan and the component regressions pass. The full Aurora link still has
103 unresolved symbols: this fixes runtime address correctness rather than
adding a missing symbol. Aurora's GXCopyDisp remains an empty implementation;
copy/presentation and GPU-completion callback integration are still required
before this video path can display the actual game.

### Persistent native system preferences

`settings_backend.c` implements the original `OSGet/SetSoundMode` and
`OSGet/SetProgressiveMode` SRAM calls. The Apple host configures
`Application Support/MeleeNative/system-settings.bin` before input startup.
A missing file starts with stereo enabled and the progressive preference off;
the latter is the game's saved preference, not the native display's capability.
The two one-bit values use an explicit versioned eight-byte format, serialized
access, and an fsynced temporary file followed by atomic replacement. This is
separate from memory-card saves. It does not implement sound mixing or display
mode changes. Only one game process should own a settings file.

`melee_settings_open` rejects malformed, truncated, oversized or unknown-version
files without changing the active settings. A host must configure a writable
path before the game writes preferences; an unconfigured or failed write raises
an explicit panic because the original setters have no error return. The
preview app displays initialization errors. The storage path can only be changed
while the game is stopped. Atomic replacement protects normal process restart;
power-loss durability of the containing directory is not guaranteed.

`make -C native test-settings` checks a fresh executable's persisted values,
corrupt-file rejection, simultaneous reads/writes, original SRAM bit masking,
and a forced write failure that preserves the previous file. ASan/UBSan and a
separate ThreadSanitizer run pass. Both Apple builds pass. The Aurora full-game
link audit now has **99 unresolved symbols**, down from 103; no full-game runtime
or playability is implied by resolving these preferences.

### Native AX delay processing

The Apple and full-game Aurora builds now include the original AXFX allocation
hook layer and an ARM implementation of the SDK delay effect. AX effect sample
and integer state fields use `s32`, preserving their GameCube widths while
retaining full-width native pointers. The console typedef remains the same
original signed-long type.

The delay callback processes the original 160-sample, three-channel blocks,
with independent delay lengths and feedback/output gains. It explicitly keeps
the low 32 bits of products and additions before the signed shift, matching the
PPC arithmetic even when intermediate values overflow. Delay lengths use the
original `ceil((milliseconds - 5) * 32 / 160)` block calculation. Configuration
rejects zero-length lines, overflowing allocations and gains outside 0–100.
Initialization reports its result; partial allocation failure cleans up, and
shutdown clears pointers so repeated shutdown is safe. Invalid settings leave
the existing lines intact; allocation failure after valid reconfiguration
leaves the effect shut down. Parameter edits and allocation-hook changes must
be made while the effect is stopped. Callback and lifecycle operations use the
native interrupt gate.

`make -C native test-axfx-delay` passes ASan/UBSan: independent impulse timing
for one-, two- and three-block delays, 14,400 random/extreme samples against a
wide-integer PPC arithmetic oracle, invalid configurations, failure of each of
the three allocations, and repeated shutdown. The component regression suite,
both Apple builds and both 984-source compilation audits pass. The full-game
Aurora link audit now reports **95 unresolved symbols**. This verifies delay
sample processing, not audio playback: AX voice mixing, AI output, reverb and
chorus processing still need native implementations and game integration.

### Native standard reverb

`reverb_std_backend.c` ports the original SDK standard-reverb processing loop:
two comb filters and two all-pass filters per channel, the low-pass state,
pre-delay, and dry/wet scaling. The ARM implementation uses explicit single
precision fused operations in the original order, byte-counted circular-buffer
positions, and bounded float-to-integer conversion. It accepts separate channel
buffers; the console assembly instead walks three contiguous 160-sample blocks.
The original pre-delay wraps at `N-1` samples, which is retained for `N>=2`.
The unsafe `N=1` case uses a single sample, and sub-sample positive delays become
zero delay. Parameter validation rejects nonfinite/out-of-range values before
reconfiguration. Allocation failures leave the effect disabled with all partial
allocations released; repeated shutdown is safe. Edit parameters or allocator
hooks only while the effect is stopped.

`generate_reverb_reference.py` translates the repository's original SDK assembly
into a **test-only** register-and-memory reference, preserving its instructions,
branches, original structure offsets and contiguous channel traversal. It is
never included in the application. `test-reverb-std` compares 122,880 native
output samples against that reference across four parameter configurations and
multiple complete wraps of the filter histories. Both paths start with the same
initialized coefficients; this tests the DSP translation, not bitwise equality
of Apple's `powf` with the console math library. Additional ASan/UBSan cases
cover noncontiguous channels, tiny pre-delays, invalid reconfiguration, all 15
allocation-failure positions and repeated shutdown. Float-to-integer overflow
handling follows the PPC behavior documented by Dolphin's
[conversion implementation](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/PowerPC/Interpreter/Interpreter_FloatingPoint.cpp).

Standard reverb is included in both Apple builds and the full-game Aurora
link target. This is sample processing only: the AX voice mixer and AI output
still need native implementations, and no audible game playback is claimed.

### Native high-quality reverb

`reverb_hi_backend.c` implements the SDK's three-comb/three-all-pass network per
channel, distinct final delay lengths for left/right/surround, low-pass state,
dry/wet scaling and the original paired-single crosstalk arithmetic. Crosstalk
preserves the SDK's asymmetric coefficient scaling (the right coefficients are
multiplied by 0.6) and current-rounding-mode integer conversion. No PPC code is
executed by the application.

The source contains a pre-delay cursor-store defect. The user's verified
GALE01 revision-2 DOL confirms `stw r29, 0x1b8(r30)` (`0x93be01b8`) at
`0x8035BEB4` and `0x8035C04C`: `r30` points into sample memory rather than the
work structure. Consequently the real cursor stays fixed and the audible
pre-delay is one sample. The native processor preserves that one-sample
behavior for nonzero sample counts while omitting the stray pointer write.
Sub-sample values become zero delay. This also avoids the original out-of-range
write for small allocated pre-delay lines.

The test-only assembly translator now supports the high-quality loop and its
paired-single crosstalk routine. `make -C native test-reverb-hi` compares
122,880 samples across four configurations, including crosstalk and pre-delay,
against those original instruction sequences. The reference retains even the
erroneous cursor store; native output still matches, without reproducing the
unsafe memory access. Coefficients are initialized by the native code before
being copied into the original console layout, so this validates processing,
not console-versus-host `powf` bit identity. ASan/UBSan also verifies separated
channel buffers, tiny pre-delays, invalid settings, all 21 allocation failure
positions and repeated shutdown. Both reverb references remain test-only.

The effect is included in the Apple hosts and full-game Aurora link audit.
Native voice synthesis and audio-device output remain unfinished, so these
sample-processing tests do not establish audible game playback.

### Original audio-driver auxiliary-effect integration

`axdriver.c` now checks native effect workspaces before narrowing their sizes or
forming pointers. Exact-fit allocation is valid; exhausted/overflowing requests
return `NULL` without advancing the allocation cursor. Native heap-size
calculation uses wide intermediates, rejects nonfinite/invalid pre-delay and
oversized delay lines, and counts exactly the high-quality reverb's allocated
filters. Invalid workspace size, alignment or bounds are rejected before the
current effect is replaced. Effect setup through `AXDriver_8038E30C` runs under
the native interrupt gate. Native callback adapters give AX's two-void-pointer
callback signature a real compatible function entry point.

`make -C native test-axdriver-aux` exercises the original driver, original default
parameter setup, real allocation hooks and all four native effect
processors together. It configures both auxiliary channels with exactly
the reported workspace sizes at addresses above 4 GiB, invokes the callbacks
registered by the driver for 120 blocks, checks guard bytes and allocation
exhaustion, rejects undersized/misaligned/oversized workspaces, and confirms
turning off one channel preserves the other. ASan/UBSan passes. Only AX callback
registration is captured by a test double; all four processors are real.
No mixer, audio scheduling or audio-device output is supplied by this test.

The rest of the audio driver's bank relocation still includes console-width
pointers and remains unfinished. Full native game audio is not yet available.

### Native chorus and all four driver effects

The original `chorus.c` now has native C replacements for both PPC sample-rate
conversion loops. They retain the original coefficient table, fused operation
order, fractional-position carry, ring-buffer wrap, conversion rounding and
three saved history samples. The original coefficient byte offset is
`rotate_left(phase, 7) & 0x7f0`, not a conventional high-bit phase index. The
verified game DOL contains `rlwinm r10,r4,7,21,27` (`0x548a3d76`) at
`0x8035CF54` and `0x8035D0EC`, confirming this unusual selection.

Native initialization/settings reject parameters that produce unsupported pitch
steps, invalid base delays, zero periods or overflowing calculations. Shutdown
clears its allocation and is repeatable. The negative-modulation fractional
pitch calculation now uses an unsigned shift. Callback/lifecycle access uses
the native interrupt gate; callers must edit parameters while stopped.

The test-only assembly translator also handles both original chorus SRC loops.
`test-chorus` compares 115,200 samples plus fractional position, integer position
and retained history across zero, boundary and nontrivial pitch increments for
both paths. It checks live positive/negative modulation, invalid settings and
allocation failure under ASan/UBSan. The original AX driver integration test now
uses **all four real effects** on both auxiliary channels with exact-sized
workspaces. Only callback registration is a test double; none of the processors
is replaced. The application builds include the original portable lifecycle
code and native SRC bodies; the assembly reference is never linked into them.

Audio sample processing is still not audio playback. Native AX voice mixing,
AI device output and sound-bank relocation remain unfinished.

### Native AX voice ownership and allocation

The Apple and Aurora builds now include the original `AXAlloc.c` priority
stacks and allocation/stealing logic. Its free-stack and callback-stack pops use
full native pointers. Native entry-point checks reject out-of-range priorities
and attempts to free or reprioritize unowned/inactive voices; callback-stack
service holds the cooperative interrupt gate. Public acquire/free/priority
operations use that gate as in the original SDK. Internal queue helpers must
be called with audio stopped or with the same gate held.

`ax_voice_backend.c` owns 64 CPU voice parameter blocks and aligned ITD storage,
with the original parameter-reset behavior. `melee_ax_voice_pool_init` (also
available as the SDK internal `__AXVPBInit`) initializes this pool while audio
is stopped. Native code retains full pointers for updates and ITD storage;
console DSP split-address fields are left zero rather than filled with truncated
host addresses. This is **not** an implementation of `AXInit`, a mixer, a DSP
command queue or device output. Those remain missing; the Apple preview does
not start game audio.

`test-ax-voices` passes ASan/UBSan and ThreadSanitizer. It verifies all 64 unique
slots, equal-priority exhaustion, oldest/lower-priority stealing, old-owner
callbacks, priority changes, LIFO callback delivery, voice reuse and double-free
rejection. Eight threads perform 8,000 acquire/free cycles with an atomic
ownership check, followed by another complete pool exhaustion/reuse check.

### Native AX voice parameter setters

The Apple/Aurora builds now compile the original `AXVPB.c` setters with native
adaptations. Console DSP synchronization, command buffers, DSP cycle accounting
and the hardware-oriented pool initializer remain excluded; they are not
replaced with fake success paths. The native pool provides parameter reset and
voice ownership. Setters validate ownership and update active voices under the
interrupt gate.

Native structured assignments replace unrolled word copies for mix, depop,
address, ADPCM, SRC and loop state. This permits valid 16-bit-aligned parameter
objects and avoids aliasing/word-alignment violations. PCM address initialization
sets the gain fields explicitly (`0x0800` for format 10 and `0x0100` for format
25) instead of writing big-endian packed constants into little-endian memory.
The original mixer-control and synchronization-flag logic is retained. Sample
addresses remain console-format **ARAM offsets**, not truncated native pointers;
a native mixer still needs to resolve them through the ARAM backend.

Pitch conversion rejects negative/nonfinite values and clamps finite values to
four before integer conversion. Per-block update writes and millisecond indices
are checked before changing state or writing beyond the update buffer.
`test-ax-parameters` passes ASan/UBSan for every mixer field in both control modes,
SRC selection and ratios, halfword-aligned address copies, PCM/ADPCM state,
address splitting, volume, ITD, FIR/depop/type/state, full update buffers and
explicit rejection of invalid operations. The stored output-mode value currently
only selects mixer-control flags; audio mixing and output are still unfinished.

### Bounded ARAM reads for native sample decoding

`melee_aram_read` copies committed ARAM bytes into caller-owned native storage.
It accepts arbitrary byte alignment, checks the complete range before copying,
and leaves the destination unchanged on failure. Reads, DMA chunk commits and
reset use the same interrupt-gate/mutex ordering, so a native mixer can read
while holding the cooperative audio gate without exposing a pointer that reset
could invalidate. Reads do not wait for pending DMA: they observe the currently
committed bytes. A multi-chunk ARQ request can therefore be partially committed;
callers must still respect the original transfer-completion protocol.

The ARAM test now exercises reads before initialization and after reset,
unaligned subranges, exact-end zero-length reads, last-byte ranges, null buffers,
32-bit offset overflow and native-size length overflow. A read while DMA is
queued and the interrupt gate is held confirms that it returns prior data
without waiting for the blocked DMA worker. After the original ARQ callback
completes, reads match the transferred source bytes. ASan/UBSan and
ThreadSanitizer pass. This API supplies sample bytes; it does not decode or mix
audio by itself.

### Native AX source-sample decoding

`melee_ax_decode_sample` decodes one source-rate sample from committed native
ARAM using an `AXPB`. Formats 0, 10 and 25 implement ADPCM, big-endian PCM16 and
PCM8 respectively. It updates address, predictor/history and loop/end state
under the native interrupt gate. PCM gain/prediction uses the stored parameters;
ADPCM clamps reconstructed samples to the signed 16-bit range. Non-stream loops
restore saved loop histories, while streaming voices retain their histories.
Stopped voices produce zero. Format and loop behavior were checked against
Dolphin's [DSP accelerator](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/DSP/DSPAccelerator.cpp)
and [AX voice handling](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/HW/DSPHLE/UCodes/AXVoice.h).

The API makes a local parameter copy before decoding and commits only on
success, so invalid formats, invalid ADPCM header addresses or failed bounded
ARAM reads leave both the caller's PB and output unchanged. This targets the
normal AX sample formats accepted by the SDK setters; special DSP MMIO formats,
address mirroring and malformed header-address edge cases are not implemented.
It decodes source samples only. Pitch conversion is implemented separately below;
envelopes and voice mixing are implemented below. Completion callbacks and
speaker output remain unfinished.

`test-ax-decode` uses actual ARAM DMA completion, the original voice allocator
and setters, and known byte/sample vectors. ASan/UBSan checks PCM signs and
endianness, two ADPCM frames with changing scale, predictor histories, positive
and negative clipping, normal/streaming loop state, terminal samples and zero
output after stopping. Failure tests verify that invalid sample ranges and
ARAM reset do not partially modify the PB. These fixtures are generated test
data, not validation of all sound banks in the user's game image.

### Native AX sample-rate conversion

`melee_ax_resample` consumes the source decoder and retains the four-sample
history and 16-bit fractional position across calls. It supports direct reads,
linear interpolation and the three selectable four-tap coefficient tables,
with signed integer arithmetic and output saturation. Blocks may contain up to
160 output samples. Invalid state, missing/short coefficient tables or source
read failures commit neither parameters nor output, even if earlier samples in
the block were decoded successfully. No linear fallback is substituted for a
missing four-tap table.

The decoder test covers known direct/linear waveforms, half-rate interpolation,
four-tap averaging and clipping with **synthetic test coefficients**, all 128
phase-table selections, and identical output/final state when a 160-sample block
is split into small calls. Continuity is checked for all three modes and ratios
0, 0.5, 1, 1.5 and 4.

`melee_ax_resample_native` supplies pinned reconstructed Dolphin coefficients
embedded at compile time. The unchanged upstream binary, revision, hash and
licensing notice are retained in [third_party/dolphin_dsp](third_party/dolphin_dsp/README.md).
`tools/prepare_ax_coefficients.py --check` verifies the dependency hash and
generated C include; `test-ax-decode` runs this check. The test independently
reads the big-endian binary and checks all three banks and 128 phases against
signed wide-integer arithmetic, then compares whole/split blocks against the
explicit-table API. No runtime file access or coefficient download is needed.
These reconstructed filters are not a verified dump of the user's console DSP
ROM. Voice mixing is implemented below; the full mixer lifecycle and
audio-device output remain unfinished.

### Native AX voice mixing

`melee_ax_mix_voice_ms` renders 32 samples into one millisecond of a 160-sample
frame containing main, aux A and aux B left/right/surround accumulators. It
uses the native source converter, applies the signed GameCube volume envelope,
then unsigned channel gains with per-channel clipping and wrapping 32-bit
accumulation. The old AX control format always enables main left/right, gates
auxiliary and surround buses, and uses one shared ramp-enable bit. Channel
volumes wrap to 16 bits; each enabled channel retains its last contribution in
the PB depop fields. A source ending during a quantum completes that quantum;
later quanta skip the stopped voice.

`melee_ax_mix_voice_frame` applies scheduled PB word updates before each of the
five quanta, through an explicit native update pointer. It rechecks update
counts and offsets each quantum, and rolls back the full PB and all buses if
any update or voice processing fails. Neither function allocates memory or
runs device/callback code. Calls hold the native interrupt gate while using
voice state and ARAM.

`test-ax-mix` checks all 16 ordinary routing/ramp combinations against an
independent integer reference, including envelope sign/wrap, channel clipping,
overflowing accumulator inputs, distinct channel gains, and last-sample
state. Hand-checked two-voice sums verify accumulation. Scheduled start, gain,
stop and restart updates are checked at exact sample boundaries, with late
frame errors verified to leave all output unchanged. Tests run with ASan/UBSan.
Both Apple app builds compile this implementation, but do not start a mixer.

The behavior reference is Dolphin's pinned
[AX voice processing](https://github.com/dolphin-emu/dolphin/blob/a2efdf1197be8132674b90fe9cf4761df39752ed/Source/Core/Core/HW/DSPHLE/UCodes/AXVoice.h)
and [older control-bit conversion](https://github.com/dolphin-emu/dolphin/blob/a2efdf1197be8132674b90fe9cf4761df39752ed/Source/Core/Core/HW/DSPHLE/UCodes/AX.cpp).
The native mixer carries GPL-2.0-or-later attribution; the license text is in
`third_party/dolphin_dsp`. This is not a comparison against recorded console
audio. Active voices requesting FIR, interaural delay or Dolby Pro Logic II
currently fail explicitly. The game synthesizer can request interaural delay,
so this is a remaining integration requirement. Pool rendering, auxiliary
returns and depop tails are implemented below. Callback scheduling and speaker
output remain unfinished; these tests do not demonstrate audible game playback.

### Native AX auxiliary frame processing

`melee_ax_aux_process_frame` connects the mixed aux A/B buses to the original
SDK `AXAux.c` ring and registered effect callbacks. The three slots preserve
the original two-frame return latency: one slot receives the current mixed
frame, another returns processed samples to main L/R/S, and the CPU processes
the third before rotation. Returns use wrapping 32-bit accumulation.

`AXAux.c` now uses full-width native addresses and 32-bit sample buffers while
retaining the original console types through aliases. The public AX auxiliary
callback registration functions are linked into both Apple builds and the
full-game Aurora target. Native interrupt locking serializes registration,
processing and reset. The AX driver adapters copy the three channel pointers
into the effect's typed buffer descriptor, avoiding incompatible struct aliasing.

The original aux B transfer gate checks aux A's callback. This behavior is
preserved: B alone receives no new input, while A alone can return unprocessed
B samples. Native initialization clears all three slots for repeatable resets
(the original initializer explicitly clears only the first slot and relies on
initial BSS zeroing). Mode 4/DPL2 and recursive frame processing fail before
changing buses or invoking callbacks. Initialize only while audio is stopped.

`test-ax-aux` checks the original ring timing and channel layout, addresses above
4 GiB, both and individual A/B registrations, wrapping returns, reset after nonzero
traffic and callback reentrancy. An impulse through the real SDK delay returns
after one effect block plus two ring blocks. Tests use ASan/UBSan; effect memory
in this isolated test uses the SDK allocation hooks with malloc/free.
This completes the auxiliary return primitive, not a running AX mixer: voice
pool scheduling, interaural delay, final output processing and
speaker delivery remain unfinished.

### Native AX depop frame initialization

`MeleeAXDepop` owns nine pending channel sums separately from reusable voices.
`melee_ax_depop_add` adds a retired voice's saved channel contributions with
explicit 32-bit wrapping. `melee_ax_depop_begin_frame` initializes all main,
aux A and aux B buses with fades before voice mixing and auxiliary returns.
It follows `AXSPB.c`'s signed division by 160, slope limit of 20 per sample,
and clearing of remainders smaller than 160. The first output sample is the
current sum; the sum advances by the full 160-sample frame for the next call.
Calls hold the native interrupt gate and allocate no memory.

`test-ax-depop` checks distinct values on all nine channels, positive/negative
division and slope-limit boundaries, 32-bit extremes, multi-frame decay,
clearing otherwise unused buses and repeated retirement sums. It runs with
ASan/UBSan. Frame initialization uses the same initial-value-plus-sample-delta
sequence as the pinned Dolphin [AX buffer initializer](https://github.com/dolphin-emu/dolphin/blob/a2efdf1197be8132674b90fe9cf4761df39752ed/Source/Core/Core/HW/DSPHLE/UCodes/AX.h).
This is not a comparison against recorded console audio. The pool renderer
below connects retirement contributions and invokes this initializer before
mixing. Interaural delay, final output processing and speaker delivery remain
unfinished.

### Native AX voice-pool frame rendering

`melee_ax_voice_pool_render` renders the actual 64-slot allocator pool into
fresh main/auxiliary buses. It keeps last-rendered channel contributions apart
from editable user PBs, consumes depop flags even on freed slots, initializes
the frame's fades, and renders allocated voices with scheduled updates.
Successful frames commit all PBs and consume sync/depop flags and update
schedules. Failed frames commit no voice, fade or output changes. Resetting
the pool invalidates the renderer's histories through a generation counter.
The entire operation holds the native interrupt gate, uses bounded stack
storage and invokes no callbacks. All voices are processed; console DSP cycle
budget voice dropping is not implemented.

`test-ax-pool-mix` fills the original allocator with 64 looping PCM voices and
checks their combined output. It exercises voice stealing, explicit stop,
free, immediate reuse, host-side dpop edits, and recovery after an invalid
voice causes a frame failure. It also verifies one-time schedule consumption
and resetting outstanding fades. ASan/UBSan caught and prompted correction of
the original signed `1 << 31` sync-mask expression to `1U << 31`. The Makefile
now tracks `ax.h` explicitly for native tests.

This connects automatic retirement fades to pool rendering, but does not yet
schedule frame callbacks or connect pool output to auxiliary processing and
the audio device. Interaural delay, final output processing and real-time
audio scheduling remain required before the game audio path is usable.

### Native AX stereo frame output

`melee_ax_output_frame` connects voice-pool rendering and auxiliary processing
to 160 signed-16 stereo samples at 32 kHz (5 ms). Output is host-endian **left,
right** interleaved for a native audio device; the console DMA's right/left
packing is not exposed to native callers. Clipping occurs after accumulation
and effect returns. `MeleeAXOutput` retains the un-clipped surround bus for
the next frame and discards that history when the voice pool is reset.

The original `AXCL.c` sequence overwrites the initial main buses with previous
surround samples before adding voices: mode 0 seeds both L/R with that history,
mode 1 negates the left channel, and both clear the initial surround bus. Modes
2/3 retain the depop initialization without a surround seed. This includes
the original overwrite of main-channel depop initialization in modes 0/1.
The raw pool-rendering API remains available without these output-mode seeds.
The pinned [Dolphin AX command handlers](https://github.com/dolphin-emu/dolphin/blob/a2efdf1197be8132674b90fe9cf4761df39752ed/Source/Core/Core/HW/DSPHLE/UCodes/AX.cpp)
provide the surround-seed and final-clamping behavior reference.

Initialize both the voice pool and auxiliary ring before rendering. The
output function holds the interrupt gate across readiness checks and both
stages, rejecting recursive calls and unsupported mode 4 before mutation.
Voice failures preserve the caller's PCM, history, pool and auxiliary ring.
Effect callbacks execute only after all voices render successfully.

`test-ax-pool-mix` now checks complete frames through the original allocator,
ARAM decoder and auxiliary callbacks: all four output modes, surround carry,
two-frame effect returns, L/R order, 64-voice clipping, failed-frame recovery,
callback reentrancy and reset. It runs with ASan/UBSan. This produces PCM
offline; it does not start an audio device. Callback/pull integration is
described below. Interaural delay, real-time scheduling, game sound-bank
integration and audible playback remain unfinished.

### Native AX callbacks and device-sized pulls

`AXRegisterCallback` now registers the native frame callback under the interrupt
gate. Successful output frames invoke it after auxiliary processing and PCM
completion, so its parameter edits affect the next frame. Failed frames invoke
neither effects nor the user callback; recursive output rendering is rejected.

`MeleeAXStream` adapts fixed 160-sample frames to device-sized pulls of up to
4096 stereo frames. It retains unused PCM, so pull partitioning cannot change
samples or callback cadence. A source/render failure preserves the completed
prefix, silences the remainder, and latches silence without further callbacks
until stream or pool reset. Pool reset also invalidates buffered PCM/history.
Initialize one stream per pool while audio is stopped. This API supplies
32 kHz host-endian signed-16 L/R samples; it neither opens a device nor converts
to a different device sample rate.

The integration test compares one 1000-frame pull with varying small pulls
while the real registered callback alternates voice volume each frame. It
checks seven callbacks, retained partial-frame position, callback reentrancy,
failure after a valid prefix, latched silence and recovery after reset.
The Apple audio-device clock and output connection remain unfinished.

### Apple audio source adapter

`MeleeAudioOutput` configures an AVAudioSourceNode with 32 kHz signed-16
interleaved PCM and connects it to AVAudioEngine's floating-point mixer. The
engine handles output-rate conversion. Its source block reads only from a
bounded single-producer/single-consumer queue: it takes no game lock, allocates
no memory and invokes no AX callbacks. Unavailable samples become silence.
The producer must retain any PCM that does not fit in the queue.

The C queue checks for lock-free counters at creation and uses acquire/release
publication. Its ASan/UBSan and TSan tests cover full/empty queues, physical and
32-bit counter wraparound, zero-filled underruns and concurrent transport of
500,000 stereo frames. Create/destroy it only when producer and consumer stop.

The adapter supports device start/stop and an explicit offline mode. On iOS,
device start activates a playback audio session; stop releases that session.
It is compiled into both Apple apps but is not started by the preview UI.
An offline test verifies Int16-to-float conversion, L/R order, 32/48 kHz output
and silence after draining the queue. Build with
`make -C native build/test-audio-output`, then run
`native/build/test-audio-output`. Apple's audio components must be available
to the process; a restricted sandbox may fail component discovery even in
offline mode. The verified test renders only to memory, without sound or UI.

The design follows Apple's [source-node render-block requirements](https://developer.apple.com/documentation/avfaudio/avaudiosourcenode/init(format:renderblock:)).
The mixer worker and pacing are described below. Interruption/route-change
handling and game startup integration remain unfinished. No device playback or iPhone
hardware test has been performed, and this does not establish game playability.

### Paced mixer worker

`melee_audio_producer_start` claims the sole mixer producer and starts a pthread
that renders fixed AX frames into the audio ring. It buffers 1,600 frames
(50 ms at 32 kHz), increasing for larger observed render requests to retain
one device quantum plus 20 ms of scheduling headroom (rounded to AX frames,
bounded by ring capacity). It polls consumption outside the device callback. The
device consumer remains lock-free. A mixer failure latches `MELEE_AUDIO_FAILED`
and ends production; already queued valid samples can drain before silence.
An asynchronous stop request is safe from an AX callback. Destruction joins
the worker and must run on the control thread without the interrupt gate held.

`MeleeAudioOutput.startMixer()` attaches this producer to the adapter's queue;
the pool/aux and callback must be initialized first. Manual PCM enqueue is
disabled while it owns the producer. `stop()` stops the engine, joins the
worker, and discards queued PCM before releasing the iOS session. One owner
controls this lifecycle, and any external manual producer must already stop.

The C integration test verifies nonzero PCM from actual voices on the worker,
20-ms pacing without device demand, resumed production when the consumer
drains samples, exclusive ownership, joined shutdown and deliberate failure.
It passes ASan/UBSan and TSan. The Apple offline test links the same mixer and
checks worker/engine start, stop and restart with an empty voice pool. This is
not a device timing or underrun-performance test. Recovery logic is described
below. QoS tuning, physical route validation, interaural delay, game startup
and audible device playback remain unfinished.

### Apple audio recovery

The adapter observes engine configuration changes and, on iOS, audio-session
interruptions and output disconnection. Notification callbacks dispatch work
asynchronously to the main thread; lifecycle calls also require that thread.
An interruption stops the engine and joins the producer while retaining play
intent. An ended interruption resumes only with `shouldResume` and retained
intent. Explicit stop cancels that intent, and an output disconnect requires
an explicit subsequent start. A configuration change recovers a stopped
engine; stale notifications are ignored if the engine is already running.
Failed recovery leaves both resources stopped and exposes `recoveryError`.

Offline tests exercise interrupted start/stop intent, denied automatic resume,
engine configuration recovery, disconnected output and recovery failure caused
by competing mixer ownership, followed by successful retry. They simulate
these events rather than claiming a physical headset or phone-call test.
The behavior follows Apple's [engine configuration notification](https://developer.apple.com/documentation/foundation/nsnotification/name-swift.struct/avaudioengineconfigurationchange)
and [audio interruption guidance](https://developer.apple.com/documentation/avfaudio/handling-audio-interruptions).
Media-services reset requiring engine reconstruction, application background
lifecycle, physical route validation and full game integration remain unfinished.

### Native SEM sound-command loading

The native `AXDriver_8038DA70` path reads SEM data through the real DVD backend
and decodes it into owned host-endian words plus full-width pointer tables.
It replaces the three console relocation loops that added a truncated base
address into 32-bit file words. Five counted tables are validated before
publication: counts must fit the file, pointer targets must be aligned and
inside the payload, and bank offsets must be ordered and within the command
index. The original command interpreter consumes the converted words.

Native reads are synchronous and bounded; the console busy-wait callback is
not invoked. A failed load preserves the current table. Replacement and unload
refuse while active sound commands reference the table. Unload clears all
counts/pointers, and sound lookup/activation now holds the interrupt gate across
those tables. Negative sound IDs are rejected before indexing.

`test-sem` checks unaligned input, host-endian values, native pointer aliases,
truncation and malformed count/offset cases with ASan/UBSan. With the supplied
CISO path as an argument, it tests the actual `audio/us/smash2.sem` DVD loader:
55 bank offsets, 4,035 command streams, reload/unload, failed paths and active
reference guards. Game bytes remain in ignored build output. This validates
command-table loading, not sound playback: complete command execution,
interaural delay and game startup remain unfinished.

### Native SEM command-pointer bounds

Native command fetches and loop destinations now validate against the owned
SEM payload before dereferencing or subtracting pointers. The interpreter
holds the interrupt gate, so unload/replacement cannot race execution. Loop
validation accounts for the original post-instruction increment, including
jumps back to the first payload word. Invalid destinations stop with an HSD
panic instead of performing an out-of-range native read. Unknown opcodes keep
the original behavior; valid retail streams contain `0xFD` marker words.

The parser records the payload boundary. Tests cover metadata pointers,
unaligned/foreign/one-past pointers, oversized backwards displacements and
unchanged output on rejection. The real US SEM scan validates every payload
word and all 371 encoded loop destinations. This is structural validation,
not execution of every command path; zero-time infinite command loops and
full synthesizer playback remain unverified.

### Native sample-bank voice retirement

The original synthesizer's bank range scan now reconstructs the 32-bit ARAM
address from its two host-endian 16-bit fields. Its previous native `size_t`
load also read adjacent ADPCM data and reversed the address halves on ARM64.
The scan and voice retirement hold the interrupt gate to exclude mixer updates.
Console builds retain the original implementation.

`make -C native test-synth-bank-range` exercises the actual synthesizer scan
under ASan/UBSan: lower and upper boundaries, adjacent poisoned ADPCM data,
stereo retirement, inactive entries, repeated scans and interrupt nesting.
This range test alone does not establish that real game sounds can play
through the synthesizer; the later tests below cover bank loading and playback.

### Native SSM metadata decoding

`melee_ssm_open` converts an SSM file into owned host entries with one or two
typed AX voice records. It decodes the 32-bit entry fields and each 16-bit
address, ADPCM coefficient/history and loop field individually. Encoded sample
bytes are copied unchanged; addresses remain relative to the sample bank.
The parser checks metadata lengths, record counts, sound-ID overflow, voice
counts and sample-data bounds before exposing a bank. It accepts unaligned
input. It does not yet validate all sample-address or codec semantics.

`make -C native test-ssm` runs synthetic stereo, byte-order, ownership and
malformed-input checks under ASan/UBSan. Passing a CISO path to
`native/build/test-ssm` also scans every SSM file: the supplied US revision 2
image contains 110 banks, 3,085 entries and 3,375 voices (290 stereo entries).
The test uploads each bank through native ARAM DMA, relocates its ADPCM nibble
addresses, and decodes the first 160 source-rate samples of every voice. This
checks 540,000 samples without opening an audio device. It is not a comparison
against console recordings and does not test every sample, loop or pitch.

The native bank publication, unload, relocation and asynchronous loading
integration are described below. This parser alone does not prove playback.

### Original synthesizer start on native voice records

The native `HSD_Synth_80389334` sound-start path now reads two typed SSM voice
records instead of using a 64-byte stride through the console's overlapping
entry layout. Its original allocation, volume, pan, priority and AX setter
calls remain in use. Initial sample-rate conversion uses `AXSetVoiceSrcRatio`
after a fresh SRC state, avoiding a host-endian 32-bit store across the two
16-bit ratio fields. Invalid native voice counts and priority/channel indices
are rejected before allocation or array access. Console code is unchanged.

`make -C native test-synth-start` uses the actual synthesizer, AX allocator,
setters and native output mixer. Synthetic stereo input verifies independent
left/right records, opposite-sign PCM and the correct 0.5 ratio for a 16 kHz
sample. With the CISO argument, the test parses `audio/end.ssm`, transfers its
samples through ARAM DMA, installs a converted entry in the synthesizer lookup
table, starts that original sound, and renders 1,600 stereo frames to memory.
It does not open an audio device or compare against a console recording.

This test directly supplies publication; the separate asynchronous loader
test below exercises the game's public loading entry point. Passing these
tests does not establish a playable game.

### Native SSM group ownership

Native group publication now allocates owned synthesizer entries, checks bank
capacity and ADPCM address bounds, relocates nibble addresses, and links the
entries only after validation succeeds. Failure preserves the bank cursor and
lookup table. Group unload removes entries by pointer identity so overlapping
sound IDs from another loaded group remain intact. Native bank and group
unload stop affected voices under the interrupt gate before releasing metadata;
bank unload also resets its allocation cursor. Individual sound lookup removal
uses native fields instead of indexing through a packed console header.

The synth-start test covers owned metadata, duplicate IDs, failed publication,
group removal and bank reset. Its retail `end.ssm` path now uses this production
publication helper, frees the parsed bank before playback, and unloads the
active sound through the original public group-unload entry point.

The asynchronous integration below calls publication after transfer completion.
Initialization and full game startup remain unfinished.

### Native sound-bank compaction

The original public bank-compaction entry point now walks native owned groups,
stops voices in the bank, queues downward ARAM moves through the real DevCom
relay, and relocates each channel's address fields individually. It updates
the actual bank cursor rather than indexing beyond the console group array.
The native group-readdress entry point checks ownership before accessing a
group and supports downward moves within its bank.

Transfer completion and the public wait synchronize their counter through
the interrupt gate. The wait sleeps between checks and requires interrupts
enabled while a transfer is pending, so the completion callback can run.
Native publication and sound-start requests reject work while compaction is
pending. Callers use the existing `HSD_SynthSFXBankDeflagSync` completion point
before starting sounds from the moved data.

`make -C native test-synth-compact` checks the original public entry points
with actual ARAM DMA, ARQ and DevCom under ASan/UBSan. It removes a 32-byte
leading group and moves a 36,896-byte group followed by a 64-byte group into
the gap. The overlapping transfer crosses relay chunks; every copied byte
and relocated address is checked. The test also covers voice retirement,
sound-start rejection during transfer, repeated compaction and an empty bank.
The asynchronous SSM loader integration is described below.

### Native asynchronous SSM loading

The original public `HSD_SynthSFXLoad` queue now has a native read/parse/transfer
path. It reads the file into aligned owned memory, parses SSM metadata, checks
ARAM bank capacity, and transfers encoded samples through the existing DevCom
DVD-to-ARAM request. Completion publishes owned native entries before invoking
the original callback with its entry number and mode. The six-request queue
retains order; pending counts and waits synchronize through the interrupt gate.

Active cancellation keeps I/O storage alive until the completion callback and
suppresses publication. Queued cancellation removes requests immediately.
Bank and group unload also discard their pending loads, preventing late
publication after unload. Missing paths return -1; malformed metadata/capacity
failures drain the queue without publishing or invoking a success callback.
Loading is rejected while compaction is pending, and compaction requires the
load queue to be empty. Full-file reads currently copy sample bytes during
parsing and read them again for DMA; reducing this overhead is future work.

`make -C native test-synth-load` tests three queued loads through real DVD,
ARAM, ARQ and DevCom implementations, checks transferred bytes, and starts a
sound through the original synthesizer and native mixer. It covers cancellation
of active/queued loads, bank/group unload during loading, missing paths and
capacity rejection. The separate `build/test-synth-load-tsan` target runs the
same callback path under ThreadSanitizer. With the supplied CISO argument, the
test enumerates all 110 SSM banks, loads each through the public API, and
starts every one of their 3,085 sound entries (3,375 channels) through the
original synthesizer. It renders one 160-frame native output block for each
sound. This checks bank publication and initial playback across the complete
retail SSM set, not every sample, loop, command stream or full-game audio.

### Native AX initialization

The public `AXInit` entry point now initializes the native voice pool and
auxiliary buffers, resets the output mode to the original default (0), and
clears the frame callback. A new pool generation invalidates native frame,
depop and surround history. Calling it recursively during rendering/effects
is rejected. Apple output scheduling starts separately after game setup;
this function does not start a host audio device or console DSP execution.

The asynchronous SSM test now initializes AX through this public entry point.
The output test also reinitializes after dirty voice parameters, mode 4 and
registered frame/effect callbacks; it verifies silent output, cleared callbacks
and all 64 distinct voices available again. Full game startup and streamed
audio still need further integration and verification.

### Original synthesizer initialization and frame callback

The native AI configuration backend supplies the subset used by Melee's
startup: idempotent initialization/reset, the fixed 32 kHz AX output rate,
and separate left/right AI stream-volume state. Selecting 48 kHz DSP output
is rejected because the native mixer produces 32 kHz frames; Apple performs
device-rate conversion. This backend does not implement DVD hardware streaming,
DMA interrupts or stream counters. AI volume state is not applied to AX voices:
the original synthesizer already controls their mix and gain separately.

The asynchronous loader test now calls `AIInit`, the original `HSD_SynthInit`
and `HSD_SynthSFXAllocateBank`. This initializes silent/bank/stream ARAM regions,
channel gains and the original synthesizer callback without manually seeding
those globals. Every rendered frame executes `HSD_SynthCallback`, including
voice housekeeping and volume processing; the test verifies its clock count.
All 110 retail banks and 3,085 initial sound frames pass with this setup under
ASan/UBSan and ThreadSanitizer. Full SEM command-driver playback, fades,
streamed music and hardware output remain separate acceptance work.

`DCFlushRange` now provides a sequentially consistent CPU fence, alongside
the other native coherent-memory cache functions. It is not GPU synchronization.

### SEM driver integration probe and ITD processing

`native/build/test-sound-driver CISO` initializes the original AX driver, loads
`audio/us/main.ssm` and `audio/us/smash2.sem`, and starts SEM sound commands through
`AXDriver_8038CFF4`. The probe initially exposed unsupported ITD on sound 0.
With the integration below it passes all 528 main-bank SEM commands, rendering
200 frames and then keying off/draining each sound. It checks that driver voice
lists and counts return to zero. It remains a separate CISO-dependent target.

`melee_ax_itd_ms` implements the 32-sample ITD processing step. It combines
32 samples of post-envelope
history with the current quantum. Left/right windows start at each channel's
shift (0..31), giving delays of 32..1 samples. Each shift moves one sample toward
its target after mixing, once per millisecond. It retains the current quantum
as history. Disabled ITD bypasses the delay and preserves history; surround
buses use current samples directly.

This follows the instructions in the repository's
`extern/dolphin/src/dolphin/ax/DSPCode.c`: history load at DSP 0x022A, initial
window setup at 0x0215, channel shift updates at 0x02FA/0x0311, and history
writeback at 0x035A. Their instruction words match the corresponding sequence
in Dolphin's [historical AX DSP analysis](https://github.com/dolphin-emu/dolphin/blob/26a8556c822148a2e34cb8291daeaee00cd91455/docs/DSP/DSP_UC_6A696CE7.txt).
The analysis's pseudocode omits the shift in two pointer expressions; the actual
`add ACC0, ACC1` instructions at 0x030E/0x0325 include it.

`make -C native test-ax-itd` checks all 1,024 shift/target pairs, independent
channels, interpolation timing, impulse delays, history retention and rejected
parameters under ASan/UBSan. The mixer integration below supplies per-voice
history, SDK buffer resets and complete frame rollback.

### ITD voice-mixer integration

The stateful frame mixer now applies ITD after the volume envelope and before
channel gains. All main/auxiliary left/right buses use the delayed windows;
surround buses use current samples. Stateless mixer entry points still reject
ITD because they have no persistent history. FIR and DPL2 remain unsupported.

The pool copies each voice's owned ITD buffer into its frame transaction and
commits it only if every voice succeeds. The SDK's COPYITD flag clears this
history unless COPYTSHIFT takes precedence, matching the original sync path.
The pool checks the native buffer pointer before use. Pool initialization
clears history along with the voice state.

Pool tests cover all nine buses, post-envelope history, target interpolation,
buffer-reset precedence and failure after an earlier voice advanced its delay.
The retry verifies that an aborted buffer reset takes effect exactly once.
The retail SEM probe renders main-bank commands and verifies key-off cleanup.
This is not exhaustive SEM execution, arbitrary mid-frame ITD
update validation, a console recording comparison, or full-game acceptance.

### Complete SEM bank sweep

`native/tools/verify_sound_driver.py CISO` reads the SSM filename table from
`src/melee/lb/lbaudio_ax.static.h` and runs the driver test for all 55 US banks.
Each process loads the common main bank and the selected bank into separate
native ARAM regions, then starts every command entry in that SEM bank. The
test renders 200 frames (one second), calls the actual `AXDriverKeyOff` API,
and drains another 40 frames before checking empty driver lists and counts.
The status-query API is not a substitute for key-off. This corrects the initial
probe, whose first 32 sounds happened to finish naturally.

All 4,035 command entries pass this bounded run with ASan/UBSan and in a
separate ThreadSanitizer build. The final
bank has one intentionally silent command: its explicit zero-volume instruction
is checked alongside silent PCM. Other banks must produce nonzero PCM across
their commands. This does not prove every command produces audio, every
branch/loop executes, or every long sound finishes; each run covers the first
second followed by an explicit stop.

Per-bank logs are stored in ignored `native/build/sem-bank-results`. Build
`build/test-sound-driver-tsan` and pass `--binary native/build/test-sound-driver-tsan`
with a separate `--output` directory to run the same sweep under ThreadSanitizer.

### Native retrace clock and HSD handoff

`native/src/vi_backend.c` supplies a monotonic 60000/1001 Hz software retrace
clock for the US game. Aurora's `VIInit` and `VIFlush` now forward to it.
It implements pre/post callbacks, retrace counts and waits, framebuffer/black
state staging, and field selection. The callback sequence follows the SDK:
advance the count, call pre, apply flushed state, call post, then wake waiters.
A flush inside pre is visible at that retrace; an unflushed setter is not.
The native interrupt gate serializes callbacks with cooperating game code.
Waiting releases and reacquires that gate, including for callers that entered
with interrupts disabled. Long stalls drop elapsed ticks instead of issuing a
burst of callbacks.

`melee_vi_presentation()` exposes the latched framebuffer identity and black
state for a future presenter. It does not read GameCube framebuffer bytes or
present a Metal texture. The clock is independent of the physical display's
refresh rate; it does not emulate a scanline counter, PAL timing, or exact
interlaced beam position. The native display reports progressive capability.
The host lifecycle owner must serialize initialization/shutdown and stop the
clock with interrupts enabled, outside callbacks and after waiters have exited.
The preview app compiles this service but does not start it.

`test-vi-backend` verifies timed waits, four concurrent waiters, callback order,
flush staging, preserved interrupt state, and restart. `test-hsd-vi` runs the
original HSD callbacks with this real clock and observes two completed buffers
advance through NEXT, DISPLAY and FREE. GX is still a test double in that test.
Both tests pass ASan/UBSan and separate ThreadSanitizer builds. The Aurora link
audit now reaches 52 unresolved symbols, down from 59; the full game is not
executed. Actual GPU completion, EFB copying, display presentation and full-game
startup remain unfinished.

Both Apple app builds pass with the retrace service included (macOS ad-hoc
signed; iOS unsigned). Aurora's render-mode snapshot now uses a mutex because
HSD can configure it from the retrace callback. The headless `vi_configure`
probe checks concurrent dimension changes without creating a window. Shut down
the retrace service before destroying Aurora's window/rendering environment.

### GX draw-done callbacks after GPU completion

Aurora's FIFO reader now records a draw-done task in the render stream instead
of calling the game immediately. After the containing command buffer is
submitted, the task creates a Dawn queue-completion future. A separate serial
worker waits for successful GPU completion, then invokes the registered callback
under the native interrupt gate. Neither the FIFO nor render worker waits to
acquire that gate for callback delivery. Failed/timed-out fences abort rather
than report a completed frame. Shutdown drains submitted completion work before
joining the worker and destroying the GPU; it requires interrupts enabled.

The recorder has an internal FIFO-only task entry point to avoid recursively
draining the FIFO from its own worker. Ordinary callers retain the existing
draining entry point. A marker is conservatively completed after its containing
frame, including any commands recorded after it. This callback change alone did not implement the blocking SDK calls. The
following section adds those calls; the original game still needs a frame owner
that submits active frames at the appropriate boundaries.

The windowless Metal harness inserts one real `GXSetDrawDone` marker per frame,
drains CPU parsing/encoding, and verifies that its callback has not fired before
submission. It then verifies exactly one callback after completion. The selected
Fox model passes four successive frames with 59 draws each and 12,916 changed
pixels. `native/build/draw-done-selected-fox-4frames.png` is byte-identical to the
previous GPU-completion baseline (SHA1
`36cbd063f48ea213b6626ac7eda055fd2bc0e337`). The all-parts Fox scene also passes.
All 16 Aurora port integration tests pass. The full-game audit still reports 52
unresolved symbols; no full-game executable was run. GPU testing required access
outside the sandbox because the sandbox exposes no Metal adapter; it created no
window or input events. No new iOS graphics execution is claimed by this test.

### Blocking SDK draw waits and submission ownership

`GXWaitDrawDone` now drains CPU parsing to capture the preceding marker count,
then waits until those GPU fences **and their callbacks** have completed.
`GXDrawDone` emits a marker through `GXSetDrawDone` and uses that same wait.
Waiting temporarily releases the native interrupt gate and restores the caller's
prior state afterward. Waiting from inside a draw-done callback is rejected.
A bounded timeout reports failure rather than returning false completion.
Repeated waits for an already-completed marker do not create additional work.

The frame owner can install `aurora::gx::fifo::set_draw_done_submitter` to submit
its active recording frame when `GXSetDrawDone` emits a marker. Without this
hook, the external renderer is still responsible for submission; waiting on an
unsubmitted frame cannot complete. The hook executes on the calling thread and
must respect the renderer's frame ownership. This is not yet installed by a
full-game Apple host.

The windowless harness uses the hook to submit each of the first three frames
in a four-frame run. It calls the actual `GXDrawDone` with interrupts disabled,
checks the preserved interrupt state and exactly one callback, then calls
`GXWaitDrawDone` again before starting the next frame. The final frame retains
the separate pre-submission callback check. The selected Fox run passes with
59 draws per frame and a byte-identical baseline PNG at
`native/build/draw-wait-selected-fox-4frames.png`. All 16 Aurora integration tests
pass. The full-game link audit now reports 51 unresolved symbols, including
`GXSetCopyClamp`; full gameplay and physical presentation remain untested.

### Persistent native framebuffer images

`native/aurora/xfb.cpp` replaces Aurora's empty display-copy functions with
source/stride/scale/filter/gamma configuration and a first GPU copy path.
`GXCopyDisp` drains preceding CPU commands, resolves the EFB region into an
owned RGBA texture, and performs the requested full-EFB clear using the existing
color/alpha/depth update masks. The original framebuffer pointer remains a
full-width opaque identity. Each copy creates a new image, so retaining an
older `melee::xfb::image(address)` handle keeps that image intact when the same
address is reused. `release(address)` removes the mapping; frame packets and
consumers retain their own references. GPU shutdown clears the remaining map.
Consumers must wait for the corresponding draw-done fence before using an image.

This first path accepts unity vertical scale, identity filtering, no AA, gamma
1.0 and a native-resolution EFB. It rejects unsupported modes explicitly,
including overlapping destination rows and partial-rectangle clears. Clamp
configuration is stored and the SDK texture-copy clamp bits are updated; the
identity-filter path does not need samples beyond the source rectangle.
`GXSetDispCopyYScale` returns the SDK's quantized line count rather than zero,
including 240 lines at 2x -> 480 and 480 lines at 1.5x -> 722. Computing that count
does not imply that scaled GPU copies are implemented.

These are native RGB images, not packed GameCube YUYV bytes. CPU framebuffer
writes/readback, YUV conversion, filtering, gamma, AA, scaled copies and split
frame assembly remain unfinished. The images are not yet consumed by an Apple
presentation surface.

The four-frame selected-Fox Metal test now reads back the actual `GXCopyDisp`
image. It copies with clear enabled, retains that image, copies the cleared EFB
to the same address again, removes the address mapping, and verifies both owned
images after GPU completion. The first image matches the previous baseline
exactly (59 draws, 12,916 changed pixels; SHA1
`36cbd063f48ea213b6626ac7eda055fd2bc0e337`); every pixel of the second image has
the expected clear RGB. Output: `native/build/xfb-selected-fox-4frames.png`.
All 16 Aurora integration tests pass. The full-game link audit reaches 50
unresolved symbols, with the remaining missing GX entry point resolved at link
time. That count does not establish complete GX behavior or playable gameplay.

### Original HSD video with the real GPU and retrace clock

The offscreen scene harness now initializes original `HSD_VIInit` before drawing
and finishes through `HSD_VICopyXFBAsync`, the original draw-done callback, and
original pre/post-retrace callbacks. It checks WAITDONE before submission,
waits for the actual GPU fence/callback, and verifies that HSD reaches DISPLAY
with the expected framebuffer identity and black disabled. The first framebuffer
image remains mapped until that handoff completes. A separate snapshot of the
cleared EFB is also verified. DISPLAY here is HSD's internal state, not evidence
of physical display presentation.

Native-only interrupt gating now protects the complete HSD video initialization,
callback replacement (including reading the previous callback), draw-wait state
updates, and configuration/black setters. The original GameCube paths retain
their previous behavior. This avoids exposing partially initialized or updated
state to the native VI and GPU completion workers.

The selected-Fox four-frame run passes with original HSD instrumented by
ASan/UBSan and in a separate `hsd_scene_probe_tsan` build. The TSAN target
instruments the original HSD/scene C units; it does **not** instrument all of
Aurora or Dawn and is not a claim that the entire renderer is race-free.
Both GPU outputs match the existing baseline exactly:
`native/build/hsd-video-selected-fox-4frames.png` and
`native/build/hsd-video-tsan-fox.png`. The native HSD video tests also pass
ASan/UBSan and TSAN, and all 16 Aurora integration tests pass. The full-game
link audit still reports 50 unresolved symbols and has not run the game loop.
Frame conversion modes, a physical presentation surface, and full-game startup
remain outstanding.

### Native card storage core

`native/src/card_store.c` adds persistent storage for the forthcoming SDK card
adapter. It provides 127 directory slots and 251 usable 8 KiB blocks, with
create/find/read/write/delete/rename, metadata, free-space queries and format.
Game filenames are stored as directory data, never interpreted as filesystem
paths. File indices survive reopening and freed indices can be reused. Banner,
icon and palette offsets follow the original SDK status calculation.

The private `MLCARD` version-1 container uses explicit big-endian directory
fields and a CRC32 over directory/payload bytes. It is **not** a raw GameCube
card image or GCI export. Existing invalid files are rejected, including when
creation is requested; they are never silently reformatted. Writes serialize a
candidate image, fsync a temporary file, and atomically replace the prior image
before committing the in-memory mutation. Failed writes retain the prior state.
A sidecar `.lock` file prevents two cooperating store handles from opening the
same path concurrently. It remains after close; the lock itself is released.
This is atomic replacement with file fsync, not a guarantee of directory-entry
durability through every power-loss scenario.

`test-card-store` passes ASan/UBSan and a separate ThreadSanitizer build. Tests
cover reopen persistence, 32-byte names, metadata/icon offsets, capacity limits,
slot reuse, alignment/bounds errors, concurrent reads/writes, lock exclusion,
rollback after the parent path moves, and rejection of corrupted data without
overwriting it. The regression suite and both Apple app builds pass. The app
compiles this core but does not open a card yet. `CARDInit`, mount/probe behavior,
async callbacks and game-save integration still need the SDK adapter; the
full-game link audit remains at 50 unresolved symbols.

### Game CARD API over native storage

`native/src/card_backend.c` now implements the 21 CARD entry points referenced
by the full game, plus synchronous counterparts used by the integration tests.
Slots start empty. The host explicitly inserts a private card container with
`melee_card_insert`; probe distinguishes presence from mounting, and unmounted
or absent slots reject file operations. Repeated `CARDInit` preserves slot state.

Each slot has one I/O worker and allows one outstanding operation. Names and
metadata are copied at submission; caller data buffers and output structures
must remain valid until completion, as with the SDK. File I/O runs outside the
native interrupt gate. Completion publishes the result and invokes the callback
under the gate, allowing the callback to enqueue the next asynchronous operation.
Synchronous waits release/reacquire the gate and preserve the caller's previous
interrupt state. Synchronous I/O inside a card callback is rejected to prevent a
worker waiting for itself. Eject/shutdown reject active operations; there is no
in-flight cancellation implementation.

The adapter covers mount/check, create/open/close, read/write, rename/delete,
status updates, free space, format and transferred-byte queries. Writes update
the save timestamp in the same atomic transaction as payload bytes. Check
revalidates the stored container, including its CRC, and an explicitly requested
format can recover a mounted container corrupted after insertion. A mount I/O
failure leaves the slot unmounted. Native insertion still rejects pre-existing
invalid containers without overwriting them. `CARDFileInfo.iBlock` is an opaque
logical cursor here, not a physical FAT address in a raw GameCube image.

`test-card-backend` passes ASan/UBSan and ThreadSanitizer. It exercises an actual
mount -> create -> write -> read callback chain, waits with interrupts disabled,
both slots, busy/error returns, persisted reopen, metadata offsets, I/O rollback,
failed mounting, corruption detection, format recovery and worker shutdown/restart.
The regression suite and Mac/iOS builds pass. The Apple preview app compiles the
adapter but does not insert cards or run Melee's save menus. Full original-game
save serialization and menu behavior remain unverified. The full-game link audit
now has 29 unresolved symbols, down from 50; the game loop has not been executed.

### Development-adapter fallback

`native/src/devkit_backend.c` implements the referenced MCC/FIO APIs for an
Apple host with no GameCube HIO development adapter. Enumeration succeeds with
zero devices; initialization and channel operations return the original SDK's
no-device/uninitialized errors. Callbacks are never invoked and read buffers
remain untouched. Remote file operations fail without accessing host files.
`FIOQuery` returns immediately rather than spinning for five seconds waiting for
hardware that cannot appear. Last-error storage is atomic.

`test-devkit-backend` links the original HSD development-device probe and checks
its fallback, enumeration, error codes, callback exclusion and unchanged buffers
under ASan/UBSan. Both Apple preview builds pass with this backend included.
The full-game link audit now reports 11 unresolved symbols: OS thread checking,
reset handling, stack diagnostics and THP movie decoding. The full game still
does not link or run; this fallback does not provide development-adapter support.

### Native thread diagnostics

`db_PrintThreadInfo` now queries Darwin's current-thread stack bounds and reports
current frame usage with full-width addresses. It does not scan stack memory or
claim a peak measurement: the SDK's fill-sentinel scheme does not exist on native
threads. The console implementation remains unchanged. The per-frame debug-only
`OSCheckActiveThreads` call is console-only, because it validates internal SDK
scheduler queues that the Darwin pthread runtime does not use; no replacement
claim of whole-process scheduler integrity is made.

`test-thread-diagnostics` passes ASan/UBSan on the main thread and a worker with
an explicitly sized stack. Both changed game units compile for iOS arm64, and
the macOS full-game audit now reaches the linker with eight unresolved symbols:
two reset APIs and six THP movie-decoder entry points. The game has not run.

### Bounded THP image decoder core

`native/src/thp_decode.cpp` adapts the portable decoder from pinned Aurora
749d6ee7a22bdfab78c8ece9047bca5d79aa72ca (MIT license retained in
`native/licenses/aurora.txt`). Its separate C interface accepts an explicit
compressed-byte length and output capacities. Header and entropy reads are
bounded, dimensions are limited to 4096, and failed decodes leave output buffers
unchanged. Successful output is three GX I8 tiled Y/U/V planes. This core uses
no Apple UI, GPU or libjpeg dependency.

`make -C native test-thp-decode` exercises a synthetic neutral MCU, insufficient
output capacity and truncated-input rollback under ASan/UBSan. The core also
compiles for iOS arm64. The independent pixel oracle uses the locally installed
libjpeg-turbo only for testing:

```sh
make -C native build/verify-thp-pixels
python3 native/tools/verify_thp_retail.py '/path/to/your-game.ciso'
```

All 75 retail still images and 20 evenly spaced frames across the 3,036-frame
opening movie pass: every decoded component sample differs from libjpeg's
floating-point IDCT by at most one level. The verifier restores standard JPEG
entropy-byte stuffing for the independent decoder and compares untiled samples.
It also validates all opening-movie packed frame extents. This establishes a
cross-decoder comparison, not bit-exact GameCube IDCT fidelity or movie playback.

The core is not yet connected to Melee's two-phase decoder API or Apple app.
Melee's THP declarations and player currently pass pointers through 32-bit
integers, and movie headers/stream buffers still need native ownership and
endian handling. Those must be fixed before connecting this decoder. Full-game
link status remains eight unresolved symbols; the game is not playable.

### Original ending-image loader integration

The native branch of `lbMthp8001FAA0` now calls the bounded decoder directly,
passing the byte count returned by `lbFile_80016760`. It verifies the requested
dimensions against the image, allocates complete GX tile storage for all three
planes and preserves full-width pointers. Compressed data and plane allocations
retain the original game heap lifetime. The console branch remains unchanged.
The full-game Aurora target now includes the C++ decoder core.

`make -C native test-game-thp-still` calls this original loader with a synthetic
image. Its test supplies the file/heap boundary and compares all three planes
with the standalone decoder; it does not replace any THP decoder entry points.
It passes ASan/UBSan, including a full-width output-address check. The same test
also passes for the retail 560x416 Fox ending still and a 640x480 opening frame.
The modified game unit compiles for iOS arm64. This is loader/plane validation,
not a test of file I/O, on-screen presentation or a played ending sequence.

The full-game link now has seven unresolved symbols. The streaming movie player
still needs native pointer, header and buffer ownership handling before its
remaining THP calls can be replaced. Reset handling and game startup remain
outstanding, and the Apple preview is still not a playable game.

### Native movie header loading

The native `fn_8001EB14` path now reads into a 32-byte-aligned byte buffer using
a full-width destination address, then explicitly converts the MTHP header's
big-endian fields. `melee_mth_header` checks the version-2 format, dimensions,
frame count/rate, buffer size, unsupported offset tables and first-frame extent
against the actual file length. Failed parsing leaves its output unchanged.
The native path no longer initializes the unavailable SDK THP hardware decoder.

`test-mth-header` runs the original header-loading function with a test file-I/O
boundary. ASan/UBSan checks pass for synthetic/truncated headers and all 28
retail movie headers from the supplied disc. The changed game unit compiles for
iOS arm64. This validates header loading only: later streaming buffers still
contain 32-bit pointer casts and frame-size words needing endian conversion.
The full-game link audit is down to six unresolved symbols; no movie playback
or game startup is claimed.

### Original movie stream and native frame decoding

The native player now stores full-width frame addresses, allocates its pointer
table using the host pointer size, and records the compressed byte count for
each of its 32 slots. Preload assigns every slot before placing the output
planes, handles movies shorter than the ring, and reads next-frame lengths as
big-endian words. Every preload/refill validates its file extent and slot
capacity. The native decode path checks dimensions and passes the recorded
frame length directly to the bounded decoder; SDK scratch-context pointers are
no longer used.

Slot stride includes the four-byte packed-frame prefix and alignment padding.
The full opening test found that retail frame 2,650 occupies 61,184 bytes while
the header's nominal buffer size is 61,152; the extra aligned block is necessary
to avoid overwriting the next slot. This case now passes with bounds checking.

`make -C native build/test-mth-stream` builds an ASan/UBSan integration test
that calls the original preload, frame advance, decode and refill callback
functions. Its file-I/O boundary copies from an extracted MTH file, and its
deferred callbacks run deterministically on the test thread. With `MvOpen.mth`,
the test passes all 3,036 frames and 3,004 refill callbacks. Passing a second
argument enables looping: 3,076 decoded frames and 3,075 callbacks pass across
the loop boundary. A two-frame fixture also passes both modes. The 28-header
test still passes, and the changed game unit compiles for iOS arm64.

These tests do not prove real asynchronous I/O races, alarm-driven timing,
audio synchronization, shutdown waits or physical presentation. The full-game
link now has only `OSGetResetCode` and `OSResetSystem` unresolved. It has not
executed the game, and the Apple preview remains unplayable.

### Reset handoff and first complete ARM64 link

`native/src/reset_backend.c` supplies the boot reset reason and transfers reset
requests to a configured native host handler. The request preserves restart,
hot-reset or shutdown kind, reset code and console-menu intent. It releases the
native interrupt gate before the handler takes control. The handler must not
return into the terminating game; missing handlers, duplicate reset requests
and returning handlers fail explicitly. Host configuration is only valid
before startup or after the prior game and its workers have stopped.

`test-reset-backend` passes ASan/UBSan for all three request kinds, boot reasons,
request data and a game-thread exit into a joining test host. It also verifies
that a reset initiated with interrupts disabled does not strand the gate.
This is a lifecycle interface, not implemented restart of the whole runtime:
the Apple host still needs worker teardown, new game state construction and
handling of console-menu intent.

The full original-game Aurora target now links as an ARM64 executable with
**zero unresolved symbols**. The audit does not run it. The entry point still
needs a configured disc, graphics/frame submission, audio and host lifecycle;
there is no evidence yet of a working startup, menus or playable match.

### First original-game startup execution

The native entry is now `melee_game_main`, callable by a host. A separate tiny
entry preserves the link-audit executable. `melee_game_startup` mounts the
supplied disc, initializes a windowless Metal/Dawn renderer, installs a reset
exit handler and submits frames at the original GX draw-done boundary. It
creates no SDL video window or surface and performs no desktop interaction or
audio playback. It exits after 120 GPU submissions or a 30-second watchdog;
process exit here is not a tested runtime teardown/restart.

```sh
cmake --build native/build/aurora-integration --target melee_game_startup -j8
native/build/aurora-integration/melee_game_startup '/path/to/your-game.ciso' native/build/startup-gpu-cache
```

On the M4 Max, the original startup executes and prints its boot information,
including disc language, 64 MiB arena and ARAM capacity. Native startup selects
the SDK's progressive 480-line mode, avoiding the interlaced deflicker copy
filter on progressive host displays. Runtime execution exposed and fixed a
truncated boundary in `lbHeap_80015900` and fixed-offset free-list initialization
in `lbMemory_8001564C`. The native list now uses struct fields; its focused
ASan/UBSan test allocates/releases all five host-heap handles and the ARAM handle.
The changed game units also compile for iOS arm64, and the full link still passes.

The current startup probe **times out before its first GPU submission**. A CPU
stack sample identifies `lbDvd_GetPreloadedArchive`, called through
`lbArchive_80017040` from game-mode initialization, as the active wait. Logs and
the sample are in ignored `native/build/game-startup-{run.log,stack.txt}`.
This is the first runtime startup evidence, not successful startup, movie
presentation, menus or playability. The next issue is the archive preload wait.

### Preload wait resolved; original archive parser reached

The preload wait came from persistent HSD I/O request nodes allocated in the
transient main game heap. Scene heap reconstruction could overwrite the nodes.
Native request pools now use process-lifetime host allocations, preserving
their original persistence across scene changes. Native DVD open/read handoffs
also assert on rejected requests instead of leaving the HSD queue permanently
busy. A diagnostic snapshot can report preload state under the interrupt gate.

`test-devcom` now destroys, overwrites and recreates the scene heap between two
complete transfer/callback sequences. That regression and a real Fox archive
read from the supplied disc pass ASan/UBSan. Startup then exposed truncated
addresses in the secondary allocator's search for the next free region; those
calculations now preserve pointer width. The allocator test verifies successive
allocations, gap reuse, contents and free-space accounting above 4 GiB.
Both Apple preview builds pass and the full-game link remains complete.

The startup probe now gets past preload into the original `HSD_ArchiveParse`.
It stops there with a verified endian mismatch: `LbRb.dat` has length 1,045
(`0x415`), but the original parser reads its big-endian header as `0x15040000`.
The separate native archive tools already decode headers explicitly; the game's
original archive API and its returned data still need integration with native
archive ownership and relocation handling. No rendered game frame or playable
startup has been verified.

### Typed rumble archive integration

`rumble_bank.c` converts the `lbRumbleData` table into owned native
`Fighter_804D653C_t` entries and host-endian 16-bit scripts. It validates archive
relocations, script bounds and supported control flow, including the retail
script that replaces the interpreter's single loop register. No pointer into
the source DAT survives conversion. Existing rumble lists are removed before
replacing an owned bank, and the native game entry checks requested indices.

The original rumble loader obtains immutable bytes from the preload cache (or
its normal file-loader fallback) and calls this typed converter. It does not
pass big-endian pointer-bearing data through the original in-place relocator.
The general original archive API remains unported.

`test-rumble-bank` passes ASan/UBSan with a synthetic script and all 40 retail
scripts. After source bytes are overwritten and freed, the scripts run through
the original interpreter for 1,209,437 frames in total. Invalid opcodes are
rejected. This verifies interpreted rumble data, not physical controller motors.

The original windowless startup now passes rumble loading and enters
`gm_Scene_MemCard_OnEnter`. Its next failure is the unported archive parser in
`lbCardGame_LoadArchive`, which loads `LbMcGame.` (`MemCardIconData`) and then
`NtMemAc` (`ScNtcCommon_scene_data`). It still stops before a GPU submission;
neither the scene nor a playable game has been rendered.

### Owned memory-card image assets

`card_icons.c` converts `MemCardIconData` into four owned, full-width native
addresses. It checks the pointer relocations against the named exports and
requires the retail layout: three 6,144-byte banners and one 1,536-byte icon
with its palette. Packed image bytes retain their original byte order for
card serialization. The converter retains no source archive pointers.

`make -C native test-card-icons` checks synthetic assets under ASan/UBSan.
The same test accepts an extracted `LbMcGame.dat` or `LbMcGame.usd`; both
retail variants pass byte-for-byte checks after the source buffer is destroyed.
Invalid relocation targets are rejected.

This converter is not yet connected to `lbCardGame_LoadArchive`. The original
save-command queue carries image addresses through 32-bit integer fields and
fixed-offset storage; those producers and consumers need a coordinated native
conversion. `NtMemAc` also requires typed scene descriptors. The existing CARD
storage adapter tests do not validate this higher-level save pipeline.

### Owned camera descriptors

`camera.c` reads big-endian camera descriptors into owned native
`HSD_CObjDesc` structures, including viewport, scissor, projection parameters,
eye/interest world objects and an optional up vector. Perspective, frustum and
orthographic projections are supported. Custom classes and world-object
constraints are rejected; camera animation remains a separate scene resource.
All referenced descriptors survive destruction of the source archive.

`test-camera` exercises the projection variants and rejects invalid projection
types, non-finite values and out-of-bounds world objects under ASan/UBSan.
Passing an extracted `NtMemAc.dat` or `NtMemAc.usd` tests the retail memory-card
camera; both variants preserve the expected 640x480 viewport, clipping planes
and eye position. The converter also compiles for iOS ARM64. This is descriptor
conversion evidence, not a rendered camera or integration of the complete
memory-card scene into startup.

### Owned light descriptors

`lights.c` converts linked `HSD_LightDesc` records into owned native structures,
including world positions, interest points, colors and light parameters.
It supports ambient, infinite, point and spot lights, including raw attenuation
coefficients. Custom classes, world-object constraints, cyclic chains, invalid
references and non-finite parameters are rejected. Animation descriptors are
separate resources and are not converted by this API.

`test-lights` covers linked ownership after source destruction, point/spot
attenuation variants and invalid inputs under ASan/UBSan. Both retail
`NtMemAc.dat` and `NtMemAc.usd` pass checks for their ambient and infinite
lights. The converter compiles for iOS ARM64. It is not yet connected to the
original scene loader, and these tests do not establish rendered lighting.

### Native scene descriptor ownership

`scene_desc.c` assembles native `SceneDesc` records from the owned model,
camera and light converters. It preserves the retail memory-card scene's
material animation and its empty camera/light animation records. Nonempty
camera/light animations, skeletal/shape animations, fog and multiple material
animation choices are currently rejected. The owner must outlive all HSD
objects created from its descriptors.

The model decoder now also converts material color, alpha and pixel-engine
animation channels through the existing bounded track decoder. The camera
matrix-dirty flag uses an unsigned shift to avoid signed-shift undefined
behavior exposed by loading the retail camera.

`hsd_scene_probe --scene-desc PATH` loads either retail `NtMemAc` archive,
destroys the source bytes, and invokes the original HSD joint, camera and light
loaders. Both variants produce two joints, four drawables, four polygons and
two lights. The test observes changing diffuse color over the material's
20-frame cycle, rejects unsupported camera animation data repeatedly, releases
the loaded objects and verifies HSD allocation counters return to zero. The
tests run with ASan and UBSan and are registered with CTest when the extracted
archives exist. The new converter and changed model converter compile for iOS
ARM64. No window or GPU presentation is involved in these scene-loader tests.

`lbCardGame_LoadArchive` now routes its `NtMemAc` load through this owner on
native builds. It obtains immutable raw bytes from the preload cache or the
normal file-loader fallback and replaces the previous owner after scene
teardown. Validation objects are released during conversion; retained
descriptors and animation streams use host allocations independent of the HSD
scene heap. The probe checks HSD counters are zero before creating the actual
scene objects from those descriptors.

Startup still encounters the earlier `LbMcGame` icon archive load first. Its
integration and the save-command pointer conversions remain unfinished, so
the new `NtMemAc` call path has not yet been reached by full-game startup.

### Native outer card request queue

The original card request producers now use a shared `HSD_CardWord` layout and
dedicated 32-entry native storage. State, filename, data and callback addresses
are retained at pointer width. Queue reset and dispatch select the same
storage rather than relying on neighboring GameCube BSS symbols. The delete
request producer accesses `CardState.file_info` through its native fields.

`test-card-requests` executes all six request producers under ASan/UBSan and
checks pointer/callback preservation, capacity, wraparound, reset and the
delete producer's open/close retries. Its CARD open/close boundaries are test
stubs; it does not execute a save. The dispatcher still calls legacy block
command functions with unported integer address parameters, and the inner
128-entry command buffer/context remains unported. These changes establish
outer queue storage only; icon integration and end-to-end saving remain open.

### Native inner card command storage

The card context prefix and 128-entry inner command buffer now occupy one
dedicated native allocation. Command records, staging words, callback field
access and queue copying use the same pointer-width stride, checked with
compile-time layout assertions. Native staging arrays have nine words to
match the enqueue copy; the former eight-word arrays could be over-read.
Stack command records are initialized before enqueueing.

`test-card-requests` also checks all 128 command entries, full-queue rejection,
wraparound, neighboring context preservation and a real typed command
producer under ASan/UBSan. State and payload addresses above 4 GiB survive
copying. The changed command code compiles for iOS ARM64 and the full game
links. This tests queue storage, not execution of every command: several
downstream function parameters and local address calculations still truncate
pointers and must be converted before connecting the icon loader or validating
an end-to-end save.

### Card address propagation and startup integration

Card request dispatch, read/write helpers, header-block staging and callback
context fields now propagate native address words. Buffer arithmetic uses
native addresses; an old block-count helper's integer-as-pointer expression
is now ordinary integer arithmetic. Tests follow comment/banner/icon pointers
through the real header-block helper chain and preserve state/callback
addresses through delete-command construction.

The game-level card APIs and stored banner/icon fields now use the same
address-width type. `lbCardGame_LoadArchive` loads `MemCardIconData` through
the owned icon converter and then uses the native `NtMemAc` scene converter.
Both take immutable preload bytes or the ordinary file-loader fallback and
retain independently owned host data. Save serialization, command execution
and persistence still require end-to-end validation; successful pointer tests
do not establish a working save feature.

The original windowless startup now passes these two memory-card archives.
Its next observed failure is `gm_801AEBB0` loading `NtMsgWin.dat` through the
unported original archive parser (header length `0x2e6f` read as `0x6f2e0000`).
It still stops before a GPU submission or playable game frame. The run log is
`native/build/card-native-startup-run.log`.

### Message-window scene integration

The native scene converter now supports one ordinary SRT animation tree per
model, with owned joint descriptors and bounded copied tracks. It rejects
constraint/path animation and unsupported flags. Camera lists terminate at a
null descriptor pointer; a sentinel does not require a second null word.
`lbArchive_NativeLoadScene` shares the preload/fallback and ownership logic
between the memory-card and message-window scene loaders.

`hsd_scene_probe --message-scene native/build/NtMsgWin.dat` loads the original
scene after destroying its input bytes: two models, 21 joints and nine
polygons. It observes joint animation across 60 sampled frames, rejects a
cyclic animation tree repeatedly, and releases all tested HSD allocations.
The test runs under ASan/UBSan and joins the HSD CTest suite when this extracted
asset exists. Changed converters and original loader units compile for iOS
ARM64.

Original windowless startup now passes `NtMsgWin.dat` and stops at
`HSD_SisLib_803A62A0` loading `SdMsgBox.usd`, whose big-endian archive header
still reaches the original parser. The text resource and subsequent rendering
remain unfinished. No game frame has been submitted; this is not yet playable.
The latest startup log is `native/build/message-native-startup-run.log`.

### Native SIS text archive ownership

`sis_bank.c` converts flat SIS pointer tables into full-width host arrays and
copies their packed font and command data unchanged. It validates each table
relocation and preserves one-past-end targets without dereferencing them.
The Japanese message bank contains actual font data; the US bank uses minimal
font-data placeholders. Tests cover both retail `SdMsgBox` variants and a
synthetic bank, including source destruction and malformed relocations.

The original `HSD_SisLib_803A62A0` now uses this converter on native builds.
Retired banks remain owned until the associated font or scene is cleared,
so text objects can retain their original stream pointers. This handles
archive ownership; command interpretation and rendered glyphs still need
runtime verification.

The windowless game startup now passes message text loading and reaches
`gm_Scene_MemCard_OnFrame`. It stops during trophy initialization in
`Toy_803124BC`, which still loads its data through the original archive parser.
No GPU submission or playable frame has been verified. The latest trace is
`native/build/sis-startup-run.log`.

### Trophy tables and first original-game GPU submission

`trophy_data.c` converts all seven `TyDatai` tables into owned native scalar
records. Integer/float fields change byte order; packed byte fields remain
unchanged. Table bounds, strides, terminal sentinels and finite floats are
checked. Synthetic data and both retail language variants pass ASan/UBSan
checks for every field and source-buffer lifetime. The original trophy
initialization functions load these tables and release them on scene reset.

The original windowless startup now passes trophy initialization and reports
`Original game GPU submission 1 completed`. It then crashes in
`HSD_SisLib_803A8134` during text measurement, called by the SIS renderer.
That routine still truncates kerning addresses and reads packed glyph codes
with native-endian loads. The crash stack is captured in
`native/build/trophy-startup-stack.log`; the run log is
`native/build/trophy-startup-run.log`.

One submitted GPU command batch does not establish a correct rendered frame
or playability. Text measurement/rendering, continued startup and end-to-end
input, audio and gameplay verification remain unfinished.

### Original memory-card screen rendered natively

The SIS measurement and renderer now read packed 16-bit glyph/style values
explicitly in big-endian order. Kerning calculations retain full-width
addresses and perform ordinary integer arithmetic on glyph margins. Fixed
point style-stack bytes are produced through an integer conversion before
truncation to a byte. `test-sis-measure` runs the original measurement function
under ASan/UBSan with default/custom metrics, scale restoration and signed
spacing restoration. SIS pointer-call commands and their return stack still
need separate pointer-width work.

The original windowless startup completes 120 GPU submissions without the
text crash. Its optional third argument after the image/cache paths captures
the latched framebuffer as a PNG using Metal readback. The observed frame in
`native/build/original-startup.png` displays the original “There is no Memory
Card in Slot A” prompt, with readable text and warning graphic; VI reports
black=false. This is an offscreen artifact, not a screenshot of the host Mac.

The capture run ends at its explicit submission cap. Advancing the prompt,
menu/gameplay behavior, interactive Apple hosts, audio and complete shutdown
remain unverified. This screen establishes native rendering, not playability.
The capture trace is `native/build/startup-capture-run.log`.

### Scripted input advances into the opening scene

`melee_game_startup IMAGE CACHE OUTPUT_PNG --confirm` connects a virtual
controller on port one and publishes A-button presses at submissions 180
and 300, releasing each six submissions later. It uses the native PAD backend
and original controller processing; it neither reads host devices nor posts
host input events. This mode has a 600-submission cap and retains the existing
30-second process watchdog.

The first press advances the original UI to “Continue without saving or
loading Game Data?” with OK selected (captured in
`native/build/startup-confirm.png`). The second press advances into
`gm_Scene_Opening_OnEnter`. The observed run stops after submission 302 while
`gmTitle_801A1AC0` loads `GmTtAll.usd` through the original archive parser.
That archive contains title/background joint and animation resources, camera,
lights, fog and a screen-object descriptor. Native conversion of those assets
remains unfinished. The latest trace is `native/build/startup-continue-run.log`.

This verifies original UI navigation through virtual-controller input. It
does not establish playable menus/matches or an interactive Apple application.


### Title model animations on ARM64

The scene owner now preserves empty shape-animation hierarchies and accepts
texture transform, blend, LOD and TEV-color channels alongside image/palette
selection. LOD/TEV tracks require the corresponding texture descriptor;
nonempty morph tracks remain unsupported and are rejected. Shape decoding is
bounded by the matching joint/drawable hierarchy and rolls back allocations
on failure.

`hsd_scene_probe --title-models native/build/GmTtAll.usd` loads both original
title models, binds their joint/material/empty-shape trees, releases the
validation objects, destroys the input archive, and recreates original HSD
objects from the retained descriptors. Sampling through frame 1600 verifies
changing joint matrices and texture state in each model. Malformed cyclic
shape trees fail repeatedly, and HSD allocation counters return to zero.
Both `GmTtAll.dat` and `.usd` pass this ASan/UBSan probe; all 18 HSD CTests pass.
The updated scene converter also compiles for arm64 iOS.

This probe does not render or enter the original title scene. The opening
startup still needs a native title bundle loader for the camera, lights, fog
and screen-object descriptor, connected to `gmTitle_801A1AC0`. Its original
archive-parser failure at submission 302 remains the latest startup result.


### Original title bundle loader connected

`melee_title_decode` now owns the title/background models and animations,
camera, bounded light lists and empty light-animation descriptors, fog (with
optional adjustment matrix), and title-mark image. The mark decoder currently
accepts a single nonpaletted, nonmipmapped image; unsupported variants fail.
`gmTitle_801A1AC0` uses this converter in native builds and retains its owner
outside the resettable scene heap. Both callers discard the legacy archive
return value, so native builds return NULL after assigning the converted
exports. Console builds retain the existing archive loader.

Both language variants pass the extended title probe, including repeated
failure/cleanup for malformed fog and out-of-bounds image payloads, plus
preserved image bytes after the source archive is destroyed. All 18 HSD tests
pass. The converter and original title loader compile for arm64 iOS; the full
game link audit reports zero unresolved symbols.

Original startup now passes title archive loading and reaches opening playback.
The next assertion revealed that the original streaming-music header array
lacked its required 32-byte alignment on the native host. Its native declaration
now explicitly aligns all three 32-byte entries. The four existing synth load,
compaction, voice-start and bank-range tests pass after this change. DevCom now
reports transfer arguments when an alignment assertion would fail.

With alignment corrected, the latest windowless run completes submission 303
and fails at `AXSetVoiceMix` with “Invalid native AX voice” during opening
startup (`native/build/startup-title-run.log`). The streaming music header path
still needs investigation; this is not evidence of working opening audio or
playable gameplay. A subsequent CLI LLDB attempt hit the harness watchdog
before reproducing the failure and did not provide its call stack. No new
opening/title framebuffer capture was produced.


### Opening movie runs through 600 native submissions

The streaming-music startup path now converts the HPS main header and each
block header from big endian before using channel counts, sample rate, AX
address/ADPCM fields or block sizes/loop context. Pitch uses separate host
high/low words, and playback position uses the typed AX voice address instead
of a fixed GameCube structure offset. Native callback adapters use the actual
DevCom callback signature. Invalid formats, channel counts, sample rates and
block boundaries fail explicitly.

`test-hps-start` exercises the original header/start/refill callbacks with real
AX voices and a captured transfer boundary. It checks stereo allocation,
32 kHz pitch, initial/current addresses, refill requests, loop context,
mono/stereo block bounds and malformed headers under ASan/UBSan. Running it
with `native/build/opening.hps` also validates all 53 retail opening blocks.
The four existing synth regression tests pass, the full game links with zero
unresolved symbols, and the updated synth compiles for arm64 iOS.

The final windowless startup run (`native/build/startup-hps-run.log`) reaches
its 600-submission limit and writes `native/build/startup-opening.png` with
VI black=false. The frame shows the original Mario opening-movie sequence.
This is original game execution with virtual-controller input and offscreen
Metal rendering. The harness does not output sound to an audio device; these
results do not yet establish audible opening music, title/menu navigation,
playable matches, or an interactive macOS/iOS game application.


### Title screen reached; heap compaction repaired

The windowless harness accepts `--title` to confirm both memory-card prompts
and press A at submissions 450–456 to skip the movie. It captures the original
animated title screen at submission 600. `--menu` adds Start at submissions
600–606 and raises the submission cap to 900; the 30-second watchdog remains.
All input is published to the virtual PAD backend, without host input devices.

The first title-transition tests exposed an intermittent heap-compaction bug:
`lbMemory_80015320` truncated host addresses to 32 bits, sometimes classifying
host RAM as ARAM and issuing an invalid transfer. Compaction now retains
full-width addresses and callback pointers. Its native alarm callback uses
`memmove` for overlapping chunks. `test-lbmemory-handles` verifies a preserved
prefix, an overlapping 200,000-byte move across two chunks, a following block,
full-width callback chaining, completion and the already-compacted fast path.
That test, native diagnostics and ARAM scheduling tests pass; the compactor
also compiles for arm64 iOS. Native OS panics now include an Apple backtrace,
and rejected ARAM requests report their parameters for future diagnosis.

The corrected `--title` run reaches 600 submissions and captures
`native/build/startup-title-screen.png` (`native/build/startup-title-fixed.log`).
A subsequent `--menu` run also passes the movie-to-title transition, then
advances into main-menu initialization. It stops after submission 601 in
`lbSnap_8001E218`, whose `LbMcSnap.usd` archive still uses the console parser.
This archive exports `MemSnapIconData`, one 6,144-byte banner and one 1,536-byte
icon/palette payload. Its native owner and snapshot pointer table are the next
integration work. The trace is `native/build/startup-menu-run.log`; no menu
framebuffer was produced. The game is not yet playable.


### Snapshot card icons loaded natively

The card-icon owner now supports the snapshot archive's one-banner/one-icon
schema through `melee_card_snapshot_icons_create`, while preserving the
existing three-banner game-card schema. `lbSnap_8001E218` loads immutable raw
archive bytes, retains owned payloads outside the scene heap, and publishes a
full-width snapshot icon pointer table. Its card-operation pointer arguments
now use `HSD_CardWord`. The native snapshot data-offset calculation also avoids
truncating pointers before subtraction. Snapshot save/load itself remains
unverified.

Synthetic tests cover both schemas, malformed pointer/sentinel rejection,
bounded indices, and all retained pixel/palette bytes after source disposal.
Both retail language variants of `LbMcGame` and `LbMcSnap` pass. The converter
and snapshot source compile for arm64 iOS, and the full game link audit reports
zero unresolved symbols.

The original `--menu` startup now passes snapshot initialization. It next fails
in `mnMain_Scene_OnEnter` when the original parser opens `MnMaAll.dat` (trace:
`native/build/startup-snapshot-run.log`). All exported `_Top_joint` models in
that archive individually load through the current HSD scene probe. Their
animation bundles, shared scene descriptors and on-demand archive lookups need
native integration before the main menu can run. No menu framebuffer or
playable match has been produced.


### Native main-menu archive adapter

`melee_menu_decode` owns the `MnMaAll.dat` archive and converts its models on
first lookup. The native HSD archive adapter retains original public-symbol
metadata and routes `HSD_ArchiveGetPublicAddress` through an explicit callback;
submenus can reuse model descriptors across HSD scene-heap resets. Native
archive destruction releases the model/environment owners. The original
`lbArchive_LoadSymbols("MnMaAll", ...)` entry point now uses this adapter.
The six name-entry tables remain unsupported and return NULL rather than raw
GameCube pointers. Each loaded model currently owns a separate archive copy.

Static camera/light/fog conversion is shared with the title loader through
`melee_environment_decode`. Menu photo models also exposed surplus empty
material/shape entries: these are preserved with bounded traversal, matching
the original HSD behavior that ignores entries beyond its drawable list.
Nonempty unmatched material entries remain rejected.

The menu probe resolves and animates all 57 model bundles through original
HSD lookups after disposing of the source buffer, verifies stable lookup
identity, and checks that HSD allocation counts return to zero. Repeated lazy
loads with a cyclic material list fail and clean up without corrupting the
valid adapter. All 19 HSD tests pass; the adapter, environment/title modules
and original archive integration compile for arm64 iOS. The full game link
audit reports zero unresolved symbols.

Original `--menu` startup now passes artwork and SIS text loading. It next
stops after submission 601 in `gm_801BA8FC`, loading `GmEvent.dat` through the
console parser (`native/build/startup-menu-native.log`). That archive exports
`sqEventInitDataLevelTbl` and needs conversion of its event configuration
tables. No rendered main-menu frame or playable match has been verified.


### Original main menu running on ARM64

The native event-data loader converts all 51 `GmEvent.dat` records, including
rule bitfields, player/stage/bonus records and the event-specific auxiliary
layouts. Relocated offset zero is handled as a valid pointer. The audio-load
loader converts the four locale tables in `LbAd.dat`, preserving shared sound
lists and validating their terminators. Both owners survive disposal of the
source archive; synthetic and retail tests pass with address/undefined-behavior
sanitizers. Both converters and their game integration compile for arm64 iOS.
The full game link audit reports zero unresolved symbols.

The first main-menu update exposed an original four-entry local pointer array
used for five or more menu options. The native declaration now follows the
option-map capacity and asserts the selected menu fits. The console declaration
is preserved. `melee_game_startup --menu` now completes 900 GPU submissions and
captures the original main menu at `native/build/startup-menu.png` with VI black
disabled. This is original game logic and artwork rendered by the windowless
Metal harness using scripted virtual controller input, not an interactive host.

The new `--versus` script navigates Down, confirms VS. Mode, then confirms Melee;
it is intended to exercise the transition into character selection. No playable
match or real-device iOS gameplay has yet been verified.


### Instrumented scene-transition checks

`MELEE_AURORA_SANITIZE_GAME=ON` enables AddressSanitizer and debug information
for all original-game objects and links the startup/audit executables against
its runtime. It is separate from the existing HSD-only sanitizer option.

The instrumented startup found and corrected three more native memory-layout
problems: trophy initialization used offsets across separate globals; the
trophy display archive array derived its element count from the console byte
size divided by the host pointer size; and main-menu camera initialization
wrote a Vec3 twenty bytes beyond its local destination. Native trophy writes
now address `Toy_804A284C` directly, the archive array keeps its original 44
entries, and native camera initialization uses its actual local Vec3.
All three modified original source files compile for arm64 iOS.

The `--versus` script now passes 902 GPU submissions without an AddressSanitizer
memory error, exits the main menu and enters `mnCharSel_Scene_OnEnter`. It stops
at the unsupported console parser for `MnSlChr.usd` (trace:
`native/build/versus-asan.log`). That character-selection archive has one
`MnSelectChrDataTable` export at data offset zero, 11,002 relocations and no
externals. Its root supplies camera, two lights, fog and nine model/animation
bundles. The companion `MnExtAll.usd` contains 105 exports for extra menu/name
flows. These need native ownership and conversion before character selection
can render. No playable match is claimed.

The final instrumented `--menu` regression completes 900 submissions, exits
zero, and writes `native/build/startup-menu-asan.png` with VI black disabled.
The captured original main menu was visually inspected. Leak detection is
disabled for this bounded process-exit harness; runtime shutdown is untested.


### Native character-selection scene integration

`melee_character_select_decode` owns the nine `MnSlChr` model bundles and
converts camera, two light descriptors and fog into full-width host data.
Original HSD public-symbol lookup returns the native `MnSelectChrDataTable`.
`mncharsel.c` uses the native animation array instead of adding the console
16-byte camera-table size. `lbArchive_LoadArchive` routes both language variants
of `MnSlChr` through this decoder and `MnExtAll` through the existing menu
adapter. The companion archive has 24 model bundles; its six name-entry tables
remain unsupported as in the main-menu adapter.

The HSD probes load and animate all nine character-selection models and all 24
companion models after disposing of the source bytes. They check stable public
lookups, camera/light instantiation, malformed fog/last-model rejection, cleanup
and allocation counts. All 23 HSD tests pass, including both language variants.
The converters and game integration compile for arm64 iOS.

The first original scene run exposed a SIS allocation fixed at 16 console bytes
for a dynamic text record that occupies 24 host bytes. It now uses the record's
size and initializes its typed counter. Native SIS allocations use pointer
alignment. A regression creates and frees 24 original dynamic text objects
alongside an odd-sized allocation under AddressSanitizer/UBSan; it passes.
The first cursor update also exposed a synthetic composite view spanning
separate icon, door, tag and miscellaneous globals. Native accesses now target
the actual globals; console expressions are retained. Cursor and SIS changes
compile for arm64 iOS.

The final `--versus` AddressSanitizer run completes 1,200 GPU submissions and
exits zero, producing `native/build/startup-versus.png` (log:
`native/build/versus-css-cursor.log`). The original character-selection screen
was visually inspected: roster, UI and P1 hand render, but the player panels
show N/A. Player-slot initialization/selection and progression into a match
remain unverified. This is a windowless virtual-input test, not an interactive
Mac app or iOS gameplay verification.

### Original fighter selection and stage transition

`melee_game_startup --select` uses virtual analog input to move P1 into the
roster and confirm a fighter. It completes 1,320 GPU submissions under
AddressSanitizer and captures Mario selected with a human player panel in
`native/build/startup-select.png`. The earlier N/A panels were inactive slots:
the original cursor logic activates them when the hand enters the roster.

`--stage` retains both virtual controller states, connects P2, chooses another
fighter and presses Start. The original game advances to
`mnStageSel_Scene_OnEnter` after submission 1,402 (`stage-script-run.log`).
The bounded watchdog is 45 seconds for this longer script. No host input,
window, UI or audio device is used.

The selection decoder now also recognizes the twelve-bundle
`MnSelectStageDataTable` in `MnSlMap`. The new `--stage-select` descriptor probe
currently rejects bundle 10: its two polygons use POBJ_SHAPEANIM (flags 0x9000)
and shape-set descriptors with six shapes, vertex and normal index tables.
The current native scene decoder supports rigid/envelope polygons and empty
shape trees only. Real morph geometry and shape animation must be implemented
before this loader or the original stage-selection screen can pass. The
failure is explicit (`native/build/stage-probe.log`); no stage frame or match
is claimed. The shared loader and integration compile for arm64 iOS.


### Stage-selection morph geometry

The native scene decoder now owns average/additive XYZ shape sets. It validates
shape counts, vertex/normal counts, index-table spans and coordinate bounds;
CPU-consumed coordinates become native float triples, while display lists and
shape-index bytes retain GameCube byte order. Signed/fractional integer normal
and position data are converted explicitly. NBT morph coordinates remain
unsupported. The new `melee_scene_bind_shapes` converts shape animation trees
and validates weight channels against each target shape set. Failed bindings
release their tracks and descriptors. The empty-only API is retained for
callers that require that narrower schema.

Both retail stage-selection archives now pass all twelve model bundles. The
probe compares every indexed morph coordinate in both shape polygons against
independent BE reads from the source, then samples animations after disposing
of the archive. Stage icon bundle 2 is sampled at its seven legal selector
frames: the original scene uses stage ids minus 0x16; arbitrary later frames
would address outside its image table. All 25 HSD tests pass, and the morph
converter, selection adapter and stage source compile for arm64 iOS.

The original stage scene also exposed a no-selection sentinel (30) read past
the thirty-entry stage table in the preview callback. Native code guards that
lookup and only restarts preview animation for actual previewable stages.
The original console conditions are retained.

The corrected `--stage` run completes 1,600 GPU submissions and exits zero
under AddressSanitizer, capturing the original stage screen in
`native/build/startup-stage.png` (`stage-preview-run.log`). The framebuffer was
visually inspected; the stage roster, background and selection cursor render.

The new `--match` script moves the stage cursor and confirms a stage. It reaches
submission 1,593, exits stage selection and enters match preparation, then hits
the existing native `HSD_SynthSFXBankDeflag` invariant from `gm_LoadAnnouncer`
(`native/build/match-script-run.log`). Bank lifecycle/compaction state must be
resolved next. No fighter gameplay or complete playable match is claimed.

### Canceled sound-load retirement before compaction

The match-preparation audio assertion was a canceled-transfer race.
`HSD_SynthSFXGetPendingLoadCount` excludes a canceled active load, allowing the
original audio manager to proceed, while the native worker still has to finish
that transfer. Native bank compaction now waits for that canceled request to
retire before moving sample data. It still rejects an uncanceled active load
and requires interrupts to be enabled while waiting.

The loader regression cancels active and queued requests while the worker is
excluded, verifies the logical pending count is zero while one physical request
remains, then compacts immediately. It verifies retirement and suppressed
callbacks. The original load/PCM/cancellation suite and overlapping ARAM
compaction suite pass with AddressSanitizer/UBSan. The synth source compiles
for arm64 iOS.

The full `--match` run passes the canceled-load compaction boundary and reaches
`gm_Scene_Vs_OnEnter` after submission 1,602 (`match-cancel-fixed.log`). It then
encountered the console archive parser for `LbRf.dat` in refraction setup.

### Native refraction parameters

`melee_refraction_decode` converts the `lbRefData` parameter pairs into
caller-owned native floats, validates capacity/bounds and rejects nonfinite or
negative values before modifying outputs. A relocated offset zero is valid.
Original refraction setup now loads this data from raw disc/cache bytes and
uses the actual image-descriptor global instead of a synthetic contiguous
view of separate globals. Synthetic and retail tests pass under
AddressSanitizer/UBSan; the decoder and original integration compile for arm64
iOS. Runtime distortion effects remain unverified.

The next full `--match` run passes refraction initialization and reaches
`efAsync_LoadSync(0)` in the original VS scene. It stops in the console parser
for the common effects archive `EfCoData.dat` through `efAsync_OnLoad`
(`native/build/match-refraction-run.log`). Effects data conversion is the next
required integration step; no gameplay frame has yet been produced.

### Native particle-bank decoder

`melee_particle_bank_decode` converts embedded particle command and texture
banks using their bank-relative offsets. It owns native command/texture pointer
tables, endian-converted scalar command headers and a copy of bytecode/texture
payloads. Version 0 and 0x40–0x43 headers are recognized; command-base indices
are represented as leading empty slots. The original particle kind-bit fixup
is applied explicitly. Texture dimensions/formats, image/palette spans,
command-header spans and finite scalar values are validated.

The common effects archive decodes 592 command records and 36 texture groups.
The test compares every command's scalar fields against independent archive
reads, checks texture payload ownership after disposing of the source, and
rejects truncated command tables, a malformed last texture-group pointer and
NaN command data. Synthetic and retail cases pass AddressSanitizer/UBSan; the
decoder compiles for arm64 iOS. This does not yet register native banks with
HSD or convert the effects model table. `efAsync_OnLoad` still reaches the
console parser, and particle bytecode execution has not been verified.

### Native effect-bank registration and lazy model ownership

The native synchronous effects loader now reads raw cached/disc archives,
converts the effect table and registers native command/texture tables with HSD.
An effects owner keeps copied archive bytes, particle data and lazily decoded
model/animation descriptors alive. It is reset by `efLib_Init`; native
`efLib_Create` resolves models through that owner. The console paths remain
unchanged. Joint animation channels 40–42 are accepted for the original
particle, sound and particle-target callbacks. Nonpaletted particle textures
ignore the unused palette-format field (the menu bank stores 0x10000 there).

`hsd_scene_probe --effects FILE SYMBOL` disposes of the source bytes before
loading every model, checks stable descriptor identity, and checks HSD pool
cleanup. The common archive (`effCommonDataTable`) decodes 46/47 models; model
36 includes a spline joint and remains explicitly unsupported, so this probe
returns failure. The menu archive (`effMenuDataTable`) passes with one empty
model and its particle bank. Synthetic and retail particle-bank regressions
pass AddressSanitizer/UBSan. Actual particle bytecode execution, spline paths,
and the original asynchronous `efAsync_OnLoad` archive path remain unported or
unverified; this is not a claim of complete visual effects support.

### Native player parameters and the next VS boundary

`melee_player_params_decode` converts `PdPm.dat`'s `plLoadCommonData` scalar
record into native storage. Every known field has a compile-time offset check;
floats must be finite, signed integer bits are preserved, and the untyped
four-byte gap is copied unchanged. The output is only replaced after successful
validation. Original `Player_80036DD8` now loads this data through the raw cache
or disc fallback into persistent native storage.

Synthetic and retail parameter tests pass AddressSanitizer/UBSan, checking all
scalar bits, the raw gap, relocated offset zero, truncated spans, NaN rejection
and output ownership. All eight changed decoder/integration sources compile for
arm64 iOS. The ARM64 full-game executable builds. The 28 available CTest checks
pass; nine dependency tests are registered as `NOT_BUILT` and were not run
(`native/build/effects-hsd-regressions.log`).

The windowless ASan `--match` run now passes refraction, common/menu effects
registration and player parameters. It reaches stage loading and stops at the
console archive parser through `grDatFiles_801C5FC0` / `Stage_802251E8`
(`native/build/match-player-params-run.log`). Stage gameplay data is the next
conversion boundary. No playable match, interactive full-game app, or verified
iPhone gameplay is claimed.

### Native stage collision tables

`melee_stage_collision_decode` converts the `coll_data` public symbol into
owned, mutable `MapCollData`, vertex, line and joint arrays. Original physics
mutates collision lines, so these arrays do not borrow archive storage. The
converter preserves signed adjacency sentinels and collision flags, validates
vertex/line references and each surface/vertex interval, rejects nonfinite
coordinates and inverted joint bounds, and enforces the original runtime
capacities (2,048 vertices, 1,536 lines, 256 joints). External references elsewhere
in an archive are allowed; unresolved references in the collision pointer slots
are rejected by the archive pointer resolver.

`make -C native test-stage-collision` passes under AddressSanitizer/UBSan.
The same test passes on all 66 stage DAT filenames found in the original ground
sources and extracted from the supplied disc (`stage-collision-retail.log`).
It compares every vertex, line and joint scalar against independent big-endian
reads and tests oversized counts, invalid vertices/adjacency, invalid surface
ranges, truncated arrays, NaN coordinates, and lifetime after source disposal.
Battlefield contains 26 vertices, 23 lines and one collision joint. The full
ARM64 executable builds with the new module; the module also compiles for arm64
iOS. It is not yet connected to `grDatFiles`: model/parameter ownership and the
remaining stage public symbols must be converted before replacing that archive
loader. The previously verified VS stop remains stage archive initialization.

### Native ground and per-stage parameters

`melee_stage_params_decode` owns a converted `grGroundParam` record and its
`StageParam` rows. It converts known scalar fields and both halfword arrays,
retains the raw padding and RGBA byte colors, normalizes the validated camera
boolean, and replaces the disk row pointer with native owned storage. The row
span/count and finite floating-point values are checked before success.
Unrelated archive external references are permitted; unresolved row pointers
are rejected.

The synthetic AddressSanitizer/UBSan test compares all scalar fields, rows,
arrays and color bytes with independent big-endian reads and checks NaN,
oversized/truncated rows, invalid camera booleans and ownership after source
release. The retail test passes all 66 extracted ground archives
(`native/build/stage-params-retail.log`); Battlefield has 18 parameter rows.
The full ARM64 Mac executable builds with this decoder and the source compiles
for arm64 iOS. Like collision conversion, this is not yet wired into the
original stage archive loader: native model ownership and other stage public
symbols remain required before that loader can be replaced. The game remains
unplayable at the stage-loading boundary.

### Native stage model records

`melee_stage_models_decode` now owns a copy of `map_head`'s model table and
lazily builds native `UnkStageDat_x8_t` records. Each record owns its model and
animation variants, environment, signed joint mappings and visibility indices;
byte animation flags borrow the owner's archive copy. Each animation variant
currently has its own scene owner, preserving descriptor lifetime at the cost
of duplicated geometry/archive storage. No model is returned on failed
conversion, and partially built owners are released. Custom camera animation
and unsupported scene/environment schemas still fail explicitly.

The environment decoder now accepts absent camera, light-list and fog offsets
(`UINT32_MAX`), preserving NULL descriptors. Three Battlefield records have no
fog; treating that as a malformed fog object previously rejected them.

The new `hsd_scene_probe --stage-models FILE` disposes of the input archive,
loads every model, checks stable descriptor identity, loads original HSD joints
from the retained descriptors and samples the first animation at frame zero,
then checks HSD pool cleanup. All seven Battlefield records pass, and this probe
is registered as `hsd_battlefield_models` when its extracted DAT is available.
All 25 preexisting HSD regressions pass. The full ARM64 executable builds and
both changed decoder sources compile for arm64 iOS. Final Destination decodes
nine of ten records; record 3 has unsupported camera animation. Later animation
frames and gameplay rendering are not verified by these probes.

The stage archive loader remains unconverted: the model table is now available
as a native component alongside collision and parameters, but map-header
metadata, other public symbols and their original lifecycle still need
integration. The previously verified full-game stop remains stage loading;
there is no playable match yet.

### Scene-relative stage confirmation and mapping stride

A fresh fixed-submission `--match` run reached the 1,900-submission cap while
still on the stage-select screen, with a locked slot selected. Its PNG is stage
selection, not gameplay. Stage input now uses an atomic frame counter updated
by the original stage-select OnFrame callback: up at scene frame 60, neutral at
70, confirm at 120, release at 126 (or on scene exit). OnEnter/OnExit reset the
counter. This avoids sending the stage movement before the scene can consume it.
Other scripted menu/character inputs still use submission timing.

The revised windowless ASan run confirms Onett (`/GrOt`) and reaches the known
console stage archive parser (`native/build/match-stage-ready.log`). Onett's
localized archive is `GrOt.usd`, rather than a filename ending in `.dat` in the
original ground sources. Its six model records pass the stage-model probe;
its collision and parameter decoders also pass. This identifies the actual
next integration target without forcing a different stage selection.

`GroundJointMapEntry` now gives all three readers of the map header's
joint-to-stage-point table the same structure. Previously the clearing path
used a pointer plus eight padding bytes (16 bytes on ARM64), while the main
mapping path used two pointers and a count (24 bytes); iteration would diverge
for native data. The random-point reader now uses the same pairs/count fields.
The full ARM64 executable builds, and both changed game sources compile for
arm64 iOS. The mapping fix awaits runtime use through the native stage loader.
No gameplay frame has been produced.

### Owned stage-point mappings

`melee_stage_models_point_maps` converts the map header's joint-to-stage-point
records into owned `GroundJointMapEntry` arrays. Root references resolve to the
exact native model descriptors returned by the same owner. If several model
records share a disk root, a mapping is generated for each native descriptor,
so the original pointer-identity lookup still works. Pairs are converted to
native signed halfwords; source joint indices are checked against the decoded
joint hierarchy, and destination indices against the 261-slot stage table.
Unresolved model roots, bad spans/counts and invalid indices fail without
changing the caller's output pointers/counts. Successful results are cached
until the model owner is destroyed.

The stage-model probe compares every converted pair with independent archive
reads, checks exact root identity, injects invalid source and destination
indices and checks rejection/output preservation. It then destroys the input
bytes, loads/samples the models, verifies cached mapping identity, and checks
HSD pool cleanup. Battlefield and Onett both pass; Onett is now a registered
CTest when `GrOt.usd` is present. The ARM64 full-game build and arm64 iOS decoder
compilation pass. These mappings are not yet installed in `grDatFiles`; the
remaining map-header overrides and stage public data still need integration.

### Stage material render-mode overrides

The stage model owner now applies the map header's material override list to
its private archive copy before converting models. This reproduces
`grDatFiles_801C6228`'s `0x04000000` render-mode bit, including every animation
variant's independently owned geometry. The list/count and each target span
are checked. This avoids writing at the GameCube +4 field offset in a native
pointer-bearing material descriptor. Source archive bytes remain unchanged.
A future native header must treat these overrides as already applied.

The probe walks original and native joint/drawable hierarchies together and
compares each material render mode with its source value plus the archive's
explicit override list. Onett and Battlefield pass these checks, subsequent
model loading/first-animation sampling, mapping tests and HSD pool cleanup.
The ARM64 full-game build and iOS decoder compilation pass. Light overrides,
other header metadata and public symbols still need conversion/integration;
this does not advance the full-game runtime past stage loading yet.

### Native light override identity and flags

`melee_stage_models_light_overrides` now converts the map header's light
settings into owned `LightOverrideEntry` records, using the shared definition
also read by original ground code. The retail header length is in 32-bit words:
Onett's 28 words cover 14 eight-byte records, ending exactly at the material
override table; Battlefield's 34 words cover 17 records. Counts must be even,
spans valid, references resolved and only the three known flag bits present.
The native output count is a record count.

Light lookup by archive offset now resolves the original descriptor identity
within each converted model environment. If multiple models have separate
native copies of a referenced light, each receives an override record. Entries
not present in model environments own a separately converted light descriptor
chain. These are retained for the lifetime of the stage model owner.

Onett produces 14 records; the probe compares all 12 model-light references
with their original flag words and verifies native pointer identity, cached
results after source destruction, malformed-count rejection/output preservation
and cleanup. Onett and Battlefield CTests pass. The ARM64 full-game build and
all four affected C sources' arm64 iOS compilations pass. The records are not
yet installed in the original stage archive bridge; other stage public data
and lifecycle integration remain necessary before playable matches.

### Native map_head assembly

`melee_stage_models_header` assembles the completed model, point-mapping and
light-override components into an owned `UnkStageDat`. The model array is
contiguous at the native struct stride, and copied records retain the same
borrowed descriptor identities as the owner’s individual model records. Point
and light counts use their converted record counts. Material override processing
is represented as an empty list because it was already applied before model
conversion. Nonempty spline and shadow-animation tables remain explicitly
unsupported, rather than being exposed as disk pointers.

Onett and Battlefield pass header assembly and cached identity checks after
source destruction. The probe compares every contiguous model record with its
individual owned record and verifies mapping/light references, empty optional
tables and HSD cleanup. The full ARM64 executable builds and the updated owner
compiles for arm64 iOS. Other stage archive symbols (hazard parameters, effect
models, particle/light data and item data) and the archive-loader lifecycle
still need integration; no full-game stage frame or playable match is claimed.

### Onett hazard parameters

`melee_onett_params_decode` converts Onett's 26-field floating-point
`yakumono_param` record into caller-owned native storage. The schema is shared
with original `gronett.c` through `gronett.h`; it includes awning spring and
movement values, building hit thresholds, and car timers/speeds. This decoder
is specific to Onett: other stages use different hazard parameter layouts.
All floats must be finite and the full record must fit before any caller
output is modified.

Synthetic and `GrOt.usd` tests pass AddressSanitizer/UBSan, comparing every
field's bits with independent big-endian reads and testing final-field NaN,
truncation rejection/output preservation and source-disposal lifetime. The
full ARM64 game builds, and the decoder plus original Onett source compile for
arm64 iOS. This component awaits stage archive bridge integration. Onett also
has item-script references in `ALDYakuAll`, particle lights in `map_plit` and
an animated `quake_model_set`; those remaining symbols are not yet converted.

### Owned dynamic model and quake animations

`melee_dynamic_model_decode` converts a public `DynamicModelDesc` into owned
joint, material and shape animation variant tables. Each variant retains its
scene descriptors after validation instances are removed. Tables remain
NULL when absent, are terminated when present, and counts are exposed for
bounded consumers. Unsupported scenes/animations and malformed table/header
spans fail with owner cleanup. Geometry/archive duplication across variants
remains a memory optimization opportunity.

Onett's `quake_model_set` has four joint-animation variants. The new
`hsd_onett_quake` test releases source bytes before loading original HSD joints
and sampling each variant through frame 60. It verifies finite transforms,
actual transform changes in all four variants, table termination, truncated
header rejection and HSD pool cleanup. The test, full ARM64 game build and
arm64 iOS decoder compilation pass. This supplies the quake descriptor for the
future native stage archive bridge; the original gameplay quake path itself
has not yet run in a match.

### Native Onett item-hitbox scripts

`melee_item_scripts_decode` converts `ALDYakuAll` into an owned native command
pointer table, preserving reserved NULL slot zero and the final terminator.
Supported streams contain item create-hitbox commands (opcode 11) and end
commands (opcode 0). Other opcodes fail explicitly. The decoder assigns native
bitfields individually, including signed halfword offsets and shield damage;
it preserves the final payload's byte order because the original item handler
reads those flags through `u8*`. Commands occupy native `CmdUnion` strides,
matching the original handler's pointer increments. Invalid hitbox IDs,
truncated streams/tables and excessive command/table counts are rejected.

Synthetic and Onett retail tests pass AddressSanitizer/UBSan. They independently
reconstruct all command words from decoded fields, compare raw flag bytes,
check termination, unsupported opcode/ID and truncation rejection, and access
the owned scripts after source destruction. Onett has five such scripts. The
ARM64 full-game build and arm64 iOS decoder compilation pass. Actual item-hitbox
execution in a match remains unverified; the archive bridge must install these
native pointers before the original item system can consume them.

### Onett archive integration and the next VS boundary

`melee_onett_stage_decode` now assembles all ten stage-facing public symbols:
map header, collision, ground/hazard parameters, the empty item table, native
item scripts, quake model, particle lights and particle-bank markers. Particle
light descriptors are resolved to the same native instances used by the header
light overrides. Banks are registered through `psInitDataBankNative` rather
than interpreting the opaque markers as GameCube data.

Original `grDatFiles_801C6038` now handles Onett using raw cached/disc bytes and
this native archive adapter. Original ground initialization registers its
second particle-bank slot through the adapter as well. Native stage archive
owners have a separate registry and are destroyed at stage registry reset,
without inspecting potentially stale console archive pointers. Repeated
match transitions and complete game shutdown remain unverified.

`hsd_onett_archive` verifies every required public symbol after disposing of
source bytes, checks particle-light override identity and checks HSD pool
cleanup. All three Onett CTests pass, along with the full ARM64 build and all
four changed sources' arm64 iOS compilations.

The windowless ASan `--match` run now passes Onett archive loading and reaches
`it_8027870C` in VS initialization. It stops in the console parser while loading
`ItCo.usd` / `itPublicData` (`native/build/match-onett-native.log`). Common-item
archive conversion is the next required boundary. This is verified progress
past the stage archive failure, not a playable match or proof of hazard/item
execution; no gameplay framebuffer has yet been produced.

### Common-item scalar parameters

`melee_item_common_decode` converts `itPublicData`'s shared `ItemCommonData`
record into caller-owned native storage. Integer bits (including historically
misnamed `_float` fields) are preserved, declared float fields/arrays are checked
for finiteness, and the byte field plus raw padding retain source byte order.
The full span is checked and output is only replaced after successful validation.

Synthetic and both retail language archives (`ItCo.usd`, `ItCo.dat`) pass
AddressSanitizer/UBSan tests comparing every scalar/raw field, final-field NaN
and truncation rejection, unchanged output on failure and ownership after
source release. The full ARM64 game builds and the decoder compiles for arm64
iOS. This is one component of the common-item archive, not a replacement for
its article/model/state/script tables. The verified runtime stop remains
`it_8027870C` loading the unconverted `itPublicData` archive.

### Native item article attributes

`melee_item_attributes_decode` converts the shared `ItemAttr` record at an
archive offset. The first two packed flag bytes are assigned field by field
for native bitfield order; the following byte/padding and all integer scalar
bits are retained. Declared float fields, including collision/grab bounds and
movement/scale parameters, are converted and checked for finiteness. The
caller’s output is replaced only after full validation.

Tests exhaust all 256 values for each flag byte, compare every scalar and raw
header byte, reject NaN/truncated records without changing output, and validate
all 43 common-item article attributes in both `ItCo.usd` and `ItCo.dat` under
AddressSanitizer/UBSan. The full ARM64 build and iOS decoder compilation pass.
The surrounding article’s model/state/script, hurtbone and dynamics references
remain to be converted before the common-item archive can be integrated.

### Owned item hurtbone lists

`melee_item_hurtbones_decode` converts each article's hurtbone list and owns its
native descriptors. It enforces the original collision routine's two-hurtbox
capacity, validates nonzero bone IDs against the dynamic bone-table count,
permits bone zero as the root sentinel, and converts finite endpoints and
nonnegative capsule radius. The descriptor layout is checked at compile time.

Synthetic tests cover both supported hurtboxes, root/indexed bones, invalid
IDs/counts, negative radius, NaN, truncated spans and source-disposal lifetime.
Every descriptor scalar is compared with an independent big-endian read.
All 14 present common-item hurtbone lists pass in each language archive under
AddressSanitizer/UBSan. The full ARM64 game builds and the decoder compiles for
arm64 iOS. Article model/state/dynamics conversion and common-item archive
integration are still pending; this does not move the verified runtime stop.

### Native item dynamics and shared record layout

`ItemDynamics` now describes both serialized sections: bone simulation and
collision dynamics. Original collision code reads the shared fields directly
instead of casting to an eight-byte padded prefix, which placed its count over
a widened native pointer. `DynamicsDesc` exposes the compact parameter-array
pointer alongside its runtime-chain pointer; native `lb_80011710` reads the
parameter array directly rather than pretending it is a runtime node.

`melee_item_dynamics_decode` owns native bone descriptors, 60-byte scalar
parameter arrays and collision descriptors. It checks bone IDs, item capacities
(24 dynamic-bone records and two collision records), bounded parameter counts,
finite scalar values, nonnegative collision size and array spans.

Synthetic and all three common-item dynamics records in both language archives
pass AddressSanitizer/UBSan. Tests compare every decoded scalar, invoke original
`lb_80011710` and compare all 15 copied parameters plus position for every
runtime node, then check invalid bones/counts, NaN/truncation and owned lifetime.
The full ARM64 executable builds and all three changed C sources compile for
arm64 iOS. Complete simulation updates and common-item archive integration are
not yet verified; the game remains stopped at common-item loading.

### Native common-item models and joint-copy constraints

`melee_item_model_decode` owns the native `ItemModelDesc` and its scene,
including bone metadata and raw flag byte. Null models with zero bones are
supported; populated dynamic-bone tables remain bounded by the original
100-entry capacity. Descriptors survive disposal of the source archive and
can be loaded again by the original HSD joint loader.

The scene decoder now converts the rotation-copy bytecode constraints used by
Fire Flower: a single local rotation argument, PUSH_ARG 0, RETURN, native joint
references and a terminated rvalue array. Other expression programs remain
unsupported. Bytecode stays in the scene-owned archive copy. The original
bytecode evaluator now copies float payloads as 32-bit bits on native builds
instead of reading pointer-sized values from four-byte floats. The bit-count
helper uses an unsigned shift, avoiding undefined behavior at bit 31.

All 43 common-item models in each language archive pass the HSD probe under
AddressSanitizer/UBSan with halt-on-error. The probe destroys the source bytes,
reloads the descriptors, checks every joint matrix for finite values, changes
source rotations and verifies all 16 Fire Flower constraints, then checks
HSD reference, animation, constraint and matrix/vector pool cleanup. A direct
bytecode negation check exercises the float-payload fix. All 31 HSD regression
tests pass; the full ARM64 executable builds and the four changed C translation
units compile for arm64 iOS.

Article state animations, scripts, special attributes and common-item archive
integration remain pending. These isolated model tests do not establish a
playable match; the verified game startup boundary remains `itPublicData`
loading in `it_8027870C`.

### Common-item state animation decoding

`melee_archive_copy_null_externals` creates an owned serialized archive copy
with external link chains resolved to null, matching `lbArchive_InitializeDAT`.
It removes the external table, adjusts the serialized size and preserves the
public table, string offsets and ordinary relocations. The default pointer
reader still rejects unresolved external references. Synthetic tests cover a
multi-slot external chain, unchanged source bytes, preserved relocation and
string data, and cyclic-chain rejection.

`melee_item_animation_decode` binds the three animation fields of one
`ItemStateDesc` against its model and returns the owning scene. The script
field is deliberately handled separately by the future article owner. The
scene decoder also preserves empty `HSD_RObjAnimJoint` lists; original HSD uses
these records to clear previous constraint animations. Nonempty constraint
animation tracks remain unsupported.

Both common-item language archives pass all 104 state records, including 41
with animation data. The probe overwrites source bytes, releases the initial
HSD objects, reloads native descriptors, attaches the three animation trees,
and samples frames 0 through 60 in steps of ten while checking finite joint
matrices and HSD reference/animation/constraint cleanup. Fixture enumeration
uses the observed table-before-Article layout only in the probe, not as a
production state-count rule. Null-model states are validated to have no
animation pointers. These checks exercise HSD animation loading and sampling;
they do not yet exercise original item state transitions or dynamic-bone
animation suppression.

All 33 HSD regressions and the archive/PAD unit tests pass with sanitizers.
The full ARM64 game executable builds; archive, item-animation and scene
sources compile for arm64 iOS. Common-item scripts, special attributes and
archive integration remain pending, and the verified full-game stop remains
`itPublicData` loading rather than a playable match.

### Common-item command graphs

`melee_item_script_decode` now owns reachable item commands (opcodes 0–25),
including relocated call/jump targets, shared subroutines and backward jumps.
It rejects out-of-range or unaligned targets, overlapping commands/payloads,
invalid hitbox indices and unsupported sound behaviors. Native-width command
slots preserve disk-word indices for stable references. The existing Onett
`ALDYakuAll` owner uses this decoder too. Owners currently allocate an arena
covering the archive's data-word span; future article integration should share
or lazily retain these arenas rather than duplicate one per state.

Original item effect, damage and sound handlers now read the converted payload
layout on ARM64. Effect halfwords remain signed/unsigned scalar pairs; damage
uses its native bitfield; sound advances by a native command slot rather than
a four-byte local struct. Sound volume/pan and the final hitbox flags remain
raw byte payloads where the original consumers read bytes.

The graph test checks all 68 nonnull common-item state scripts in each language
archive after disposing of source bytes. Over a bounded 120-frame sample it
runs original common command functions (timers, calls, returns and jumps),
checks 130 item-command events, follows nine calls and seven jumps, and invokes
the three corrected original handlers with captured effect, damage and sound
arguments. Other item commands have their payloads checked or are stepped over;
this is not a complete item simulation. The synthetic hitbox tests, all five
Onett hazard scripts and the Onett archive regression pass under sanitizers.
The full ARM64 executable builds and both changed C sources compile for iOS.

Special attributes and common-item archive integration are still pending.
The original item dispatcher's command-stack/budget enforcement also remains
to be addressed before relying on arbitrary scripts. These tests do not move
the verified full-game boundary beyond common-item archive loading.

### Food special attributes and native item consumers

Food special data is a count followed by 16-byte records containing a model
pointer, healing amount and X/Y offsets. The old `itFoodsAttributes`/`Vec4`
overlays placed each Y offset at the next record's first word, including a
trailing scalar after the last model. Those views cannot survive pointer
widening. Native code now uses an explicit header and entry array, and original
food spawn and positioning functions access those fields directly.

`melee_item_foods_decode` owns every native model descriptor and scalar entry,
checks a bounded positive count, complete record spans, model references,
nonnegative healing and finite offsets, and retains descriptors after releasing
validation objects. Both common-item language archives pass all 28 food models,
healing values and offsets, source disposal, descriptor reload, finite joint
matrices and HSD cleanup. Invalid counts, NaN and a truncated final record are
rejected. The original food spawn and positioning functions pass a separate
sanitized test over all 28 indices and both facing directions, with captured
model/state-change calls and real HSD joint translation fields.

The full ARM64 game executable builds and both changed C sources compile for
iOS. Other item-specific schemas and common-item archive integration remain
pending. These checks do not establish an in-game item spawn or a playable
match; the verified startup boundary remains common-item archive loading.

### Warp Star special attributes

`melee_item_wstar_decode` owns the nine scalar parameters and variable array of
animation/sound variants. The native `itWstarAttributes` declaration uses a
flexible array so the seven widened records can be indexed correctly. Counts
are bounded to 2–7 to keep the original no-repeat picker nonempty and within
its seven-slot candidate array; scalar values must be finite and the flight
speed divisor nonzero.

The original rider code calls `HSD_JObjAddAnim` on one root joint. It does not
traverse the child/next animation trees. `melee_scene_bind_joint_root` implements
that exact binding scope, retaining owned root tracks and omitting unused
hierarchy links; the full-tree binding API is unchanged. The seven variants
are not compatible with blindly attaching their entire trees to the item model.

Both language archives pass scalar and sound-ID comparisons, source disposal,
reloading and sampling all seven variants at frames 0–120, observed matrix
changes, finite matrices and HSD cleanup. Tests also reject invalid variant
counts and a zero speed divisor. The Warp Star and base-scene CTests pass with
sanitizers. The full ARM64 executable builds; decoder, scene and original Warp
Star source compile for arm64 iOS. Rider/fighter gameplay and the picker are
not exercised by these isolated animation checks. Other special attributes
and common-item archive integration remain pending, with no change to the
verified full-game stop at common-item loading.

### Mushroom special attributes

Super and Poison Mushroom records each contain two scalar parameters followed
by two animation pointers. The original animation accessor treated the whole
record as a pointer array and skipped two entries, which skips the scalar
prefix only on a four-byte-pointer platform. Native `KinokoAttrs` now names
the two animation pointers explicitly, and `it_80293660` reads those fields.

`melee_item_mushroom_decode` owns the scalar record and root-only animation
tracks, matching the fighter's `HSD_JObjAddAnim` consumer. Both language archives
pass both item records and four animation bindings after source disposal. The
probe creates the same ordinary single-joint setup used by fighter scale
animation, samples frames 0–120, and checks finite, changing matrices and HSD
reference/animation cleanup. NaN and truncated records are rejected. The full
ARM64 executable builds; the decoder and both original mushroom sources compile
for arm64 iOS. These tests do not establish an in-game mushroom pickup. Remaining
special attributes and common-item archive integration still block verified
match startup.

### Scalar common-item special attributes

`melee_item_special_decode` adds explicit scalar-only schemas for 18 common-item
kinds: Capsule, Crate, Barrel, Party Ball, Barrel Cannon, Bob-omb, Bat, Ray Gun,
Flipper, Super Scope, Lip's Stick, Fan, Fire Flower, Poke Ball, Ray Gun Ray,
Ray Gun Beam, Hammer Head and Fire Flower Flame. Original struct sizes are
checked at compile time. Per-schema masks distinguish finite float fields,
integer bit patterns and raw padding; Super Scope's nominal first padding word
is converted numerically because its spawn code reads that word as ammo.
Party Ball and Barrel Cannon's unused byte padding remains byte-identical.
Unsupported kinds return no schema rather than guessing their layout.

Both language archives pass independent comparisons of every decoded word and
raw padding span, truncated records, NaN rejection, integer-bit preservation
and source-disposal lifetime under AddressSanitizer/UBSan. The full ARM64 game
builds and the decoder compiles for arm64 iOS. These are attribute conversion
checks, not runtime verification of all 18 items. Remaining schemas and the
common-item archive owner are still needed before match startup can advance.

### Remaining common-item special schemas

The shared special-attribute decoder now covers 39 kinds. Alongside the food,
Warp Star and two mushroom owners, every common-item kind has a conversion
path for its special record in both supplied language archives. Six original
scalar structs moved into the shared header so gameplay and decoding use one
layout. Common Green Shell's shared type has a distinct name from the separate
adventure shell's incompatible local type.

Beam Sword's three initial words are serialized integers with no relocation;
the old `UNK_T` declarations widened them into pointers on ARM64 and shifted
its scalar/color fields. Native declarations now preserve their four-byte
size. Egg's special data now has its own two-field type instead of using the
larger runtime item-variable struct. Event Yoshi Egg converts its integer
threshold and validates the extra pointer as null before constructing the
widened native record. A nonnull extension remains unsupported.

Both language archives pass all 39 shared schemas, comparing every converted
word and raw padding/color span, with bounds, NaN and owned-lifetime checks
under sanitizers. The full ARM64 executable builds. Ten decoder/gameplay files,
including the moved schemas and affected shell, sword and egg consumers,
compile for arm64 iOS. Full item gameplay is still unverified; assembly of
common-item articles and archive integration is the next step. The verified
full-game boundary remains `itPublicData` loading.

### Assembled common-item article owners

`melee_item_article_decode` combines common attributes, special attributes,
model descriptors, hurtbones, dynamics, state animation descriptors and script
graphs into one owned native `Article`. External symbols are resolved to null
according to the original archive initializer. The caller supplies a state count
from its item schema; production decoding never infers it from neighboring
archive objects. Counts are bounded to 64 and absent state tables require zero.

Native `ItemStateArray` now points to an allocation sized for the actual state
count. This avoids the original eight-element declaration's limit: the common
Super Scope Beam record has ten states. Original native consumers keep their
existing `x0_itemStateDesc[index]` access syntax. Console layout is unchanged.

Combined tests assemble every common article in each language archive, dispose
of source bytes, reload all 104 states' models/animations, check finite matrices
and script pointers, then verify HSD reference/animation/constraint cleanup.
The fixture probe derives state counts from the observed retail layout only
for enumeration; the production API requires an explicit count. The full ARM64
executable builds and the owner plus four original state-table consumers compile
for iOS. Script arenas and scene copies are still owned per state/article;
sharing or lazy ownership will be needed when retaining the whole archive.

This is article assembly, not full archive integration. The `itPublicData`
header also references fighter/adventure and Pokemon article tables and shared
auxiliary data. Those need a native archive owner before match startup can use
these articles. The verified runtime boundary remains common-item loading.

### Shared item color-animation bank

`melee_item_colors_decode` owns the explicit-count metadata table and reachable
color-command graphs, including native branch pointers and raw RGBA payloads.
It accepts common control flow used by color scripts and color operations
10–20, validates command boundaries/targets, and rejects unsupported operations,
zero loop counts and zero-duration blends. External effect/sound callbacks are
not yet decoded by this color-bank API.

Native `ColorOverlay_x8_t` commands now have the same pointer-width stride as
`CmdUnion`. `ColorOverlay` also reserves the same five pointer-width command
stack slots as `CommandInfo`, replacing its old mixed integer/pointer overlay.
Compile-time checks tie their command pointer, stack depth and stack offsets
together. The console declarations remain unchanged.

The seven-entry common-item bank contains one null entry and six scripts. Both
language archives pass metadata/ownership checks and 600-frame bounded runs
through original `lb_80014258`: four scripts finish and two remain looping,
with 632 observed color changes and a known first-frame RGBA value checked.
Tests dispose of all input bytes before execution and reject invalid counts,
truncated tables and a zero-count loop. These tests exercise the original
color interpreter; they do not prove the full game has reached an item effect.

All 41 HSD regressions pass with sanitizers after the runtime layout change.
The full ARM64 executable builds; the decoder, color/common interpreters and
item source compile for arm64 iOS. Auxiliary enemy parameters and the remaining
article tables still need integration into the full `itPublicData` archive
owner. The verified full-game boundary remains common-item archive loading.

### Item archive integration and fighter startup boundary

`melee_items_data_decode` now owns the native `itPublicData` header, all 43
common articles, shared parameters, enemy parameters and seven-entry color
bank. The production decoder uses explicit article state counts. Writable
fighter/adventure and Pokemon registration tables are allocated at native
pointer width. Their built-in articles are still deferred, except the Yaku
stage-hazard article; missing articles fail with a diagnostic rather than
being interpreted as native pointers. Deferred serialized offsets are retained,
but source bytes are not, so future conversion must reload or retain that data.

The Yaku article owns its root hurtbone and twenty initially empty state
records. Original Onett setup attaches its already-converted hazard scripts.
The runtime item loader uses this archive owner, and the original item lookup
checks that an article has been converted or registered. Onett's warning model
lookup now indexes its native model array instead of advancing by the original
52-byte record size.

Both `ItCo.dat` and `ItCo.usd` pass archive lifetime, auxiliary scalar, common
article and dynamic registration checks. All 43 HSD CTests pass with sanitizers
(`items-data-regressions.log`). The complete ARM64 executable builds; the item
decoder and changed item/stage consumers compile for arm64 iOS. The archive
currently retains large per-script arenas; iOS peak memory is not yet measured.

The windowless `--match` run now passes common-item loading and Onett setup,
then reaches original fighter initialization. Its reset routine assumed
adjacent globals and fixed addresses from the console executable; native code
now explicitly references the corresponding arrays. The ASan global-buffer
overflow is resolved. The next verified stop is `Fighter_LoadCommonData`
loading `PlCo.dat` through the unconverted console archive parser
(`match-fighter-reset.log`). No playable match or new gameplay image has been
produced.

### Shared fighter scalar parameters

`melee_fighter_common_decode` converts the 0x818-byte parameter block at entry
zero of `ftLoadCommonData`. Twenty-one undocumented fields are numeric words,
not relocated addresses; native declarations now preserve that scalar layout.
The corresponding x500 countdown storage is also numeric. Console declarations
remain unchanged. Integer words and the embedded hit-capsule descriptor retain
their bits, RGBA/byte fields retain byte order, and floating-point parameters
must be finite. The decoder rejects relocations within this scalar block and
truncated input without changing the caller's output.

`make -C native test-fighter-common` checks synthetic data, and
`native/build/test-fighter-common native/build/PlCo.dat` checks the disc's
parameters under AddressSanitizer/UndefinedBehaviorSanitizer, including output
ownership after source disposal. Both pass. The full native executable builds,
and the decoder/countdown consumer compile for arm64 iOS. This component is
also covered by passing original fighter action and visibility regression
tests after the layout change. It is not yet connected to
`Fighter_LoadCommonData`: the other 22 archive entries
still need native ownership/conversion before the full archive can be loaded.

### Shared fighter bone and shake tables

`melee_fighter_parts_decode` owns entries 4 and 5 of `ftLoadCommonData`: all
33 joint/part maps and the optional accessory records for Kirby, Link and
Young Link. The canonical map has 54 entries, including unnamed part 0x35
used by original setup. Joint counts are bounded by `MAX_FT_PARTS`; map values,
accessory joint indexes, attachment modes, counts and byte ranges are checked.
These byte maps retain their original values while pointer-bearing table
records use native layout.

`test-fighter-parts` validates 2,268 joints and 15 accessory records against
`PlCo.dat`, destroys the source bytes, then executes original bone lookup,
accessory-mask and remapping code. All 74,844 cross-fighter remaps pass under
AddressSanitizer/UndefinedBehaviorSanitizer. Invalid indexes/counts and truncated
data are rejected. The owner compiles for arm64 iOS.

`melee_fighter_shakes_decode` owns entries 9–11: three damage-shift tables,
grab mash and smash charge. It validates byte-sized nonzero counts and finite
Vec2 samples. Native `Fighter_804D6530` and its damage consumers now use explicit
pointer/count records rather than treating counts as pointer array entries.
Console declarations and consumers are unchanged.

`test-fighter-shakes` compares all five tables to the disc data, disposes of the
source bytes, and exercises original damage-shift calculation for every sample
in both facing directions, including the slope transform and inactive case.
Zero/oversized counts, NaNs and truncation are rejected. The sanitizer test,
full native executable build and arm64 iOS compilation pass.

Build and run these retail-data checks with:

```sh
make -C native build/test-fighter-parts build/test-fighter-shakes
UBSAN_OPTIONS=halt_on_error=1 native/build/test-fighter-parts native/build/PlCo.dat
UBSAN_OPTIONS=halt_on_error=1 native/build/test-fighter-shakes native/build/PlCo.dat
```

These owners are components for the pending complete `PlCo.dat` loader; they
are not yet installed into the running game. The verified runtime stop remains
`Fighter_LoadCommonData`, and no match gameplay is established.

### Shared fighter throw, swing and movement modifiers

`melee_fighter_modifiers_decode` converts PlCo entries 1–3 and 12–15 into an
owned value: 26 three-float throw records, six five-float swing rows, nine
staling weights and the scale, bunny, metal and gravity/weight modifiers.
The throw count is checked against the original common motion-state range;
modifier layouts have compile-time size checks. All 182 values must be finite,
and failed decoding leaves the output unchanged.

The retail-data sanitizer test compares every value, rejects NaNs and truncated
records in each of the seven tables, and checks ownership after source
disposal. It passes, and the decoder compiles for arm64 iOS:

```sh
make -C native build/test-fighter-modifiers
UBSAN_OPTIONS=halt_on_error=1 native/build/test-fighter-modifiers native/build/PlCo.dat
```

This is another component of the pending complete PlCo loader; the runtime
boundary has not advanced. Investigation of entries 6 and 7 found color banks
with 123 and six entries. The existing item-color decoder does not yet accept
all their scripts: original fighter color callbacks additionally dispatch
opcodes 21–23 to effect-spawn, sound and rumble/action handlers. Their payloads
need native conversion before these banks can be installed.

### Fighter color-command payloads

`melee_fighter_colors_decode` now shares the color-bank owner while enabling
the three fighter callbacks: five-word effect spawn, three-word sound and
one-word vibration commands. It reconstructs native bitfields, signed effect
offsets, effect IDs/ranges, sound IDs/volume/panning and vibration flags/frame
counts. Unsupported sound behavior values fail decoding. The item entry point
continues to reject these fighter-only callbacks.

Execution testing exposed a light-command opcode whose six one-bit fields
were reversed by the native bitfield layout; the decoder now explicitly sets
the complete opcode after those fields. It also exposed blends reaching 256:
native color output now converts through a signed integer before storing the
low byte, avoiding undefined direct float-to-byte conversion. Tests include
256, -1 and fractional byte-edge values. Console conversion is unchanged.

Both PlCo banks (123 and six entries, two null) pass 600-frame bounded runs
through original `lb_80014258` after all archive bytes are destroyed. Across
127 scripts, 77 finish in that window. The test validates native callback
payloads against independently retained numeric source words and observes
4,532 effect, 92 sound and 88 vibration callbacks. These callbacks are checked
and advanced by the test; this does not establish rendered effects, audible
sound or physical controller vibration in the complete game.

```sh
make -C native build/test-fighter-colors
UBSAN_OPTIONS=halt_on_error=1 native/build/test-fighter-colors native/build/PlCo.dat
```

The sanitizer test, both original item-color language tests, all 43 HSD
regressions (`fighter-colors-regressions.log`), full native executable build
and arm64 iOS decoder/interpreter compilation pass. Full
PlCo archive ownership/integration is still pending; gameplay has not started.

### Shared fighter models, colors and crowd data

`melee_fighter_models_decode` owns the animated respawn platform (entry 8),
trophy platform (entry 16) and shared material model (entry 20). Three scene
owners retain native descriptors after releasing validation objects. The
`--fighter-models` probe destroys archive bytes, reloads all three models
(11 joints), and samples frames 0–120. It checks finite matrices, actual
respawn animation changes and HSD pool cleanup. The new `hsd_fighter_models`
CTest passes under sanitizers, as do the full native build and iOS compilation.

`melee_fighter_aux_decode` converts entries 17–19 as three banks of five raw
RGBA records, preserving byte order, and entry 21 as the 17-word crowd config
with explicit integer/float interpretation. The retail test checks all 15
colors and 17 parameters, invalid float/truncated inputs, transactional output
and ownership after source disposal. The sanitizer test and iOS compile pass.

```sh
native/build/aurora-integration/hsd_scene_probe --fighter-models native/build/PlCo.dat
make -C native build/test-fighter-aux
UBSAN_OPTIONS=halt_on_error=1 native/build/test-fighter-aux native/build/PlCo.dat
```

The remaining unconverted PlCo entry is the CPU behavior header and its
referenced tables/scripts (entry 22). The converted components still need to
be assembled and installed by the full-game archive loader. The runtime has
not yet passed `Fighter_LoadCommonData` or entered verified gameplay.

### CPU tables and integrated shared fighter archive

`melee_fighter_cpu_decode` owns PlCo entry 22: 62 script slots (slot zero is
null), seven attack-table sets for the 32 CPU fighter kinds, 32 distance
thresholds and six weapon reaches. It preserves valid CPU command bytes,
checks operand lengths/termination against the 256-byte runtime buffer, and
converts bounded, zero-terminated 36-byte attack records with finite scalar
fields. The attack record declaration now lives in its shared header. A CPU
distance reader now uses the named native header field instead of byte offset
0x20 into the widened header.

The retail sanitizer test checks 61 scripts, 1,136 attack records and all
distance/reach values, destroys archive bytes, then runs the original script
copy routine and 224 original weighted attack selections with deterministic
random input. These pass; complete CPU match behavior is not yet verified.

`melee_fighter_data_decode` assembles all 23 `ftLoadCommonData` entries with
owned scalar, bone, shake, color-command, model, auxiliary and CPU data.
`Fighter_LoadCommonData` now uses this owner in the native build. The assembled
archive probe checks all entries, source disposal, original model reloads and
HSD pool cleanup. It passes, along with all 45 HSD regressions
(`fighter-data-regressions.log`), the full native build and arm64 iOS
compilation of the owner, CPU decoder and changed runtime consumers.

The windowless ASan `--match` run now passes shared fighter initialization and
reaches `Fighter_Create`. Its next stop is `ftData_8008572C` loading Fox's
`PlFx.dat` (size 0x3f70a) through the still-unconverted console archive parser
(`match-fighter-data.log`). This is a verified startup advance, not playable
gameplay. Per-character archive integration is the next runtime boundary.

```sh
make -C native build/test-fighter-cpu
UBSAN_OPTIONS=halt_on_error=1 native/build/test-fighter-cpu native/build/PlCo.dat
native/build/aurora-integration/hsd_scene_probe --fighter-data native/build/PlCo.dat
```

### Per-character common and Fox/Falco attributes

`melee_character_attributes_decode` converts entry zero of a character's
`ftData` header into `ftCo_DatAttrs`. It checks the 0x184-byte scalar layout,
preserves the five integer fields, validates floats and retains the raw throw
mask/padding bytes. `melee_fox_attributes_decode` converts the shared Fox/Falco
special-move schema at entry one, including six integer fields, signed/float
values and the raw reflector behavior byte. Both produce independent values
and preserve the previous output on failure.

The retail sanitizer test checks all 96 common words for Fox, Falco, Peach,
Kirby and Mr. Game & Watch, and all 52 special-move words for Fox/Falco. It
checks raw bytes, rejects NaN/truncated input and verifies output independence
after overwriting source data. The tests and arm64 iOS decoder compilation
pass.

```sh
make -C native build/test-character-attributes
UBSAN_OPTIONS=halt_on_error=1 native/build/test-character-attributes native/build/PlFx.dat native/build/PlFc.dat native/build/PlPe.dat native/build/PlKb.dat native/build/PlGw.dat
```

These are components of the pending per-character archive owner. The full
runtime still stops at Fox's archive; these tests do not establish movement
or special moves in a playable match.

### Per-character hurtboxes and collision parameters

`melee_character_collision_decode` owns ftData entries 0x30–0x40: the native
hurtbox header and up to 15 scalar hurtboxes, shield descriptor, two body
bound descriptors, camera vectors and item-pickup regions. It validates joint
indexes against the supplied skeleton count, hurt-height values, finite
coordinates and nonnegative radii/scales. Binding assigns only these five
fields of a caller's ftData; their lifetime belongs to the owner.

The retail sanitizer test checks every serialized word for Fox, Falco, Peach,
Kirby and Mr. Game & Watch (60 hurtboxes total), rejects too-small skeleton
counts, oversized hurtbox counts and NaN radii, and checks owned hurtboxes
after source disposal. These tests and arm64 iOS decoder compilation pass.

```sh
make -C native build/test-character-collision
UBSAN_OPTIONS=halt_on_error=1 native/build/test-character-collision native/build/PlFx.dat native/build/PlFc.dat native/build/PlPe.dat native/build/PlKb.dat native/build/PlGw.dat
```

This component is not yet integrated into the per-character archive loader.
The runtime boundary remains Fox archive loading; collision behavior during
a match has not yet been verified.

### Per-character sound metadata and auxiliary tables

`melee_character_sounds_decode` owns ftData entry 0x4C with 11 scalar sound
fields and three nullable counted sound-ID lists. The native `FtSFX.x1C`
declaration is now a pointer: original damage code uses it as a list address,
and keeping the console's integer declaration would truncate it on ARM64.
The console declaration is unchanged. The decoder checks list counts and
ranges and retains independent native IDs.

`melee_character_aux_decode` owns entries 0x44 and 0x50–0x58: six signed
halfwords and four floats for ledge parameters, a Vec2, five effect-bone IDs,
and limb-position bytes/padding with three floats. Native `ftData.x54` is now
an integer-array pointer, matching the effect reader's use of the serialized
field. Float fields must be finite; byte and halfword fields retain their
intended order.

Sanitizer tests for Fox, Falco, Peach, Kirby and Mr. Game & Watch compare all
fields and list IDs against disc bytes, reject invalid counts/truncation/NaNs
as applicable, and access native lists after freeing archive data. The full
native executable builds; both decoders and the affected damage/effect
consumers compile for arm64 iOS. These are metadata/ownership checks, not
proof of audible sound or limb behavior in a match.

```sh
make -C native build/test-character-sounds build/test-character-aux
UBSAN_OPTIONS=halt_on_error=1 native/build/test-character-sounds native/build/PlFx.dat native/build/PlFc.dat native/build/PlPe.dat native/build/PlKb.dat native/build/PlGw.dat
UBSAN_OPTIONS=halt_on_error=1 native/build/test-character-aux native/build/PlFx.dat native/build/PlFc.dat native/build/PlPe.dat native/build/PlKb.dat native/build/PlGw.dat
```

The per-character archive owner still needs the remaining animation, model,
dynamics and item tables. The runtime remains stopped at Fox archive loading.

### Per-character idle selection tables

`melee_character_wait_decode` owns nullable ftData entries 0x24 and 0x28 as
bounded, terminated animation-ID/probability tables. Animation IDs must fit
the caller's animation count, each weight is positive and totals must reach
100. Native `WaitStruct` now contains the two integers directly; the selector
reads the integer ID instead of interpreting the ID/weight pair as a pointer.
The console declaration and selector remain unchanged.

The sanitizer test verifies all 100 probability draws through original
`ftCo_8008A7A8` for Fox, Falco, Peach, Kirby and Mr. Game & Watch, including
Kirby's second crouching-idle table: 600 selections in total. Archive bytes
are freed before selection. Animation loading is stubbed to record the chosen
ID; this test does not play the selected animations. Invalid IDs and zero
weights are rejected. The full native executable and iOS decoder/consumer
compilation pass.

```sh
make -C native build/test-character-wait
UBSAN_OPTIONS=halt_on_error=1 native/build/test-character-wait native/build/PlFx.dat native/build/PlFc.dat native/build/PlPe.dat native/build/PlKb.dat native/build/PlGw.dat
```

The per-character animation records, command scripts and remaining archive
components still need integration. Runtime startup remains stopped at Fox's
archive; no playable match is established.

### Per-character motion index and lazy animation ownership

`melee_character_motions_decode` owns the main motion index (ftData entry
0x0C), copied animation names and the combined AJ file. It validates each
record's offset, size and named embedded archive, preserving flags and
serialized action-script offsets. `melee_character_motions_tree` decodes a
native FigaTree on first use through the existing animation converter and
caches it until the owner is freed. Missing animations remain null.

The retail sanitizer test checks all index fields, frees both input files,
then requests every animation and verifies stable repeated lookups. It passes
for Fox, Falco, Peach, Kirby and Mr. Game & Watch. Fox has 327 index records,
278 nonempty animations and 315 nonnull script references. The decoder also
compiles for arm64 iOS. These tests validate native animation trees, not their
attachment to fighters during gameplay.

```sh
make -C native build/test-character-motions
UBSAN_OPTIONS=halt_on_error=1 native/build/test-character-motions native/build/PlFx.dat native/build/motion-checks/PlFxAJ.dat 327
```

This owner deliberately exposes action scripts as serialized offsets for the
pending native command conversion. It does not install partially converted
`Fighter_WaitAnimData` into the game. Per-character loader integration and
the remaining model/dynamics/item/command data are still required; runtime
startup remains stopped at Fox's archive.

### Native single-word fighter command payloads

`melee_fighter_command_single` converts the 39 single-word gameplay formats
used by the original action dispatch table. It assigns native bitfields with
explicit signed extension for model/texture indexes, part animation fields,
damage changes and other signed values. Parameterless commands retain their
opcode. Control flow, multiword commands and unsupported opcodes return false
without changing output. Target indexes still require runtime validation
against the active fighter; this API only converts representation.

The sanitizer test includes original `ftaction.c` in its translation unit so
it can execute the existing private command-variable and throw-flag handlers.
It checks all four command variables at boundary values, throw flags/timing,
negative model and texture indexes, signed field minima, all 39 opcodes and
transactional rejection. The tests and arm64 iOS converter compilation pass.

```sh
make -C native build/test-fighter-command
UBSAN_OPTIONS=halt_on_error=1 native/build/test-fighter-command
```

This converter is not yet wired into a native fighter script graph. Multiword
effect, hitbox, sound and other payloads remain to be converted before the
character loader can install native gameplay scripts. Runtime startup remains
stopped at Fox's archive, with no playable match established.

### Native multiword fighter command payloads

`melee_fighter_command_decode` extends the payload converter with seven
formats: effect (10), sound (17), throw hitbox (34), random sound (38), stage
sound (39), smash charge (56), and wind (58). Each format requires its exact
serialized word count and writes native `CmdUnion` slots only after successful
conversion. Signed coordinates and wind fields are explicitly extended;
sound IDs retain all 32 bits. This is representation conversion; fighter
indexes, random ranges and other semantic limits still need validation at
runtime.

The sanitizer test passes for all seven formats and invokes the original
effect handler with captured effect output. It verifies signed offsets,
ranges, bone selection, alternate bones, invisible-fighter suppression and
command advancement. The other six formats have field conversion checks;
their gameplay behavior has not been exercised by this test. Truncated and
unsupported commands leave output unchanged. The updated converter also
compiles for arm64 iOS.

Hitbox creation (11), footstep effects (54) and context-dependent effects
(55) remain unsupported. Their original handlers include byte-offset or
overlapping-structure reads that need native adaptations before conversion
can be safely enabled. The script graph and per-character loader are still
pending integration. No playable match is established.

### Footstep and context-dependent fighter effects

The payload converter now also supports commands 54 and 55. Their original
handlers use native `CmdUnion` arrays for temporary sound commands, copying
slots instead of reading console byte offsets. The sound handler explicitly
reconstructs the shared serialized behavior field from each command's native
fields. Context effects reconstruct the default effect ID from its two
bytes instead of a host-endian halfword overlay. Console paths are unchanged.

The sanitizer test executes both original handlers and captures their sound
and effect calls. It checks context sound overrides, default sounds, volume
and pan, fallback suppression, alternate foot bones, serialized versus
context-provided effect IDs, effect-only flags, command advancement and the
context handler's common callback. Nine multiword formats now pass alongside
the 39 single-word formats. This test captures callbacks rather than playing
audio or rendering effects. Both the converter and original interpreter
compile for arm64 iOS; the full native startup executable builds successfully.

Hitbox creation remains the unsupported gameplay payload. Script graph and
character-loader integration are still required before a playable match can
be verified.

### Fighter hitbox payloads and retail lookahead

`melee_fighter_hitbox_decode` converts the five hitbox words and additionally
requires the following serialized word. The US 1.02 DOL confirms that the
original handler advances past the five words, then reads bit 12 of the next
word for `hit_grabbed_victim_only` (`lbz r3, 1(r3)` at 0x80071570 followed by
the bit insertion). This differs from the similarly named field in the
hitbox header. The native header stores the confirmed lookahead flag in its
otherwise unused upper word, preserving the existing eight-byte `CmdUnion`
size. The handler also reads throw-dependent suppression from the fourth
native slot instead of console byte offset 15. Console code is unchanged.

The sanitizer test runs the original hitbox handler and checks hitbox reuse,
damage and angle callbacks, signed coordinate boundaries, shield damage,
knockback fields, direct and common bone selection, thrown-owner suppression,
command advancement and the lookahead flag. Tests deliberately set the header
and following-word flags to opposite values. The converter and interpreter
compile for arm64 iOS, and the full native startup executable builds.

The generic payload decoder still rejects opcode 11 because it cannot infer
the required following word. The pending script graph must use the dedicated
hitbox decoder and validate that lookahead against the source archive. These
tests do not establish gameplay collision behavior or a playable match;
character-loader and script integration remain outstanding.

### Shared native fighter script graph and motion ownership

`melee_fighter_actions_decode` builds one owned command arena from all supplied
script roots. It converts gameplay payloads, resolves call/jump pointers,
rejects overlapping operands and applies the hitbox lookahead from the source
archive. `melee_fighter_actions_script` exposes only valid command starts.
The existing callback-based action API retains its original behavior.

`MeleeCharacterMotions` now owns this arena alongside its motion index and AJ
bytes. `melee_character_motions_script` returns the native script for an
animation, or null for a missing script. This avoids allocating a full archive
arena separately for every animation. Script pointers live until the motion
owner is freed; they are not yet installed by the original character loader.

Retail sanitizer checks cover all main-motion scripts in five characters:

| Character | Script references | Reachable commands | Branches | Hitboxes |
| --- | ---: | ---: | ---: | ---: |
| Fox | 315 | 2119 | 82 | 200 |
| Falco | 315 | 2132 | 87 | 197 |
| Peach | 318 | 2658 | 83 | 212 |
| Kirby | 468 | 3109 | 133 | 270 |
| Mr. Game & Watch | 323 | 1843 | 91 | 193 |

The graph test checks command starts, operand exclusion, branch targets and
hitbox lookahead, then destroys the source file and checks every reachable
native command and branch pointer again. The combined motion test frees both
input files before requesting all animations and scripts. All five fixtures
pass. Synthetic tests additionally verify signed model fields, shared roots,
source-independent calls and rejection of a branch into an operand. Existing
action timer/loop/stack tests pass. Both modified modules compile for arm64
iOS, and the full native startup executable builds.

```sh
make -C native build/test-fighter-graph build/test-character-motions build/test-action
UBSAN_OPTIONS=halt_on_error=1 native/build/test-fighter-graph native/build/PlFx.dat 327
UBSAN_OPTIONS=halt_on_error=1 native/build/test-character-motions native/build/PlFx.dat native/build/motion-checks/PlFxAJ.dat 327
UBSAN_OPTIONS=halt_on_error=1 native/build/test-action
```

Graph conversion does not establish valid fighter indexes for every runtime
state or bounded execution in the original dispatcher. Character-loader
integration, the remaining archive components and runtime verification remain
required. No playable match is established.

### Native motion records and original animation-loader entry points

The motion owner now exposes `Fighter_WaitAnimData` records with owned names,
scripts, preserved flags, native-width animation identities and an owner/index
pair for lazy animation lookup. The alternate `ftData_80085FD4_ret` view uses
the same native field layout; its console pointer/size fields previously did
not line up with the motion record after widening. Native animation loop
checks now read bit 30 of the preserved flags instead of a host byte bitfield.

`ftData_80085A14` accepts the owned native records without replacing their
identities with console archive addresses. Native `ftData_80085CD8` and
`ftData_80085E50` retrieve cached decoded FigaTrees through the record owner
instead of copying and reparsing serialized AJ data into console buffers.
They preserve primary/secondary animation identity fields and reject negative
or out-of-range requests. Console implementations remain intact.

The motion sanitizer test calls these original entry points for every motion
in Fox, Falco, Peach, Kirby and Mr. Game & Watch after freeing the main and AJ
source files. It checks record aliases, flags, script pointers, returned trees
and both identity fields. All five fixtures pass. The full native executable
builds; the owner, loader and animation consumer compile for arm64 iOS. Existing
warnings remain in the unconverted demo path and unrelated animation functions.

This exposes the runtime bridge but does not yet install a complete native
`ftData` through `ftData_8008572C`. Demo motion loading and the remaining
per-character archive components still need conversion/integration. No playable
match is established.

### Per-character dynamic bones and selector tables

`melee_character_dynamics_decode` owns the main/demo two-byte selector tables,
dynamic bone descriptors, collision spheres and per-selector integer chain
cutoffs. It reuses the item dynamics scalar converter for the common first
16 bytes of the dynamics header, then binds native fighter descriptors. The
owner retains the copied parameter arrays until destruction.

The original `ftDynamics.x10` declaration as `FigaTree***` is misleading:
`ftCo_8009CB40` consumes the entries as integer cutoffs, assigning them to
`bone_id` and comparing them against chain indexes. Peach's serialized rows
contain values such as 1, 2 and 0x100, without pointer relocations. The native
field and consumer now use integer arrays; console types remain unchanged.
No animation-tree decoding is applied to these integers.

Sanitizer tests pass for Fox (one dynamic set and one collision), Falco,
Peach (nine sets, 86 selector slots, 774 integer controls), Kirby and
Mr. Game & Watch. They compare both selector tables, counts and every cutoff
against the archive, dispose of source bytes, check owned dynamic descriptors
and reject more than ten dynamic sets. They do not simulate dynamic chains.
The decoder and original dynamics consumer compile for arm64 iOS, and the
full native startup executable builds.

```sh
make -C native build/test-character-dynamics
UBSAN_OPTIONS=halt_on_error=1 native/build/test-character-dynamics native/build/PlFx.dat 327 14
UBSAN_OPTIONS=halt_on_error=1 native/build/test-character-dynamics native/build/PlPe.dat 318 14
```

This component still needs binding by the complete per-character archive
loader. No playable match is established.

### Per-character model-part metadata

`melee_character_parts_decode` owns ftData entry 0x08: all four visibility
lookup tables per costume, their nested drawable choices, costume texture
selectors and five bone IDs. Nullable costume entries remain null so the
original initializer retains its costume-zero fallback. Texture selectors
are converted from big-endian halfwords. Counts and bone IDs are bounded;
drawable indexes are checked against the engine's 124-object maximum and
still need validation against the particular loaded costume.

Retail sanitizer tests pass for Fox (four costumes, one group), Falco (four,
one), Peach (five, seven), Kirby (six, two) and Mr. Game & Watch (four,
eleven). They compare every drawable index, texture selector and bone field,
free the source file, then execute original `ftParts_8007487C` for every
costume. Tests verify fallback selection and exact hidden flags for both
normal and low-poly object lists while preserving unrelated flags. Invalid
bone limits are rejected. The decoder compiles for arm64 iOS, and the full
native startup executable builds.

```sh
make -C native build/test-character-parts
UBSAN_OPTIONS=halt_on_error=1 native/build/test-character-parts native/build/PlFx.dat 4
UBSAN_OPTIONS=halt_on_error=1 native/build/test-character-parts native/build/PlPe.dat 5
```

The test uses drawable fixtures rather than rendering fighter costumes. This
owner still needs binding into the complete character archive loader. No
playable match is established.

### Character guard and metal model descriptors

`melee_character_models_decode` owns the guard joint tree and scalar at ftData
entry 0x20 and the metal model at entry 0x5C. Both trees use the existing scene
converter; validation objects are released while the descriptors remain owned.

The guard entry's `x0` is a joint descriptor, despite its console declaration
as a joint-pointer array. The original `x0[2]` accesses the child pointer at
console offset 8. Native code now uses a typed joint pointer and `->child`,
avoiding the incorrect offset produced by eight-byte array elements.

The HSD probe reloads both models after freeing the source archive, checks
finite matrices for every joint, removes the reloaded objects and verifies
HSD pool cleanup. Five retail fixtures pass: Fox 146 joints, Falco 134,
Peach 228, Kirby 92 and Mr. Game & Watch 106. These checks are registered as
`hsd_character_models_*` when the extracted files are present. The full native
startup executable builds, and both the decoder and guard consumer compile
for arm64 iOS.

```sh
cmake --build native/build/aurora-integration --target hsd_scene_probe melee_game_startup
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --character-models native/build/PlFx.dat
```

This validates descriptors and object loading, not guard gameplay or rendered
metal effects. The complete character-loader integration is still pending;
no playable match is established.

### Partial-body character animations

`melee_character_part_anims_decode` owns ftData entry 0x1C: up to five part
records, their bone-index lists, and ordinary SRT animation hierarchies with
copied FObj streams. The pointer arrays have no serialized counts, so callers
must supply explicit per-group animation counts. The retail fixtures use
4/4/3/4/4 for Fox and Falco, 4/4/3 for Peach and Mr. Game & Watch, and 3/3/3
for Kirby. These lengths agree with the adjacent referenced object boundaries.

The converter bounds bones, groups, animation counts, hierarchy depth and
node allocation; ancestor cycles are rejected. Object-ID references and
constraint animation are currently unsupported. Kirby's first two groups
contain literal -1 entries at animation zero. These remain pointer-width
unavailable sentinels and must not be dereferenced by a runtime consumer.

Sanitizer ownership tests pass after source disposal: Fox 232 joints/257
tracks, Falco 180/261, Peach 120/276, Kirby 46/111 with two unavailable slots,
and Mr. Game & Watch 56/108. Invalid bone limits are rejected. The full native
startup executable builds, and the decoder compiles for arm64 iOS.

```sh
make -C native build/test-character-part-anims
UBSAN_OPTIONS=halt_on_error=1 native/build/test-character-part-anims native/build/PlFx.dat 4 4 3 4 4
UBSAN_OPTIONS=halt_on_error=1 native/build/test-character-part-anims native/build/PlKb.dat 3 3 3
```

These checks validate owned descriptors and streams; they do not establish
correct application to live fighter bones. Runtime selector validation and
complete character-loader integration remain outstanding. No playable match
is established.

### Fox and Falco character articles

The item article decoder now supports Fox/Falco lasers, blasters and
illusion/phantasm articles. Their special attributes are verified scalar
layouts: ten floats each for laser/blaster and two for illusion. The existing
article converter owns common attributes, models, state animations and scripts.
The state counts are 2 for laser, 9 for blaster and 3 for illusion; the
blaster's two additional callback states have animation index -1 and add no
serialized animation records.

`melee_fox_items_decode` owns the four article slots consumed by the original
Fox/Falco OnLoad callbacks, retaining the unused null slot at index 3 for Fox
or index 2 for Falco. Both character probes pass after source disposal,
reloading all three article models through HSD and verifying pool cleanup.
The full native executable builds, and the owner plus extended scalar/article
converters compile for arm64 iOS.

```sh
cmake --build native/build/aurora-integration --target hsd_scene_probe melee_game_startup
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --fox-items native/build/PlFx.dat fox
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --fox-items native/build/PlFc.dat falco
```

These tests validate owned article data and model reloads, not fired lasers,
blaster interaction or illusion gameplay. The array still needs installation
by the complete character loader. No playable match is established.

After the Fox/Falco extension, all 50 registered `hsd_*` regression tests
pass (72.87 seconds); see `native/build/fox-items-hsd-regressions.log`.

### Fox/Falco normal-match character loader integration

`MeleeFoxData` now assembles the converted common/special attributes, model
parts, motions/scripts, partial animations, dynamic bones, collision data,
wait tables, guard/metal models, sounds, auxiliary data and character articles
into a native `ftData`. The assembled-owner probes pass for Fox and Falco
with 278 nonempty animations and 315 script references each after both input
files are freed. Demo motions are explicitly absent and the native demo
loader checks for their presence before accessing them.

`ftData_8008572C` installs these owners for Fox and Falco, reading main/AJ
files from the native disc cache or the existing file loader. The owners
remain cached for the process lifetime so descriptors survive match resets.
The windowless match harness now passes the previous Fox main-archive stop
and reaches costume loading (`native/build/match-fox-loader.log`).

Fox/Falco costume loader entry points now also use cached native scene and
material descriptors instead of parsing console pointers. Other character
loaders remain unconverted. Native builds, iOS compilation, assembled-owner
tests and the original motion-loader regression pass.

```sh
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --fox-data native/build/PlFx.dat native/build/motion-checks/PlFxAJ.dat fox
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --fox-data native/build/PlFc.dat native/build/motion-checks/PlFcAJ.dat falco
```

No playable match is established. Demo loading, other fighters, runtime
validation and remaining gameplay/platform paths still need work.

The next windowless run passes Fox's costume loader and reaches
`Fighter_Create` line 907. ASan then found `ftCo_8009F578` passing a five-float
console-layout literal as an `HSD_WObjDesc`, causing an eight-byte read past
the global (`native/build/match-fox-costume-loader.log`). The native literal
is now a typed world-object descriptor. A focused probe executes the original
fighter light loader 20 times, verifies position (0.57, 0.57, 0.57), frees each
light and passes cleanup checks. It also compiles for arm64 iOS. Full match
startup has not yet been rerun after this light fix.

```sh
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --fighter-light
```

### Runtime collision-line indexes after first fighter creation

The next native run passed fighter light setup and reached floor collision
initialization. `mpCheckFloor` computed a byte offset using truncated pointers
and divided by the console's eight-byte `CollLine` size; native lines are
larger. It now uses typed pointer subtraction for the native line index.
Eight endpoint traversal helpers also retain pointer width when addressing
collision lines by byte offset. Console paths remain unchanged.

A sanitizer test exercises a floor at nonzero index 2, exact intersection and
normal, skip-line filtering and both floor endpoint helpers. It passes, and
`mplib.c` compiles for arm64 iOS. The full windowless startup rerun now finishes
creation of the first fighter and attempts the second fighter's main archive
(`native/build/match-collision-stride.log`). The failing archive size matches
`PlNs.dat` (Ness) in the supplied disc's filesystem index; that character
loader is not converted yet. This is no longer the Fox or collision failure.

```sh
make -C native build/test-collision-indices
UBSAN_OPTIONS=halt_on_error=1 native/build/test-collision-indices
```

A match has not started rendering or become playable. Ness and other character
loaders, demo motions, runtime validation and platform gameplay remain open.


### Ness normal-match archive conversion

`ness_data.c` now owns the normal-match Ness `ftData`, including 326 motion
records (276 nonempty animations), 326 script roots, 54 special scalar words,
the common attributes, model parts, collision, dynamics, sounds, wait tables,
partial animations, guard/metal models and eleven item articles. The yo-yo
article owns two additional joint models and its material animation; pointers
are decoded separately from the scalar prefix. PK Fire has two attribute
floats while its pillar has three. Thunder trails have one lifetime float.
Demo motion records are still absent.

The original fighter loader now installs this owner and caches Ness costumes,
alongside the existing Fox/Falco loaders. The windowless Metal match run
`native/build/match-ness-loader.log` passes Ness main-data installation and
stops next in `efAsync_LoadSync` while decoding `EfNsData.dat`. The effects
parser currently assumes models are packed up to their first referenced
object; this archive contains model payload before those joint descriptors,
so its table extent needs separate validation. No effects workaround has
been installed, and Ness costume loading has not yet been reached in this run.

The Ness article probe reloads all eleven models after disposing of input,
reloads the yo-yo string and body, applies its material animation, and checks
HSD allocator cleanup. The assembled-data probe requests all 276 animations
and checks all 326 scripts after disposing of both main and AJ inputs. The
guard/metal probe checks 126 joint matrices. These probes pass. Six-character
wait/attribute regressions pass, including Ness's nullable idle table. Seven
changed production modules compile for arm64 iOS; both native harnesses build.

```sh
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --ness-items native/build/PlNs.dat
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --ness-data native/build/PlNs.dat native/build/motion-checks/PlNsAJ.dat
```

This remains a startup milestone, not playable gameplay on macOS or iOS.

After integration, all 53 registered `hsd_` regression tests passed in 69.50
seconds (`native/build/ness-hsd-regressions.log`).


### Ness effects and match pause scene

Ness's effects now use the explicit three-record schema corresponding to
`efSync_Spawn` model IDs 0x2710..0x2712. Their texture payload precedes the
first referenced joint, so that joint is not a valid table boundary. All
three models decode after input disposal and pass HSD cleanup. The menu
fixture remains 1/1; common effects remain 46/47 with the previously known
unsupported spline model 36. The effects converter compiles for arm64 iOS.
The next windowless match run (`native/build/match-ness-effects.log`) passes
both fighter creations and reaches pause initialization.

`gmpause.c` now uses an owned native scene descriptor, cached across match
resets. `scene_desc.c` preserves a static fog descriptor through the existing
environment converter; fog animations remain unsupported. Both GmPause.dat
and GmPause.usd pass source-free model loading, five background selection
frames, fog-value preservation, malformed-fog rejection and HSD cleanup.
The original pause setup runs successfully in the next windowless Metal
match attempt (`native/build/match-pause-loader.log`). That run stops in
`ifAll_802F390C` on the still-unconverted 901,564-byte `IfAll` HUD archive.
No match rendering or playable gameplay has been verified.

```sh
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --effects native/build/EfNsData.dat effNessDataTable
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --pause-scene native/build/GmPause.dat
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --pause-scene native/build/GmPause.usd
```

Both native harnesses build; `effects.c`, `scene_desc.c` and `gmpause.c`
compile for arm64 iOS. The new targeted tests pass.

The full registered HSD regression set passes: 56/56 tests
(`native/build/pause-hsd-regressions.log`).


### Match HUD archive

`hud.c` converts all eleven public symbols in `IfAll.dat` / `IfAll.usd`:
ten model tables (18 models and 36 animation variants) and the damage-placement
scene. It reuses the dynamic-model converter through a new offset-based entry
point, since these descriptors sit inside public model lists. Expected table
sizes preserve the original consumers' eight countdown entries and two stock
models. Malformed model references fail conversion. The archive bridge owns
all descriptors and answers the original `HSD_ArchiveGetPublicAddress` calls.

`ifAll_802F390C` now installs this bridge, cached separately for each saved
language across match resets. Both locale fixtures pass source disposal,
model reloads, application of every animation variant at frames 0..2, bad
model-reference rejection and HSD allocator cleanup. Both native harnesses
build; the three changed production modules compile for arm64 iOS.

The windowless Metal run `native/build/match-hud-loader.log` passes the HUD
archive and its damage, countdown, stock, timer, magnifier and name-tag
initializers. It reaches the later `un_802FF1B4` initialization and stops on
`IfCoGet.dat`, which still uses the console archive parser. The run still
has not rendered a match or established playable gameplay.

```sh
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --hud native/build/IfAll.dat
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --hud native/build/IfAll.usd
```

All 58 registered HSD regressions passed in 72.40 seconds
(`native/build/hud-hsd-regressions.log`); the final stricter HUD checks also
pass independently for both locale fixtures.


### Coin-get interface, animated background flashes and damage-joint traversal

`ifcoget.c` now caches an owned `IfCoGet.dat` scene and uses it in the original
camera/light/model initializer. The scene probe applies all count frames
0..100 after disc bytes have been destroyed. That test and both pause-scene
regressions pass. The next full startup run passes HUD initialization and
reaches animated background-flash setup (`native/build/match-coget-loader.log`).

`lb_0219.c` converts the 16-entry `LbBf.dat` color table with the existing
native color interpreter (one empty entry and 15 scripts). Its user-data
allocation now uses the native `BgFlashUserData` size/alignment; clearing and
triggering effects use the typed overlay and actual GObj user-data fields,
replacing console byte offsets. A sanitizer test runs all 15 scripts after
source disposal, observes 940 color transitions (13 scripts finish within
600 frames), and exercises the original reset, trigger and per-frame flash
callbacks against a native GObj. This does not test GPU compositing.

```sh
make -C native build/test-bg-colors
UBSAN_OPTIONS=halt_on_error=1 native/build/test-bg-colors native/build/LbBf.dat
```

The following startup run (`native/build/match-bg-loader.log`) gets past
background-flash initialization and stops during damage HUD creation.
`ifStatus_802F6194` was treating a JObj as a GObj because their console
next/child offsets matched; the native offsets differ. Its native path now
walks JObj child/next fields directly. The percent-sign texture animation
also uses its actual material-animation types, and HUD player clearing uses
the full native array size. Both HUD fixtures verify the original traversal
returns the four expected joints and apply the percent-sign texture track.
Both native harnesses build; all three changed original-game modules compile
for arm64 iOS. One existing pointer-truncation warning remains in the damage
HUD death-animation path and needs a separate runtime audit.


The HUD-joint run (`native/build/match-hud-joints.log`) passes damage HUD
creation and reaches actual match updates. It stops when the Onett animation
updates a texture. A diagnostic rerun reports image value 0 with native table
count 0 (`native/build/match-texture-index.log`). The separate stage texture
binder `grAnime_801C6710` assigned its image table but omitted the native count;
it now copies `n_imagetbl` at the same time. The existing HSD binder already
did this, which explains why the earlier descriptor tests passed.

`--stage-playback` now binds all six Onett models through the original
`grAnime_801C6C0C` path and advances each for 120 frames after input disposal.
This focused test passes, and `granime.c` compiles for arm64 iOS. The prior
full HSD run passed 59/59 tests in 77.02 seconds; the additional Onett playback
test passes separately. The image-index diagnostic remains to identify any
future malformed or incorrectly bound table. These tests still do not prove
playable gameplay or successful GPU rendering of a match.

```sh
UBSAN_OPTIONS=halt_on_error=1 native/build/aurora-integration/hsd_scene_probe --stage-playback native/build/GrOt.usd
```


The next full run (`native/build/match-onett-metadata.log`) passes the texture
animation check and reaches GX draw processing. It aborts in Aurora's FIFO
worker while `push_gx_draw` calls the buffer-append helper in
`lib/gfx/recording.cpp`. The macOS crash report
`melee_game_startup-2026-09-10-051939.ips` identifies this worker stack; the
captured log contains no game assertion or sanitizer report. The next task
is to inspect the draw-array sizes and recorder buffer state at that abort.
No completed match image has been captured, so rendering and playability
remain unverified.


### Indexed draw-buffer capacity at match startup

The Aurora FIFO abort was its mapped ByteBuffer capacity check. Diagnostic
run `native/build/match-buffer-diagnostic.log` reached 8,370,432 bytes and
requested 8,390,072 bytes against an 8,388,608-byte allocation. No indexed
array exceeded 1 MiB in that run. The pinned backend patch now provides a
16 MiB indexed-data storage buffer and reports used/requested/capacity when
any mapped ByteBuffer overflows. GPU and staging allocations share that
constant. The temporary per-array diagnostics were removed.

With that capacity, `native/build/match-storage16.log` completes the first
match drawing submission with 11,723,784 bytes of indexed data. The following
update fails at common effect 36, which still contains an unsupported spline.
The archive has one type-2 B-spline joint with nine parameter knots and eleven
control vertices; decoding and animation binding still need implementation.
The 16 MiB capacity is verified for these initial Fox/Ness/Onett submissions,
not all stages, rosters or player counts.

The startup harness now records storage usage and captures up to three early
high-storage presentation checkpoints. A first checkpoint still showed the
loading screen, so drawing submission alone is not evidence of a visibly
rendered or playable match. GX ABI and misc tests pass; the pinned Aurora
checkout matches the persisted patch and `prepare_aurora.py` accepts it.


The third presentation checkpoint in `native/build/match-presentation-capture.log`
now captures actual native match rendering: `native/build/first-match-frame.png`
shows Onett, a 2:00 timer and the Ready countdown. The early loading-screen
checkpoint was the previous latched XFB. This image has visible HUD errors
(stock portraits and franchise icons do not match the loaded Fox/Ness pair),
and fighter-entry rendering is incomplete. The run still aborts on effect 36
before controllable gameplay. This is evidence of an initial rendered match
scene, not playable macOS or iOS support.


### Common spline effects and native particle emission

The scene bridge now owns and validates HSD spline descriptors, control
vertices, parameter knots and arc-length polynomials, and binds joint path
animations to the decoded spline joint identity. Common effect 36's original
animation advances through 120 frames with finite, changing joint matrices
after disposing the archive bytes. All 47 common effects decode. Malformed
spline knots and non-finite control vertices are rejected. The complete HSD
regression suite passes 61/61 tests (`spline-hsd-regressions.log`).

The next runtime failure was a truncated generator pointer passed through
`hsd_80398F0C`. Its command/generator integer arguments and the generator-list
address cursors now retain native pointer width while preserving console
32-bit types. The runtime then reached particle rendering, where direct
writes to the GameCube FIFO address faulted (`match-particle-pointers.log`).
All 35 scalar FIFO stores in `psdisp.c` now route through native GX parameter
emission; the console macro expansion preserves the original scalar stores.
The renderer compiles for macOS and iOS ARM64. Generator callbacks and other
particle paths remain subject to runtime verification.

The first particle-rendering rerun hit the harness's 45-second watchdog at
submission 1506, before reaching the match. Match probes now allow 120 seconds
while retaining their 1900-submission cap; earlier scene probes keep their
existing timeouts. This timeout did not verify the particle fix.


The longer run (`match-particle-fifo-120.log`) passes particle rendering and
advances to submission 1630. ASan then finds a 16-byte quaternion accessor
copy into the 12-byte Euler vector in `fn_8002113C` during fighter joint
updates. The native Euler branch now uses a full quaternion temporary for
accessors, transfers only XYZ through the Euler math, and preserves W.
The modified file also compiles for iOS ARM64.

Two HUD frame-selection errors were confirmed against `disc-main.dol`:
`gm_80168B34` must retain the character ID as its default atlas slot, and
`gm_80168BF8` must return the called function's float result. Both are now
explicit in C. `test-hud-frames` exercises the original functions for all
33 character IDs and six costumes, plus transformed Zelda/Sheik, under
ASan/UBSan. It passes; disassembly evidence is in `hud-frame-ppc.txt`.


`match-hud-rotation.log` passes the Euler-copy failure and reaches submission
1643. Its early capture (`hud-rotation-match.png`) now shows the correct Fox
and Ness stock portraits and franchise marks. The next failure occurs in
Onett's stage-start callback: the list producer stored an `HSD_GObj*`, but
the consumer read the argument as `s32`. Both now use one shared typed
`GroundStartCallback` record with an `HSD_GObjEvent` callback. This preserves
the console layout and retains full pointer width natively. `ground.c`
compiles for iOS ARM64. Subsequent gameplay remains under verification.


The corrected stage-start list passes: `match-ground-callback.log` exits 0
at the 1900-submission limit. `ground-callback-match.png` shows Onett after
the countdown, timer 1:56.39 and correct 0% damage HUDs. This run held neutral
inputs after stage selection. Fighter models remain absent or malformed in
the image, so this is not evidence of a playable release. The match harness
now reports fighter positions, motion IDs and visibility flags every 20
submissions after 1660, runs right at 1700–1729 and presses attack at
1800–1805 to exercise actual input response independently of rendering.


`match-input.log` also exits 0 at 1900. Fox's X position changes from -47.700
to -20.741 in response to the right-stick interval, with motion 15 while
moving and 14 afterward. Both fighters report draw=0 despite all joints
being unhidden. `UnkFlagStruct` overlaid PowerPC flag bytes with ARM's opposite
bitfield allocation order: initialization byte 1 enabled b0 instead of b7.
Its native little-endian declaration now reverses field allocation to retain
b0=0x80 through b7=0x01. `test-flag-byte` checks all 256 values and masked
writes under ASan/UBSan and passes. Runtime rendering is being rechecked;
the scripted attack's actual animation/hit behavior is not yet verified.


With fighter drawing enabled, `match-flag-byte.log` reaches the fighter's
custom material setup and asserts because a TEV constant template has the
wrong type. `ftmaterial.c` had cast `&ftMObj` to a fabricated structure
covering the following two separate static globals; native object sizes and
linker placement invalidate that alias. All template reads now use the actual
`ftMaterial_803C69D0` and `ftMaterial_803C6A44` objects. The modified material
source compiles for arm64 iOS. After the shared flag-byte correction, all
61 HSD regressions pass in 79.20 seconds (`flag-byte-hsd-regressions.log`).


`match-material-template.log` exits 0 at submission 1900 with both fighter
draw flags enabled. `material-template-match.png` shows Fox and Ness rendered
on Onett with their corrected HUD, timer 1:56.24, and Fox moved from the left
roof to ground level by the scripted input. The frame's indexed data uses
15,678,728 bytes, within the current 16 MiB allocation. Rendering this one
pair and stage is not validation for all fighters/stages or longer matches.
The attack input was sent, but the 20-frame telemetry interval does not
establish which attack animation or hit behavior ran. Interactive Apple app
integration, combat, audible output and device gameplay remain unverified.


### Extended match inputs and slope adjustment

The windowless startup harness accepts `--combat` to run through 2600 GPU
submissions, with a 120-second watchdog. It moves Ness toward Fox, sends
repeated attacks, a special move, jump and shield, and logs damage and each
motion-state transition (plus 20-frame position/visibility samples). These
are scripted backend inputs and never interact with host devices.

The first run (`match-combat.log`) terminated during character selection on
a GX GPU-completion timeout at submission 1295; no combat inputs had run.
The retry passed that point, then ASan found a stack write immediately before
`sp1C` in fighter slope adjustment at submission 1693 as Ness began moving
(`match-combat-retry.log`). `ft_80089B08` now uses its declared volatile
`line_len_sqrt` temporary on native builds, preserving the arithmetic and
console path. The change compiles for iOS ARM64. The same combat sequence is
being rerun before claiming those controls or hit behavior are verified.


The slope-adjustment rerun passes that fault and reaches submission 2484,
where shielding destroys particle generators and reveals another fabricated
cross-global structure in `hsd_8039D0A0` (`match-combat-slope.log`). Cleanup
now uses `hsd_804D0908` for list heads, `hsd_804D08E8` for joint references,
and `hsd_804D0F60.alloc_data` for deallocation directly. The focused original-
function test `test-particle-cleanup` passes ASan/UBSan for head/middle/tail
removal, generator/ID filtering, hooks, AppSRT cleanup, joint release and
allocator identity. The modified particle source compiles for iOS ARM64.

`match-combat-cleanup.log` exits 0 at submission 2600. The motion log records
Fox's jab and Ness entering damage motion 75 at submission 1804, with damage
rising from 0 to 4. Later both fighters receive 30 damage from the Onett car.
The run also reaches jump and shield states and captures the match at timer
1:44.61 (`combat-cleanup-match.png`). The first special-move input occurred
during hitstun, so the combat harness now retries neutral B at submission
2620 and down B at 2720, with its cap extended to 2800. These later special
moves still require runtime verification.


`match-combat-special.log` exits 0 at submission 2800. At 2621–2653 Fox
transitions through neutral-special states 341/342/343 and returns to idle;
Ness's damage rises from 46 to 49. At 2721 Fox enters reflector state 360,
Ness takes five more damage and enters damage motion 80 with knockback, and
Fox proceeds through 361/363 back to idle. This verifies these moves and hit
processing for the current matchup, not all characters or moves.
`make -C native test-match-runtime` runs the HUD-frame, flag-byte and particle-
cleanup regressions; all three pass. The earlier 61 HSD regressions remain
passing; this turn's changes are in fighter slope adjustment and particle
cleanup, covered by the extended ASan match and focused cleanup test.

Next integration work is to connect the actual game runtime and framebuffer
to the existing Apple controls app, preserving one owner of HSD frame/input
processing. Its current UI sample function calls `melee_hsd_input_frame` for
preview; an integrated game must own that processing itself. Mac GUI checks
must use the test VM per the parent AGENTS.md; no host screen/input automation.


### App runtime and Mac game bundle

`native/include/melee_runtime.h` exposes a dedicated-thread, process-lifetime
game session, a mutex-protected RGBA framebuffer copy, status/error reporting
and pause/resume. `native/aurora/game_runtime.cpp` runs the original game and
Aurora rendering without an SDL window, copying the latched XFB into a reusable
readback buffer. The UI receives independent 640x480 RGBA snapshots. This
initial path includes GPU-to-CPU readback each frame; UI/game frame rate and
long-session memory behavior still need measurement. Session restart/teardown
inside the same process is not implemented; the Mac app uses a single window
and terminates when that window closes. Reset requests report a restart message
and park the game thread instead of terminating the UI abruptly.

`melee_runtime_probe` starts the runtime on a worker thread, checks bounded
frame copies, pauses and resumes, and saves a nonblack frame at sequence 180.
The original and macOS 26-targeted probes pass ASan (`app-runtime-probe.log`,
`app-runtime-macos26-probe.log`). The Swift app's `MELEE_GAME_RUNTIME` build
publishes PAD snapshots without running the preview's HSD input init/frame
processing. Its game screen displays runtime frames and reuses keyboard
remapping, controller discovery and touch controls. The normal platform build
remains separate. Existing Apple input-model regressions pass for remapping,
persistence, controller slots, disconnect and focus handling.

Build the actual Mac bundle after preparing/configuring Aurora:

```sh
cmake -S native/aurora -B native/build/aurora-integration -DCMAKE_OSX_DEPLOYMENT_TARGET=26.0
cmake --build native/build/aurora-integration --target melee_game_runtime melee_runtime_probe
python3 native/tools/build_runtime_app.py
```

The result is `native/build/macosx-game/MeleeNative.app`. The packager reads the
runtime's actual minimum macOS version, includes transitive non-system dylibs,
rewrites their dependency paths and ad-hoc signs the result. Current build:
macOS 26 minimum, ASan-instrumented, bundled dependencies/signature verified.
It is a development bundle, not a distributable release or verified playable app.
This packager supports macOS; the separate iOS runtime packager is described below.

The app was installed and launched in the private macOS 26.6.2 VM. A stack
sample confirmed the original disc-open path on its background thread. An
initial VM Downloads access prompt blocked that open, so the test-only image
was moved to `/Users/logan/MeleeTestAssets/Melee.ciso`. The subsequent launch
created Dawn and pipeline caches. VM screen capture was black because
`CGSSessionScreenIsLocked=Yes`; the guest console account is logged in but
locked. The user has been asked to unlock the VM. No host GUI was driven.
The current bundle is installed there with `MELEE_DISC_IMAGE` pointing to the
unprotected fixture and `ASAN_OPTIONS=detect_leaks=0`, ready for UI verification.
No claim of UI rendering or keyboard/controller gameplay follows from the
successful process launch and cache creation alone.


### Game audio integration and animation flag words

The app now owns `MeleeAudioOutput` after its first game frame, starts the
original AX mixer and AVAudioEngine output, and stops them on focus loss or
when opening controls. Audio errors are shown separately from the game frame.
`melee_runtime_probe --audio` uses an offline consumer with the real game;
its 600-frame run produced 343,840 PCM frames and 402,108 nonzero samples,
including producer stop/resume. Apple manual-rendering audio tests also pass
conversion, mixer lifecycle, interruptions and configuration recovery. Running
that Apple test inside the filesystem sandbox failed system audio-component
lookup; the same offline test passed with that lookup permitted. No host audio
device was opened during these tests.

The new `--combat-audio` harness runs the full scripted match with the real
mixer and a paced offline PCM consumer. Its first run reached 2800 submissions
but timed out joining the audio producer while HSD held the interrupt gate.
The harness now releases that gate before joining. The app runtime likewise
releases it during framebuffer readback and UI pause waits, and before parking
on a reset request, so audio and VI workers can proceed. The pause regression
now stops the producer after the game has entered its pause wait.

The next combat run reached a ledge-grab animation and failed in `ftPartsRemap`
with a source character ID of 48 (`match-combat-audio-gate.log`). Fighter's
`x594` union overlaid a scalar archive word with native byte/bitfield order.
Its little-endian layout now preserves all PowerPC bit positions: character
ID in bits 0–5, root selector in bits 6–8, animation mask in bits 9–21, and
named flags in bits 24–31. Retail disassembly confirms the low-six-bit kind
read and the three-bit root selector (`animation-kind-ppc.txt`,
`change-motion-ppc.txt`). `test-animation-flags` passes 10,000 read/write cases
under ASan/UBSan and is part of `test-match-runtime`; it compiles for iOS ARM64.

With both fixes, the 2800-submission combat run exits successfully and produces
1,189,280 PCM frames with 2,231,098 nonzero samples
(`match-combat-audio-flags.log`). The stronger app pause test also passes:
600 game frames, 270,560 PCM frames, and producer shutdown while the game is
paused (`runtime-audio-pause-probe.log`). All 61 HSD regressions and the match
regressions pass. The rebuilt Mac bundle passes deep signature verification
and is installed in the still-locked private VM. These offline checks do not
establish audible fidelity, device playback or interactive gameplay.

### iOS device runtime build

The full original game, native backends, Aurora GX renderer, pinned Dawn iOS
package and Apple controls UI now link for iOS ARM64. The runtime uses static
third-party dependencies and links only system dynamic libraries/frameworks.
Its deployment target is iOS 17.0 (built with SDK 27.0). The GX adapter and
runtime now declare their Abseil header dependency explicitly, which had been
hidden by the Mac environment's include paths. The Mac runtime also rebuilds.

```sh
cmake -S native/aurora -B native/build/aurora-iphoneos -G Ninja \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
  -DAURORA_DAWN_PROVIDER=package -DAURORA_SDL3_PROVIDER=vendor \
  -DAURORA_CACHE_USE_ZSTD=OFF -DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew
cmake --build native/build/aurora-iphoneos --target melee_game_runtime -j 8
python3 native/tools/build_ios_runtime_app.py
```

The result is `native/build/iphoneos-game/MeleeNative.app`, with the runtime
embedded as `Frameworks/MeleeRuntime.framework`. The packager validates the
runtime platform and dependencies, rewrites the framework install name and
builds the same `MELEE_GAME_RUNTIME` Swift UI used on Mac. This is an unsigned
development bundle requiring signing/provisioning before device installation.
Both Mach-O binaries report platform IOS, minimum 17.0. Build logs are
`ios-runtime-build.log` and `ios-runtime-app-build.log` in `native/build`.
No physical iPhone is currently available to Xcode; this build has not been
installed or executed. The device-only Dawn archive cannot run in Simulator;
a separate source build is used for that target, as described below.

### iOS Simulator execution and text formatting

The pinned Dawn source builds for ARM64 iOS Simulator with the optional
protobuf IR serialization disabled. Its WGSL/Metal shader path remains enabled.
Use the iOS configuration above with build directory
`native/build/aurora-iphonesimulator`, `CMAKE_OSX_SYSROOT=iphonesimulator`,
`AURORA_DAWN_PROVIDER=vendor`, `DAWN_BUILD_PROTOBUF=OFF` and
`TINT_BUILD_IR_BINARY=OFF`. Then:

```sh
cmake --build native/build/aurora-iphonesimulator \
  --target melee_game_runtime melee_runtime_probe melee_game_startup -j 8
python3 native/tools/build_ios_runtime_app.py --sdk iphonesimulator
```

The simulator packager checks the IOSSIMULATOR Mach-O platform and ad-hoc signs
the app/framework. The device mode rejects Mac or simulator runtime binaries.
The windowless runtime probe passes 180 frames, bounded frame copies and
pause/resume (`ios-simulator-runtime-probe.log`). The 2800-submission combat
script also completes with Fox/Ness on Onett, 30%/34% damage and 1,396,160 PCM
frames with 2,642,730 nonzero samples
(`ios-simulator-selection-checkpoint.log` and `.png`). Audio was consumed
offline; no audio device was opened.

The first simulator combat run trapped in fortified `vsnprintf`: the original
SIS text formatter passed `-1` as the capacity of a 128-byte stack buffer.
Native formatting now supplies the real capacity, handles formatting errors,
and reserves enough encoded storage for the worst-case kerning/glyph expansion.
The console path is unchanged. `test-sis-allocator` now covers a long alternating
letter/digit string, truncation, text replacement, tail preservation and cleanup
under ASan/UBSan, and runs with `test-match-runtime`. Mac, iPhone and simulator
runtimes/apps rebuild with this correction.

Selection in the fixed-time combat script remains timing-sensitive: one run
selected Mario and hit the unadapted character archive path before the repeat
run selected Fox/Ness successfully. A capture at submission 1320 now helps
inspect both selections if a later stage load fails. This is a test limitation
and full-roster support remains unfinished.

The simulator app's first layout screenshot showed overlapping controls and
a small game region. Its iOS game layout now places independent touch targets
around a larger central game area; the platform preview keeps its old layout.
The revised simulator screenshot (`ios-game-live-layout.png`) verifies spacing,
but its game image was black at that point. A thread sample confirms the real
game loop, frame submission and readback are running (`ios-game-app.sample.txt`).
This does not yet verify app-level rendering or actual touch input. The
`MELEE_TEST_SILENT=1` environment option skips app audio-device startup during UI
tests. Physical devices are still unavailable and the private Mac VM is locked.

### Opaque app frames and real simulator touch events

The black SwiftUI game area was caused by zero alpha in the GX readback image,
not absent RGB pixels. VI output is an opaque display image, so the runtime now
sets alpha to 255 before publishing app-facing RGBA frames. The header documents
that contract and the runtime probe checks every output pixel's alpha. Both
Mac and simulator probes pass 180 frames, nonblack RGB, opaque alpha, pause/resume
and bounded copies (`mac-opaque-probe.log`, `simulator-opaque-probe.log`). All
three runtime targets and app bundles rebuild. The Mac bundle passes deep
signature verification and is installed in the still-locked private VM with
`MELEE_TEST_SILENT=1` for UI testing.

The simulator screenshot `ios-game-opaque-layout.png` shows the actual memory
card prompt through the SwiftUI app. `prepare_ios_ui_tests.py` generates a
separate XcodeGen project using the same four Swift sources and the real
simulator runtime framework. Its app has a separate test bundle identifier and
uses the local disc fixture, never host mouse/keyboard events or an audio device.
The project follows the [XcodeGen project specification](https://github.com/yonaskolb/XcodeGen/blob/master/Docs/ProjectSpec.md).
`GameUITests.swift` targets the actual control view's accessibility identifier
and sends XCTest press/drag events at its button/stick coordinates.

The initial A/Start UI test passes and its screenshots verify navigation from
the memory-card prompt to the main menu (`ios-ui-controls.xcresult`,
`ios-ui-controls-final.png`). XCTest's app-only screenshot had incorrect
landscape cropping on this simulator, so later tests use full-device captures
and an independent `simctl io screenshot` checkpoint. Passing the UI runner's
process assertions alone is not treated as proof of game navigation; screenshots
are reviewed. This initial test establishes startup touch navigation; the later
interactive session below reaches a match.

### Simulator app combat through touch controls

`ios-ui-stick.xcresult` verifies the same UIKit analog control reaching VS Mode
and character selection. The optional file-driven XCTest session allows the
same controls to be exercised interactively without host GUI events:

```sh
python3 native/tools/prepare_ios_ui_tests.py --session-dir native/build/ui-session
xcodebuild test -project native/build/ios-ui-tests/MeleeUITests.xcodeproj \
  -scheme MeleeUITests -destination 'platform=iOS Simulator,id=YOUR_SIMULATOR_ID' \
  -derivedDataPath native/build/ios-ui-tests/DerivedData \
  -resultBundlePath native/build/ui-session.xcresult \
  -parallel-testing-enabled NO \
  -only-testing:MeleeGameUITests/GameUITests/testControlSession
# From another terminal, after state.json appears:
python3 native/tools/ios_control.py native/build/ui-session snapshot
python3 native/tools/ios_control.py native/build/ui-session stick --x 0.5 --duration 0.1
python3 native/tools/ios_control.py native/build/ui-session press --button A
python3 native/tools/ios_control.py native/build/ui-session quit
```

Use a fresh session directory and result bundle. The test bootstraps to character
selection, accepts one acknowledged command at a time, saves full-device frames,
and terminates the app on exit. It has a ten-minute limit after bootstrap.

`ios-control-session-2.xcresult` passed after selecting Fox, enabling a level-1
CPU Fox in the orange costume, choosing Onett and playing through the actual app.
Reviewed frames in `native/build/ios-control-session-2` include `frame-19.png`
(CPU attacking, P1 at 21%), `frame-20.png` (after touch A, CPU at 30%),
`frame-22.png` (P1 Pause) and `frame-24.png` (resumed, P1/CPU at 135%/60%).
Movement events were also delivered during combat. This establishes a playable
slice in Simulator, not a completed match or physical-device performance.
The session used silent audio. White rectangles in effects need investigation.
The boxed pause text was initially suspected to be a defect, but the archive
inspection below finds that appearance in the original textures.

The preceding session exposed an independent crash when left at the title:
attract mode tried to load an unadapted stage archive and asserted through
`grDatFiles_801C5FC0` / `lbArchive_InitializeDAT`. Bootstrapping promptly avoids
that path for this focused test; attract mode itself remains unfixed.

### Reproducing pause-text rendering without an app

The windowless startup harness accepts `--combat-pause`. It follows the existing
Fox/Ness combat script, presses Start at submission 2750, releases it at 2756,
and captures the paused game at 2800. `pause-render.log` completes successfully
and `pause-render.png` reproduces the boxed/inverted-looking pause glyphs on Mac
Metal as well as Simulator. This isolates the defect from SwiftUI composition
and touch-event handling; it is not yet a graphics fix. The original font atlas
has normal zero-intensity backgrounds, so simply inverting the font data would
not be an evidence-based correction.

```sh
ASAN_OPTIONS=detect_leaks=0 native/build/aurora-integration/melee_game_startup \
  native/build/simulator-test.ciso native/build/pause-render-cache \
  native/build/pause-render.png --combat-pause
```

The optional `MELEE_STARTUP_GX_DUMP=PATH` writes the decoded final GX blending
and TEV alpha state during the paused submission. `pause-state.log` and
`pause-final-state.txt` show source-alpha/inverse-source-alpha blending and a
texture-times-register alpha expression. This is end-of-frame state, not proof
that every earlier draw used it. A temporary per-draw SIS hook produced no calls
in the paused frame (`pause-sis-trace.log`); the hook was removed. The visible
pause scene is loaded from `GmPause` through `gmpause.c`, so its animated scene
textures/materials are the next diagnostic target. The default font-byte check
does not validate those scene textures.

Direct inspection of `GmPause.usd` finds the borders and negative-looking
lettering in its original I4 images (descriptors `0x1048`, `0x10f4`, `0x13a4`,
`0x1450`). The label materials use texture blending 1.0 and source-alpha blending.
Re-extraction from the user's CISO produces the same SHA-256 as the test archive:
`3e8490299edf670af0aebb7f27117e0c61297d6b9e2af7ab5c443091e554907f`.
Thus the boxed label is not established as a native rendering defect, and no
texture inversion or material workaround was applied. Earlier descriptions of
it as incorrectly rendered were premature.

`--match-finish` extends the combat harness to 10,000 submissions with a
480-second watchdog to exercise the timed-match/results transition. Reaching
the submission limit alone still requires inspecting the resulting scene.

### Fighter death and match-exit layouts

The extended match first crashed at submission 4228 in the damage HUD's death
animation (`match-finish.log`). That code cast `IfDamageState` to a structure
with fixed GameCube padding. Native pointer sizes move its arrays. The native
animation now uses the owning state's digit pointers, velocity arrays and
`randomize_velocity` flag. The focused `hud_death_native` test checks all four
digit trajectories, gravity, bounds, pointer preservation and unaffected fields
against the real runtime under ASan.

With that correction, the match reaches submission 8964 and exits, but the
bonus calculation reads a null fighter for an unused slot
(`match-finish-hud.log`). `lbl_8046B6A0_24C_t::x0` was incorrectly typed as a
pointer even though it stores a frame counter and overlays `MatchEnd::x0`.
It is now `u32`; the two frame-counter comparisons use that type. The
`match_end_layout_native` regression checks the total size, player/bonus offsets,
timer, outcome and active/unused slots across both result views. The complete
match replay after this change is recorded in `match-finish-layout.log`;
results-screen success must be verified from that run, not assumed from the
layout test.

That replay fails earlier, at submission 3431, while rendering a spawned item:
`HSD_RObjUpdateAll` calls `HSD_ByteCodeEval` with an out-of-range argument operand.
The failing condition is `operand < nb_args`. This is a separate unresolved
item-expression path; the extended scripted runs do not currently reproduce
identical item behavior. The result-layout fix therefore has structural
regression coverage, but its full results transition remains unverified.

`gmResultLoadArchive` now decodes both `pnlsce` and `flmsce` into owned native
scene descriptors, cached separately for each language. The GameCube loader
remains on the console path. `hsd_results_dat` and `hsd_results_usd` load every
model, animate 120 frames after the archive buffer is destroyed, and verify HSD
cleanup. These two tests plus the HUD/layout regressions pass in
`match-exit-regressions.log`. Device and simulator runtimes also rebuild with all three changes
(`iphone-match-layout-build.log`, `simulator-match-layout-build.log`). None of these checks alone establishes a working results screen.

All 65 selected HSD/HUD/result-layout regressions pass after the shared type
correction (`match-layout-hsd-regressions.log`, 75.93 seconds). Mac, simulator
and unsigned device app bundles rebuild with these changes; packaging logs are
`mac-match-exit-app-build.log`, `simulator-match-exit-app-build.log` and
`iphone-match-exit-app-build.log`.

### Fire Flower expression-list correction

`--flower` runs the existing combat setup and spawns a Fire Flower through the
original item-spawn function at submission 1850. It immediately reproduces the
item-expression assertion (`flower-probe.log`): argument zero was requested,
but the expression had zero arguments. `loadRvalue` built its list through an
`HSD_Rvalue*` cast of an unrelated stack `HSD_SList`; the optimized game build
returned an empty head. The native branch now uses a typed head/tail builder.
The console implementation is unchanged.

`robj_list_game_native` links the actual optimized full-game objects and checks
three argument records and repeated allocation/cleanup. It fails with the old
builder (`robj-list-negative.log`) and passes after restoring the fix
(`robj-list-restored.log`). This matters because the ASan/UBSan scene harness
had passed its expression checks while the optimized game still failed.

The forced-spawn game run now completes 2800 submissions
(`flower-list-fix.log`); `flower-list-fix.png` shows the Fire Flower beside Ness
on Onett. A native bytecode failure now also reports the requested argument and
available argument count before the original assertion. Complete timed-match
verification after this correction is tracked in `match-finish-robj.log`.

All 66 selected HSD/game-layout/list regressions pass with the fix
(`robj-hsd-regressions.log`, 77.86 seconds). Both iOS runtime targets and all three
app bundles rebuild (`iphone-robj-build.log`, `simulator-robj-build.log`, and the
`*-robj-app-build.log` packaging logs). The private Mac VM was rechecked and is
still locked; these builds do not establish Mac GUI or physical-device testing.

The complete run with the expression-list fix passes bonus calculation and
enters sudden death at submission 8979 (`match-finish-robj.log`). It then crashes
in `ifMagnify_802FC3C0` while freeing a stale offscreen-indicator GObj. Its scene
initializer cleared only the original 0x74-byte prefix, leaving later player
handles uncleared on ARM64. It now clears `offsetof(ifMagnify, image_descs)`;
the descriptor suffix retains its original lifetime. The corrected replay
completes 10000 submissions (`match-finish-magnify.log`, exit 0), and
`match-finish-magnify.png` shows Fox and Ness in sudden death at 300% each.
That script leaves sudden death idle; it does not verify the results screen.
All three runtimes and app bundles rebuild with the correction
(`*-magnify-build.log`, `*-magnify-app-build.log`).

`--transition` shortens the active match timer to five seconds at submission
1850, then sends controller movement and attacks during sudden death. It has
a 4000-submission cap and a 180-second watchdog. This changes only the test
harness, not app match rules. `transition-combat.log` reaches the results
scene at submission 3141, where loading the fighter's victory/defeat archive
still fails in the original big-endian archive parser. The first missing
archive is `GmRstMFx.dat`, whose `ftDemoResultMotionFileFox` symbol owns the
combined motion bytes. Fox's normal `PlFx.dat` contains a separate 14-entry
demo motion table at ftData + 0x14: entries 0–9 use results motion bytes;
entries 10–13 use other demo archives. The native normal-match motion adapter
currently leaves this table uninstalled. A results adapter must retain the
full table indices and use the appropriate owned motion byte stream; treating
all 14 entries as offsets into the results archive would be incorrect.

A separate fresh-cache attempt (`transition-probe.log`) failed in the prize
unlock scene at submission 602. `ifprize.c` now decodes `IfPrize` through the
native scene adapter, caching one owned scene per language. Prize objects
borrow the cached descriptors, and cleanup does not pass them to the original
archive destructor. Both language fixtures pass source-disposal, model load,
120-frame animation, and HSD cleanup checks (`prize-tests.log`), alongside both
results-scene fixtures. The subsequent game replay (`prize-startup.log`)
passes startup and reaches the same unported results-fighter loader at
submission 3252. That replay does not independently establish that a prize
notification was displayed; the focused scene tests validate its adapter.

All 68 selected HSD, HUD, layout and optimized-list regressions pass after the
prize change (`prize-hsd-regressions.log`, 82.45 seconds). Mac, iPhone and
simulator runtimes and app bundles rebuild (`*-prize-build.log` and
`*-prize-app-build.log`). Results fighter loading remains an explicit runtime
failure, and neither physical iPhone play nor Mac GUI play has been verified.

The results motion adapter now decodes a selected range of a motion table while
preserving its full index space. Normal-match decoding uses the same path with
the entire table selected. Results loading owns entries 0–9 and their embedded
animation archive bytes; uninstalled entries have no native owner. Ness's entry
4 legitimately has an action script but no animation, so validation distinguishes
that case from an unloaded entry. `hsd_results_motions_Fx` and
`hsd_results_motions_Ns` check source disposal, every installed animation,
script ownership, preserved indices, and invalid range/stream rejection.
All 70 selected regressions pass (`results-motion-regressions.log`, 76.89 s).

`results-motion.log` passes the earlier archive parser failure and reaches
results camera creation. It fails because `CameraKindData` overlays separately
declared globals and assumes the GameCube addresses remain contiguous. The
native camera now uses the actual camera descriptor and copies offsets from
the named character tables. Results display images, GObjs, joints, and state
also now share one native `ResultsDisplayLayout` object rather than depending
on linker placement of four globals. Packed dimension and score constants are
unpacked explicitly into their original big-endian halfword order. Replay and
app verification of these follow-up layout corrections is still pending.


The layout replay completes 4000 submissions (`results-layout.log`, exit 0).
`results-layout.png` visibly shows the original results UI, Fox's winner model
and Ness's runner-up panel. This run predates the packed halfword correction;
`results-endian.log` is the follow-up replay of that final source state.
Mac, iPhone, and simulator runtimes and bundles rebuild with the final changes
(`*-results-layout-build.log`, `*-results-app-build.log`). Falco's results
archive also passes the owned-motion checks, alongside Fox and Ness
(`results-all-motions-tests.log`). These scene and windowless runtime checks
still do not establish physical-device or Mac GUI play.

The final byte-order replay also completes 4000 submissions (exit 0).
`results-endian.png` shows Ness as winner and Fox as runner-up, exercising the
opposite results animations from the preceding Fox-win replay. Match outcome
varies with the runtime input/frame timing; both completed through the original
results code. Returning from results to selection and starting a subsequent
match still requires verification.

`--rematch` extends `--transition` to 6000 submissions (260-second watchdog).
It presses Start on both human controllers at submission 4000, captures the
return screen at 4300, and presses Start again at 4400. The second stage-select
sequence moves the reset cursor back to Onett before confirming. A scene-state observer now requires
results, a subsequent return to character selection, and a second active VS
scene before accepting the final frame; reaching the frame cap alone is not
sufficient. The initial replay is `rematch.log`; the observer was added while
that replay was already running, so it requires a subsequent guarded replay.

The first rematch run exits results but its diagnostic fighter logger faults
at submission 4013 (`rematch.log`). Player slots retain the old fighter handles
while scene objects are torn down. The logger now checks the live fighter
GObj list before dereferencing a slot handle, including the object classifier.
This is a probe correction, not a change to gameplay or scene cleanup.
`rematch-guarded.log` is the replay with this correction and the scene-state
success check enabled.


`rematch-guarded.log` verifies return to CSS at submission 4014 and SSS at
4403. It correctly exits 6 at the cap because no second match was reached.
The stage cursor resets to its initial position instead of retaining Onett;
the second selection now repeats the original upward movement before A.
The live fighter list is walked backward from its tail so both players are
reported. `rematch-stage.log` is the replay of both probe corrections.

`rematch-stage.log` fails earlier, during sudden death at submission 3646,
with an ASan texture-table heap overread in `psDispParticles`. The allocation
is a decoded particle texture group from `melee_particle_bank_decode`; this
is a game-runtime failure, distinct from the previous diagnostic logger bug.
Native image and palette selection now assert with bank/group/index/count
before reading outside the decoded table. This diagnoses rather than fixes
the invalid selection. `rematch-particle.log` is the instrumented replay.
The final second-match path remains unverified.

The particle decoder omitted the original loader's `palnum == 0` fallback:
for indexed textures without the shared-palette flag, zero means `num`
palettes, not no palettes (`particle.c`, `psInitDataBankLocate`). Allocation
and pointer conversion now include these per-image palettes. The diagnostic
palette bound uses the same three layouts: shared, explicit count, and
per-image. No invalid particle index is clamped or ignored.

The focused synthetic test covers all three layouts and owned lifetime. A
separate binary using the old decoder fails with an ASan overread
(`particle-palettes-negative.log`); the corrected decoder passes
(`particle-palettes-tests.log`). The retail-bank test compares all palette
bytes against the original disc (`particle-palettes-disc-tests.log`). Common
bank group 9 has ten C8 images and zero palnum/palflag; the old 104-byte
allocation matches the reported runtime overread size. Group 3 also uses
this implicit per-image palette layout. All 71 selected HSD/HUD/layout/list
regressions pass (`particle-palettes-regressions.log`, 85.16 seconds).

The diagnostic replay before the palette correction completes the guarded
6000-submission rematch (`rematch-particle.log`, exit 0): results at 3133,
CSS at 4014, SSS at 4404, and the second VS scene at 4566.
`rematch-particle.png` shows the second Onett match with both fighters at 0%
and a fresh timer. This run does not reproduce the intermittent palette
overread. `rematch-palettes.log` is the full cycle with the corrected decoder.

The corrected palette replay completes 6000 submissions (exit 0), including
results, CSS, SSS and a second active match at submission 4565.
`rematch-palettes.png` visually confirms the second match on Onett.
All three runtime and app builds pass (`*-palettes-build.log` and
`*-palettes-app-build.log`). This establishes a repeat-match cycle in the
windowless native runtime; app UI repeat-match and physical-device checks
remain separate outstanding verification.

The rebuilt simulator app was exercised through onscreen controls in
`ios-control-session-3.xcresult`. Touch input selected Fox, enabled a CPU,
changed it to Ness, selected Onett and started a match. Frame 21 shows live
Fox-versus-Ness gameplay (Fox 33%, Ness 0%). The app then aborts when a Poké
Ball requests unconverted Goldeen article 161, before the next touch command.
The XCTest run fails; it is not evidence of completed touch-match verification.
Diagnostics are exported in `ios-control-session-3-diagnostics`.

Goldeen's three float attributes, model, three animation states and scripts
are now decoded and registered in the built-in Pokémon table. Both language
fixtures pass source-disposal and 120-frame animation checks
(`goldeen-tests.log`). `--goldeen` spawns it through the original item creation
function at submission 1850; `goldeen.log` reports creation and completes 2800
submissions without a crash. This is a focused spawn/lifetime check, not yet
a repeated simulator Poké Ball test.

Poké Ball's native special-attribute schema previously decoded only 8 bytes,
but the Pokémon spawning functions read 180 bytes of launch parameters and
selection weights. It now converts the 14 floats and 31 integers used by those
functions and allocates the full native descriptor with zeroed unused padding.
The scalar fixture check passes (`mball-params-tests.log`). Other Pokémon
remain unconverted; their normal selection is not disabled or redirected.

All 71 selected regressions pass after Goldeen and Poké Ball parameter changes
(`goldeen-regressions.log`, 78.29 seconds). Mac, iPhone, and simulator runtimes
and bundles rebuild (`*-goldeen-build.log`, `*-goldeen-app-build.log`). The
private Mac VM was rechecked and remains locked; all physical iPhones remain
unavailable. The simulator UI test must be repeated after broader Pokémon
support before a normal item-enabled touch match can be claimed reliable.

Chikorita, its leaf projectile, and Snorlax now have owned native article
registrations and scalar attribute schemas. The item-data fixture exercises
their animation states in both language archives after disposing of the
source bytes. All 71 selected regressions pass in
`pokemon-wave1-regressions.log` (87.00 seconds); the scalar fixture also passes
in `pokemon-scalar-tests.log`.

The first Chikorita replay exposed an eight-byte special-attribute allocation
being read at offset eight. Its source descriptor now declares the three
additional floats used for leaf position and velocity, and the decoder
converts all 20 bytes. The leaf itself consumes a separate four-byte timer.
`chikorita-behavior.log` observes the leaf at submission 1881 and Chikorita's
next attack state at 2084, then fails at 2163 when the temporary test Poké Ball
releases unconverted Scizor (article 171). This run is not a passing replay.
The focused fixture now removes its temporary ball after applying emergence
parameters; normal game Pokémon selection remains unchanged.

`snorlax-behavior.log` completes 2800 submissions and observes emergence state
2 followed by attack states 0 and 1. The harness requires these states, or an
active leaf for Chikorita, before accepting a replay. It also rejects spawning
outside an active VS match, making menu input timing failures explicit.
Mac, iPhone and simulator runtimes and app bundles rebuild successfully in
`*-pokemon-wave1-build.log` and `*-pokemon-wave1-app-build.log`. Physical-device
and complete Pokémon-roster verification are still outstanding.

`chikorita-fixture.log` passes the corrected focused replay through 2800
submissions (exit 0), observing emergence, attack state 0, an active leaf at
1881, and state 1 at 2095 without a sanitizer failure. This verifies the
Chikorita/leaf path in the native match runtime; it does not close the separate
Scizor or normal random Poké Ball coverage gaps.

Scizor (article 171) now decodes its 36-byte attribute prefix (six floats,
three integers) and owns its model and three animation records. Both item-data
language fixtures pass (`scizor-data-tests.log`), and the scalar test checks
44 schemas (`scizor-scalar-tests.log`). All 71 selected regressions pass in
`scizor-regressions.log` (91.24 seconds). All three runtimes and app bundles
rebuild in `aurora-*-scizor-build.log` and `*-scizor-app-build.log`.

`scizor.log` completes 2800 submissions without a sanitizer failure and observes
states 3 (emergence), 0 and 1. It does not observe departure state 2. The
`--scizor` probe was extended to 3600 submissions to investigate its longer
500-tick second attack. `scizor-lifecycle.log` fails that initial departure-state
requirement. Additional lifetime logging in `scizor-lifetime.log` shows its
timer advancing from 492 to 414, second attack phase active, position below
the stage, and removal at submission 2110. That run also fails the overly
strict departure-state requirement; it does not crash. Scizor's second attack
does not perform ground collision, so it can leave the stage before exhausting
the timer. The corrected probe requires states 0 and 1, the second attack
phase, and subsequent removal, without forcing any gameplay transition.

`scizor-complete.log` passes the corrected check and completes 3600 submissions
(exit 0) without a sanitizer failure, observing second attack phase at 2000
and removal at 2109. Departure animation state 2 is covered by the archive
animation fixture but was not reached in this gameplay path. Scizor support
does not establish full Pokémon-roster or random Poké Ball match coverage.

Blastoise and Hydro Pump now have owned native articles, with three model
animation records for Blastoise and one for the projectile. Its 32-byte
attributes contain seven floats and an integer attack-repeat count; Hydro
Pump consumes a four-byte lifetime. `blastoise.log` passes 2800 submissions,
observing water at 1959, finishing state 2 at 2231 and removal at 2269.
Both language archive fixtures pass (`blastoise-data-tests.log`).

Weezing and both gas variants are also registered. The parent consumes four
floats; the gas variants consume lifetime and velocity scaling fields.
`weezing-scalar-tests.log` passes all 49 registered common/Pokémon scalar
schemas, including these additions. The item-data fixture now loads all ten
converted Pokémon/projectile articles after disposing of the source archive
and exercises their animation records in both languages.

`weezing.log` passes 2800 submissions without a sanitizer failure, observing
gas variants 193 and 194 at 1909 and 1911, finishing state 0 at 2132 and removal
at 2212. Mac, iPhone and simulator runtimes and app bundles rebuild in
`aurora-*-water-gas-build.log` and `*-water-gas-app-build.log`. These are focused
native match checks, not complete random-item or physical-device coverage.

The broader regression run passes 69 of 71 tests; both item-data fixtures fail
their assumption that every projectile owns a joint. Retail gas articles 193
and 194 have null joint/model-animation pointers and spawn their visuals as
effects (`it_2725_Logic32_Spawned` / `it_2725_Logic33_Spawned`). The fixture now
explicitly validates those empty model fields for these two kinds and retains
the joint/animation checks for the other converted articles. The runtime code
did not need a change for this fixture failure.

Both corrected fixtures pass in `gas-fixture-tests.log` (28.67 seconds).
Together with the 69 unchanged passing checks, this resolves the failures
from `pokemon-water-gas-regressions.log`.

Charizard and its four flame variants now have owned native registrations.
The parent consumes a 48-byte attribute block; offset four is an integer repeat
count despite its shared source type declaring a float, so that word is
decoded as integer data. Each flame consumes two floats for lifetime and
velocity scaling. All four retail flame articles have empty model/animation
fields and spawn their visuals as effects; the archive fixture validates
those fields explicitly. The scalar fixture passes 54 schemas in
`charizard-scalar-tests.log`.

All three runtimes and app bundles rebuild in `aurora-*-charizard-build.log`
and `*-charizard-app-build.log`. The first `charizard.log` fails its active-match
guard at submission 1850 due to scripted menu selection; no Charizard was
spawned in that run.

All 71 selected regressions pass in `charizard-regressions.log` (102.71
seconds). A second replay, `charizard-match.log`, also fails before entering
VS; the 1320-submission capture confirms Fox and Ness are selected. The
probe now captures the framebuffer at its active-match failure to diagnose
the stage-selection input, rather than treating either run as Charizard
gameplay evidence.

`charizard-menu.log` reaches VS and observes all four flame kinds (195–198),
then fails at submission 2097 in `ftColl_8007BE3C`: the environmental damage
source is read as a truncated 32-bit pointer. Collision calculation had cast
two differently aligned fighter prefixes to a pointer-bearing `DmgResult`.
The native path now computes into a local result and copies named fields into
the chosen fighter record. The source field is a real `HSD_GObj*`; direction
and damage fields formerly misdeclared as pointer/integer are floats. The
console calculation retains its original overlay path.

`damage_result_native` checks both result destinations, a full-width source
pointer, fractional damage, position and neighboring record preservation.
The Charizard gameplay replay is repeated after this collision fix; the
earlier flame-spawn run remains a failed run.

`charizard-damage.log` passes 2800 submissions (exit 0) after the record fix,
observing all four flames, finishing state 3 at 2329 and removal at 2330.
Mac, iPhone and simulator app bundles rebuild in
`*-damage-result-app-build.log`; device runtimes rebuild in
`aurora-*-damage-result-build.log` and the Mac runtime in
`damage-result-final-build.log`.

All 72 selected regressions pass in `damage-result-regressions.log` (103.96
seconds), including the new damage-result check. Full random Pokémon roster,
remaining fighters/stages, and physical-device validation remain outstanding.

Moltres, Zapdos and Articuno now own native articles with three animation
records each. Moltres consumes four floats; Zapdos and Articuno consume three
floats and a timing integer. `birds-scalar-tests.log` passes 57 schemas.
The two language item-data fixtures now cover 18 converted Pokémon/projectile
articles, including source disposal and animation playback where models exist.
All three runtimes and app bundles rebuild in `aurora-*-birds-build.log` and
`*-birds-app-build.log`. The focused bird probes require attack states 1 and 2.

`moltres.log` passes 2800 submissions, observing states 1 and 2 at 1867 and
2046, and removal at 2264. `zapdos.log` also passes 2800 submissions, observing
states 1 and 2 at 1871 and 2089, and removal at 2288. Neither reports a sanitizer
failure. All 72 selected regressions pass in `birds-regressions.log` (113.83
seconds).

`articuno.log` passes 2800 submissions without a sanitizer failure, observing
states 1 and 2 at 1867 and 2077 and removal at 2189. These three focused
replays verify native attack-state execution; full random Poké Ball match
coverage and physical-device gameplay remain separate unfinished checks.

Wobbuffet and Bellossom now own native articles with two and five animation
records, respectively. Wobbuffet consumes nine floats and an integer lifetime;
Bellossom consumes a float scale and three integer timers. The scalar fixture
passes 59 schemas (`sonans-kireihana-scalar-tests.log`). The archive fixture
now covers 20 converted Pokémon/projectile articles in both languages after
source disposal. `--wobbuffet` requires a nonzero damage-reaction value, while
`--bellossom` requires entry into its attack state.

The first Wobbuffet replay (`wobbuffet.log`) fails its reaction requirement:
it spawns and is removed without a sanitizer failure, but the fighter at
x=10.354 attacks right while Wobbuffet is behind it at x=0. The focused
fixture now places Wobbuffet at x=20 to exercise its reaction through ordinary
fighter attacks, without directly invoking or forcing the reaction callback.
All three runtimes and app bundles rebuild in `aurora-*-sonans-kireihana-build.log`
and `*-sonans-kireihana-app-build.log`.

`bellossom.log` passes 2800 submissions without a sanitizer failure and
observes all five motion states (0, 1, 2, 4, 3). This is state execution
coverage; it does not specifically verify a fighter being put to sleep.
All 72 selected regressions pass in `sonans-kireihana-regressions.log`
(122.46 seconds).

`wobbuffet-reaction.log` also misses the reaction: the fighter moves past the
new spawn position before attacking. The probe now follows the live relative
position and facing direction through PAD input before jabbing.
`wobbuffet-steering.log` passes 2800 submissions (exit 0), observes an actual
damage reaction at 1942 and removal at 2539, and reports no sanitizer failure.
The steering is confined to the windowless test harness. Full random Pokémon
coverage and physical-device gameplay remain unfinished.

Entei, Raikou and Suicune now own native articles with one animation record
each. Their consumed special attributes are a float scale and integer timer;
the other fields in the shared source type are not read by these three kinds.
The scalar fixture passes 62 schemas (`beasts-scalar-tests.log`), and item-data
fixtures cover 23 converted Pokémon/projectile articles. The new probes
require the original accessory callback to set its active-attack flag after
spawning the corresponding effect (Suicune does not use a separate projectile
article).

`entei.log` passes 2800 submissions without a sanitizer failure and observes
attack-effect activation at 1989. All three native runtimes and Apple app
bundles rebuild in `aurora-*-beasts-build.log` and `*-beasts-app-build.log`.

`raikou.log` passes 2800 submissions, observing attack activation at 1974 and
removal at 2459 without a sanitizer failure. All 72 selected regressions pass
in `beasts-regressions.log` (129.95 seconds).

`suicune.log` passes 2800 submissions without a sanitizer failure, observing
attack activation at 1924 and removal at 2309. These probes establish the
three native attack paths; complete random-item matches and physical-device
validation remain outstanding.

Electrode now owns its native article and seven animation records. Its
consumed special attributes are a float scale and three integer charge/timing
parameters. The scalar fixture passes 63 schemas
(`electrode-scalar-tests.log`), and the item-data fixtures cover 24 converted
Pokémon/projectile articles. `--electrode` requires explosion state 6 followed
by removal, without forcing the transition.

`electrode.log` passes 2800 submissions (exit 0) without a sanitizer failure,
observing states 0, 1, 2, 5 and 6, with explosion at 2095 and removal at 2189.
Animation records 3 and 4 are exercised by the archive fixture,
but those states are not reached by this gameplay replay. All three native runtimes and app bundles
rebuild in `aurora-*-electrode-build.log` and `*-electrode-app-build.log`.

All 72 selected regressions pass in `electrode-regressions.log` (132.38
seconds). Random-item roster coverage, remaining fighters/stages, and
physical-device gameplay are still incomplete.

Unown and its swarm now have owned native articles. Each special descriptor
has nine scalar words followed by 26 joint pointers. The native loader decodes
the scalar prefix, allocates the correctly aligned native descriptor, and owns
all 26 variant scenes separately; no retail joint addresses are cast to host
pointers. The parent uses six floats and three integers; the swarm uses an
integer lifetime and eight floats. Scalar checks pass 65 schemas in
`unown-scalar-tests.log`.

The item-data fixture now checks 26 converted Pokémon/projectile articles.
For each Unown article, it loads every letter variant and plays its animation
records for 120 frames after source disposal. The gameplay probe requires a
live swarm article to appear through the original spawning path.

`unown.log` passes 2800 submissions without a sanitizer failure, observing
the swarm at 1972 and removal of the lead Unown at 2198. All three runtimes
and app bundles rebuild in `aurora-*-unown-build.log` and
`*-unown-app-build.log`. This checks a randomly selected swarm in gameplay;
the archive fixture supplies coverage of all letter models.

The broad regression run passes 70 of 72 checks. Both item-data fixtures
fail their extra default-model assertion after testing the lead Unown's
letter variants. Retail articles 172 and 199 have null default joint pointers
and zero bone counts: the letter model is selected at spawn. The corrected
fixture asserts that representation and tests the 26 variants for each
article instead of requiring an additional default joint. Runtime code is
unchanged by this fixture correction.

Both corrected item-data fixtures pass in `unown-variant-fixture-tests.log`
(68.94 seconds), completing variant checks for both the lead and swarm in
both languages. Together with the other 70 passing regressions this resolves
the fixture failures. Full random-item matches and physical-device gameplay
remain unfinished.

Lugia and all three Aeroblast articles now have native registrations. Lugia
owns six animation records and a 68-byte special block with sixteen floats
and one integer; each Aeroblast variant consumes its lifetime and velocity
scaling prefix. Retail Aeroblast articles have no joint models and render
through effects, which the fixture validates explicitly. Scalar checks pass
69 schemas (`lugia-scalar-tests.log`); item-data fixtures now cover 30 converted
Pokémon/projectile articles. `--lugia` runs 5000 submissions and requires all
three Aeroblast variants to appear through the original spawning code.

`lugia.log` passes 5000 submissions (exit 0) without a sanitizer failure. It
observes all six states, Aeroblast kinds 200/201/202 at 2211/2212/2214, and
removal at 2513. All three native runtimes and Apple app bundles rebuild in
`aurora-*-lugia-build.log` and `*-lugia-app-build.log`.

All 72 selected regressions pass in `lugia-regressions.log` (132.22 seconds).
Remaining Pokémon, fighters/stages, full random-item matches and physical-device
verification are still outstanding.

Ho-Oh and Sacred Fire now have native article registrations. Ho-Oh owns six
animation records and a 32-byte special block; its unused four-byte field is
preserved as raw padding. Sacred Fire owns its model and animation record and
consumes a float lifetime. Scalar checks pass 71 schemas
(`hooh-scalar-tests.log`), and item-data fixtures cover 32 converted
Pokémon/projectile articles. `--ho-oh` uses the extended 5000-submission probe
and requires a live Sacred Fire article from the original attack code.

`hooh.log` passes 5000 submissions (exit 0) without a sanitizer failure,
observing all six states, Sacred Fire at 2266, and Ho-Oh removal at 2497.
All three native runtimes and Apple app bundles rebuild in
`aurora-*-hooh-build.log` and `*-hooh-app-build.log`.

All 72 selected regressions pass in `hooh-regressions.log` (163.46 seconds).
Remaining Pokémon/content, random-item match coverage and physical-device
verification remain outstanding.

Mew and Celebi now own native articles with three animation records each.
Both consume four float attributes controlling scale and departure motion.
The scalar fixture passes 73 schemas (`mew-celebi-scalar-tests.log`); item-data
fixtures now cover 34 converted Pokémon/projectile articles. Their focused
probes require appearance/departure states 1 and 2 followed by removal.

`mew.log` passes 2800 submissions (exit 0): appearance/departure states 1/2
at 1863/1905 and removal at 2045. Both language item-data fixtures pass in
`mew-celebi-data-tests.log` (90.00 seconds). The first Celebi replay stopped
at the spawn guard because fixed stage-select input landed on a locked tile;
`celebi.png` captures that menu failure, not a Celebi gameplay failure.

The windowless probe now steers with normal PAD input toward the live Onett
tile center. A native read-only observation in the stage cursor update reports
horizontal/vertical guidance; it does not change stage availability, selection,
or scene transitions. Four centered observations precede A confirmation, and
stage steering overrides later fixed combat inputs while the menu is active.
`celebi-steering.log` passes 2800 submissions (exit 0): Onett confirmation at
scene frame 99, Celebi states 1/2 at 1864/1916, removal at 2057, and no
sanitizer failure. All three runtimes and app bundles rebuild in
`*onett-steering*build.log`.

All 72 selected regressions pass in `mew-celebi-steering-regressions.log`
(149.48 seconds). `rematch-steering.log` passes 6000 submissions (exit 0):
results at 3199, character selection at 4013, stage selection at 4403,
and the second active match at 4587. Both stage selections confirm the
centered Onett tile at scene frame 99. The final framebuffer shows the
second Fox/Ness match. `git diff --check` passes.

The private test Mac still reports a login window, and physical phones remain
unavailable in CoreDevice; iOS Simulator is connected. Complete content and
physical-device/controller verification remain outstanding.

Cyndaquil, its flame, Marill and Venusaur now have owned native articles.
Cyndaquil consumes three float parameters, with two animation records. Its
flame consumes six floats, including gravity at offset 0xC read by the shared
physics callback in `itmaril.c`; it is rendered through spawned effects.
Marill consumes six floats and one animation record. Venusaur consumes three
floats and two animation records. Scalar checks now pass 77 schemas
(`cyndaquil-marill-venusaur-scalar-tests.log`). Item-data fixtures cover 38
converted Pokémon/projectile articles.

`cyndaquil.log` passes 2800 submissions (exit 0), entering state 1 at 1864,
spawning a flame at 1917 and removing Cyndaquil at 2342, without a sanitizer
failure. Its two language archive checks pass in `cyndaquil-data-tests.log`
(107.28 seconds). `--marill` and `--venusaur` require state 1 and subsequent
removal from the live item list; `--cyndaquil` requires a live flame created by
the original attack code.

`marill.log` passes 2800 submissions (exit 0), observing states 0/1/2 at
1851/1863/1884 and removal at 2403. `venusaur.log` passes 2800 submissions
(exit 0), entering state 1 at 1864 and removing Venusaur at 2278. Neither
replay reports a sanitizer failure. The Venusaur final framebuffer shows
continued Fox/Ness gameplay on Onett. All three runtimes and app bundles
rebuild in `aurora-*-cyndaquil-marill-venusaur-build.log` and
`*-cyndaquil-marill-venusaur-app-build.log`.

Both final language item-data fixtures pass in
`cyndaquil-marill-venusaur-data-tests.log` (109.87 seconds), exercising all
38 registered Pokémon/projectile articles after source archive disposal.
`git diff --check` passes. Remaining Pokémon/content conversion and broad
item-enabled match, app UI and physical-device/controller verification
remain outstanding; these focused checks do not establish a complete port.

Staryu and its star projectile now have owned native articles, with two and
one animation records respectively. Staryu's 92-byte special block preserves
three integer counters at offsets 0x3C/0x40/0x44; its other parameters are
floats. The star consumes one float lifetime. Scalar tests pass 79 schemas
(`staryu-scalar-tests.log`); language item-data fixtures now exercise 40
converted Pokémon/projectile articles. `--staryu` requires an actual star
projectile created by the original targeting/attack code.

All three native runtimes and app bundles rebuild in
`aurora-*-staryu-build.log` and `*-staryu-app-build.log`.

`staryu.log` passes 2800 submissions (exit 0), observing states 2/0/1 at
1851/1867/1992, a star projectile at 1999 and removal at 2164, without a
sanitizer failure. Both language fixtures pass in `staryu-data-tests.log`
(111.84 seconds). `git diff --check` passes. Remaining Pokémon/content and
broad match/app/device/controller verification still prevent declaring the
complete port finished.

Chansey and its healing egg now have native article registrations. Chansey
owns four animation records and a 28-byte parameter block (four floats,
three integers). Its healing egg has no animation records and consumes a
float lifetime plus an integer healing amount. The item-data fixture loads
and releases its static model after source archive disposal. Scalar checks
pass 81 schemas (`chansey-scalar-tests.log`); item-data fixtures now cover
42 converted Pokémon/projectile articles.

`chansey.log` passes 2800 submissions (exit 0), observing states 0/1/2/3/4
at 1851/1864/1865/1885/1972, a live healing egg at 1884 and Chansey removal
at 2041, without a sanitizer failure. `--chansey` requires the healing egg
from the original egg-spawning code. All three runtimes and app bundles
rebuild in `aurora-*-chansey-build.log` and `*-chansey-app-build.log`.

Both language item-data fixtures pass in `chansey-data-tests.log`
(101.88 seconds); `git diff --check` passes. Healing-egg consumption has not
been explicitly exercised by this spawn-focused replay. Remaining content,
broad item-enabled matches and app/device/controller verification are still
outstanding for the complete port.

Porygon2 now owns its native model and two animation records. Its retail
article has no special-attribute block; the decoder accepts and preserves
that null field specifically for Porygon2 while retaining required-block
validation for other registered kinds. Item-data fixtures assert this null
field and exercise both animations after source archive disposal. There are
now 43 registered Pokémon/projectile articles; scalar schema count remains
81 because Porygon2 has no scalar block. `--porygon2` requires appearance
and dash states 0/1 followed by removal from the live item list.

All three runtimes and app bundles rebuild in `aurora-*-porygon2-build.log`
and `*-porygon2-app-build.log`.

`porygon2.log` passes 2800 submissions (exit 0), observing state 0 at 1851,
the dash state at 1949 and removal at 1974, without a sanitizer failure.
The final framebuffer shows continued Fox/Ness gameplay on Onett.

All 72 selected regressions pass in `porygon2-regressions.log` (175.47
seconds). `git diff --check` passes. Remaining Pokémon/content and broad
match, app UI and physical-device/controller verification still prevent
calling the complete port finished.

Clefairy and Togepi now have native article registrations with six and seven
animation records. Their parameter blocks consume 24 and 28 bytes: scale is
a float, while timing and random-selection weights remain integers. Scalar
checks pass 83 schemas (`metronome-scalar-tests.log`); item-data fixtures now
cover 45 registered Pokémon/projectile articles. `--clefairy` and `--togepi`
require a naturally selected effect state (2 or above) followed by removal.
These focused probes do not force the random selection or cover every effect.

`clefairy.log` passes 2800 submissions (exit 0), observing states 0/1/2 at
1851/1867/1986 and removal at 2141, without a sanitizer failure. All three
runtimes and app bundles rebuild in `aurora-*-metronome-build.log` and
`*-metronome-app-build.log`.

`togepi.log` passes 2800 submissions (exit 0), observing states 0/1/3 at
1851/1871/1998 and removal at 2143, without a sanitizer failure. Both
language item-data fixtures pass in `metronome-data-tests.log` (123.63
seconds). `git diff --check` passes. All random-effect branches, remaining
content, broad item-enabled matches and app/device/controller verification
are still outstanding for the complete port.

The unshortened `--match-finish` probe now observes live item kinds and
requires both the results scene and at least three distinct common item
kinds by 10000 GPU submissions. It retains ordinary item spawning and the
original match timer. The coverage observer does not spawn items or alter
game state; fighter projectiles and stage hazards do not satisfy the common
item requirement.

An archive audit finds positive ordinary Poké Ball weights for all kinds
161–179, 181–182 and 185–190 (27 kinds); each now has a native article.
Ditto (180), Mew (183) and Celebi (184) have zero ordinary weights; Mew/Celebi
already have focused native coverage. The item-data fixture now asserts a
native registration for every positive-weight outcome in the loaded table.
Ditto and the unknown article 207 remain unregistered, and this assertion
does not claim every random-effect branch has been exercised.

`full-items-match.log` reaches 10000 submissions without a sanitizer error
but correctly fails its results guard (exit 6): the tied timed match has
entered sudden death, with both fighters at 300% in the captured framebuffer.
It observes common kinds 31/0/16 at 3401/5381/7192, plus stage hazard 160 and
fighter projectiles 74/54. The probe previously supplied no inputs to resolve
a tie. It now holds player 2's stick right for 300 frames after the sudden
death intro and allows 12000 submissions, preserving the original timer,
item selection and scene transitions. This is test-only PAD input.

Both positive-weight registration fixtures pass in
`weighted-pokemon-data-tests.log` (123.08 seconds).

`full-items-sudden-death.log` passes 12000 submissions (exit 0), with ordinary
common items 21/6/34/19 first observed at 3246/5190/6852/8492. The unchanged
timed match reaches sudden death at 8995. Normal player 2 stick input at
9115 resolves it, and the game reaches results at 9368; the final capture
shows the results screen. No sanitizer failure is reported. The run also
contains fighter deaths/respawns. It does not establish CPU, four-player,
all-item interaction, all-stage/fighter or physical-device coverage.
`git diff --check` passes. Only test harness/fixture code changed in this
step; previously built apps retain the same production runtime.

The real iOS Simulator app control session now passes with the expanded
Pokémon registrations: `ios-control-session-4.xcresult` and
`ios-control-session-4.log` report one XCTest passed in 547.467 seconds.
The session uses only XCTest touches inside the simulator, with silent audio,
and terminates the app through the test's normal cleanup after command 33.
Diagnostics are exported in `ios-control-session-4-diagnostics`.

Screenshots in `ios-control-session-4` document the actual touch-driven flow:
frames 0–15 select Fox, CPU Ness and Onett; frame 16 shows the running match;
frame 17 shows Ness at 4% after touch A; frames 18–20 cover movement and the
P1 pause screen; frame 21 resumes; frames 22–23 show continued CPU gameplay;
frames 24–26 show Ness's winning animation/results; frame 27 returns to
character selection; frames 28–31 select Onett again; frame 32 shows a second
active Fox/Ness match at 1:56.47. The original two-minute CPU match completes
with three Fox falls, one Ness fall, and Ness winning. This extends the older
session 2 touch evidence and gets past the unsupported-Pokémon crash seen in
session 3, although it does not prove every random Pokémon/effect occurred.

A visual issue remains to investigate: frame 25 shows Fox's total as `2`
despite one KO and three falls (expected -2). Ranking and the winner agree
with Ness winning. The negative-score sign may be missing in the results
rendering; the current captures are evidence for follow-up, not a fix.
Physical iPhone/iPad, Bluetooth controller and private-Mac UI validation,
remaining content and broader gameplay coverage remain outstanding.

The results sign issue from iOS control session 4 is traced to packed
big-endian Shift-JIS constants used as C strings in `gmresult.c`. Native
little-endian storage placed a NUL first, hiding minus/plus signs and empty
slot placeholders. The native branch now uses explicit Shift-JIS byte strings
for the signs, three-dash placeholder and the `-:-` KO-time placeholder;
original console constants remain unchanged.

Mac, iPhone and Simulator runtime builds pass in
`aurora-{integration,iphoneos,iphonesimulator}-results-text-build.log`.
All three app packages pass in `{mac,iphone,simulator}-results-text-app-build.log`;
the device package remains unsigned and requires development provisioning.
The windowless `score-signs-retry.log` reaches 4,000 submissions and exits zero;
its viewed results capture shows the formerly missing empty-slot dashes.
That run has zero totals, so it does not verify a nonzero signed total.
The first `score-signs.log` exits 6 after scripted menus enter Home-Run Contest
instead of VS. The transition probe now captures its failure and reports
explicitly when it misses the active timed match. A further movement-only
attempt (`results-signed-score.log`, exit zero) reaches results but leaves Ness
against terrain before time expires; it is not signed-score evidence.

The final signed-score probe passes: `results-signed-jump.log` exits zero
after 4,000 submissions and `results-signed-jump.png` was visually inspected.
Scripted PAD jump/right input clears Onett's terrain, Ness crosses the right
boundary at frame 2120 and respawns by frame 2180 before the timer expires.
The real results screen displays Fox `+1`, Ness `-1`, Fox winning, and Ness's
one fall. This verifies both score signs through the original results/SIS/GX
rendering path. The time placeholder byte fix is build-verified but its
specific display branch has not been visually exercised here.
The transition harness retains this input to cover nonzero scores; normal
app inputs/game rules are unaffected. Probe rebuild and `git diff --check`
pass. This fixes the results-text issue, not remaining native content or
physical-device/controller validation.

The runtime costume cache now uses the original per-fighter costume counts
and fighter-kind indexing instead of restricting loading to Fox/Falco/Ness
and a fixed four costumes. Each loaded costume still owns native joint and
material-animation descriptors for the process lifetime, so later matches
can reload them after disc buffers and prior HSD objects are released.

`python3 native/tools/verify_costumes.py native/build/simulator-test.ciso`
passes all 131 retail costume archives and 135 model roots, including
`PlCaRe.usd`, bosses and wireframes. `roster-costumes.log` records the summary;
`costume-checks/results.json` records every archive, root, material symbol,
exit code and per-model log. The new `hsd_scene_probe --costume` mode destroys
the original archive, reloads each owned descriptor twice, attaches its
material animations, advances frames 0–120, inspects model counts and checks
HSD pool cleanup under ASan/UBSan. This is descriptor/HSD validation, not a
visual or gameplay check of every fighter.

The generalized runtime path also passes the 2,800-submission windowless
Fox/Ness combat check (`roster-costumes-combat.log`, exit zero). Its viewed
capture shows both fighters on Onett, Fox at 30% and Ness at 34%, with the
match still running at 1:41.84. Mac/device/Simulator runtime builds and all
three game app packages pass (`roster-costumes-build.log`,
`aurora-{iphoneos,iphonesimulator}-roster-costumes-build.log`, and
`{mac,iphone,simulator}-roster-costumes-app-build.log`). `git diff --check`
and Python compilation pass. Character-specific gameplay data outside the
three adapted fighters remains unconverted; costume coverage is a prerequisite,
not evidence that the entire roster is playable.

Mario/Dr. Mario conversion now includes their shared special-move parameter
schema and owned four-slot item tables. `melee_mario_attributes_decode`
converts the 0x84-byte Mario layout, treats the cape kind/tornado integers and
reflector bone/damage as integers, validates floating-point fields, and
preserves the final reflector behavior byte/padding without swapping it.
The original `ftMr_Init_OnLoadForDrMario` uses this same layout for Dr. Mario.

`mario-attributes-tests.log` passes retail Mario/Dr. Mario data plus existing
Fox/Falco/Ness attribute cases under ASan/UBSan. The new checks verify original
cape kinds, positive reflector dimensions/damage limits, reject NaN at every
floating-point field, accept the same bit patterns in integer fields, reject
every truncated block length without changing the destination, and preserve
copied data after the source is destroyed.

`melee_mario_items_decode` owns the original four article slots: Mario has
fireball/cape in slots 0/2; Dr. Mario has pill/sheet in slots 1/3; the other
slots remain NULL. Fireball and pill each use five float attributes. Cape and
sheet preserve their unused four-byte parameter block. Animation counts are
1/2 for fireball/cape and 6/2 for pill/sheet (pill gameplay state callbacks reuse
some animation records). `hsd_mario_items_Mr` and `hsd_mario_items_Dr` pass in
`mario-items-tests.log`: the source archive is destroyed, each article's model
is loaded, and every animation record runs through frames 0–120 using original
HSD, followed by pool cleanup. Projectile speed/lifetime values are checked
against the retail values as well. This does not yet test projectile behavior
inside a match.

Existing Ness items/data regressions pass in
`mario-prerequisites-regressions.log`; all three runtime builds pass in
`aurora-{integration,iphoneos,iphonesimulator}-mario-prerequisites-build.log`.
`git diff --check` passes. These new decoders are not yet connected to a Mario
ftData owner/runtime OnLoad path: remaining fighter descriptors and motions
must be assembled before Mario or Dr. Mario can enter native gameplay. App
packages were not rebuilt for these currently unreferenced prerequisites.

Mario and Dr. Mario now have assembled native fighter-data owners connected
to `ftData_8008572C`. They include common/special attributes, collision,
dynamics, guard models, all 303 normal-motion records, three part-animation
groups (4/4/3), five costume part tables, sounds, wait data and owned items.
Mario's demo dynamics use 16 entries; Dr. Mario's use 14, matching the original
fighter table. Results motions remain installed by the existing separate
results loader. The process caches these owners across matches.

The initial part-animation count audit overcounted the third group by reading
an adjacent model pointer as an animation pointer; both assembled decoders
correctly rejected it. After establishing the actual 4/4/3 groups,
`mario-data-tests.log` passes both retail data/AJ pairs. Each test destroys the
source buffers and decodes all 252 nonempty animation trees and 303 scripts,
loads the owned model and checks pool cleanup. `mario-final-tests.log` passes
all five Mario tests (both item tables, both assembled owners and the shared
effect archive); the latter contains two lazy effect models. All three runtime
builds pass in `aurora-{integration,iphoneos,iphonesimulator}-mario-data-build.log`.

`--mario` now selects Mario through scripted PAD input and requires observing
both Mario and his fireball before success. The initial `mario-combat.log`
missed the portrait and remained in character selection; its exit-zero
submission limit is not gameplay evidence. The corrected
`mario-combat-retry.log` passes 2,800 submissions, observes the original Mario
fireball at frame 2634, and exits zero. The viewed `mario-combat-retry.png`
shows Mario/Ness on Onett at 1:41.63 with 30%/39% damage. This proves native
Mario match entry/combat and a fireball spawn; it does not yet cover every
special, full-match results/rematch, CPU Mario or Dr. Mario gameplay.

All three game app packages pass in
`{mac,iphone,simulator}-mario-data-app-build.log`. `git diff --check` passes.
Physical devices/controllers, remaining roster/stages/modes and broader
Mario validation remain outstanding.

Mario now passes the native results/rematch path. The new `--mario-rematch`
probe uses the existing shortened-timer transition fixture and normal scripted
PAD menu input, requiring results, return to character selection, a second
active match, and a live Mario fighter at the final submission. The normal
`--mario` combat probe retains its fireball requirement; it now checks that
Mario is still present rather than relying on a historical observation.

`mario-rematch.log` exits zero after 6,000 submissions. It records first match
at 1587, results at 2304, character selection at 4014, stage selection at 4403
and the second active match at 4587. The viewed `mario-rematch.png` shows Mario
and Ness in that second Onett match with both damage counters at zero. This
checks the results transition and owner reuse across matches; it does not
constitute an unshortened CPU Mario match or physical-device validation.

The results-motion fixture now accepts the original demo-record count rather
than always assuming 14. Mario has 16 and Dr. Mario has 14; both retain the
first ten result records and leave the remaining slots unbound. Retail
`GmRstMMr.dat` and `GmRstMDr.dat` are extracted from the user's disc for these
checks. `mario-results-tests.log` passes all five results-motion tests for
Fox/Falco/Ness/Mario/Dr. Mario, including invalid ranges, source disposal and
original HSD cleanup. `mario-results-tests-build.log`,
`mario-rematch-build.log` and `git diff --check` pass. Only test code and its
registration changed this step; the previously packaged production apps are
unchanged. Dr. Mario gameplay and broader Mario specials remain unverified.

The Mario combat probe now covers all four special-move families. `--mario`
runs 4,000 submissions and requires a live Mario, a spawned fireball, a spawned
cape, the fighter's active reflector flag, and entry into Super Jump Punch
and Mario Tornado (using the original motion-state enums). Additional normal
PAD inputs request side/up/down specials; `--mario-rematch` keeps its separate
6,000-submission match-transition requirement.

The first `mario-specials.log` correctly exits 6: fireball, cape and Tornado
occurred, but an Onett car hit Mario before the scheduled up-special input,
leaving him in damage/ledge states. The revised input requests Super Jump
Punch earlier and retries it later. `mario-specials-retry.log` exits zero at
4,000 submissions, recording fireball at 2334, Tornado at 2722, cape at 3002,
reflector activation at 3007 and Super Jump Punch at 3081. The final captured
frame was inspected in `mario-specials-retry.png`; the match remains running.
This verifies move entry, item spawning and reflector activation in the
original game runtime, not every grounded/aerial variation, successful
projectile reflection, competitive accuracy or physical-device performance.
`mario-specials-build.log` and `git diff --check` pass. Only the test harness
changed, so production app packages from the Mario data step remain current.

Captain Falcon and Ganondorf now have native special-attribute decoders and
assembled fighter-data owners connected to the runtime loader. Both retain
318 motion records, 14 demo-dynamics entries and three 4/4/3 part-animation
groups; Captain has six costume part tables and Ganon five. Their original
item-table pointers are NULL and the decoder checks this instead of inventing
items. The 0x8c special-move block contains three integer fields and 32 floats.
`captain-attributes-tests.log` verifies both retail blocks, every float/integer
classification and all truncation boundaries, with Mario/Dr. Mario regression
cases. `captain-final-tests.log` passes both assembled owners and both effect
archives. After source destruction, the owner tests decode 275 animations/313
scripts for Captain and 264 animations/307 scripts for Ganon, then check cleanup.

Captain's effect test exposed a particle palette rejection: its C8 group has
`tlutfmt=0x01000002`, while the original renderer explicitly casts that word
to u8 for GX. `particle_bank.c` now validates the low byte, preserving the full
word, rather than rejecting upper-byte metadata. The synthetic palette tests
cover that retail value and reject low-byte format 3 in all three palette
layouts. `captain-particle-tests.log` passes; the common, Ness, Mario, Captain
and Ganon effect tests all pass in `captain-effects-regressions.log`.

The new `--captain` probe requires both a live Captain Falcon and Falcon Punch
motion before passing. `captain-combat.log` and `captain-combat-retry.log`
exit 6 because scripted horizontal input overshoots the portrait; neither is
gameplay evidence. Corrected input in `captain-combat-final.log` reaches native
Captain/Ness combat on Onett, observes Falcon Punch at frame 2622, completes
2,800 submissions and exits zero. Its final framebuffer was visually inspected
in `captain-combat-final.png`. This verifies match entry/combat and move entry,
not all Captain specials, results/rematches, CPU behavior or Ganon gameplay.

All Mac/device/Simulator runtime builds pass in
`aurora-{integration,iphoneos,iphonesimulator}-captain-particle-build.log`, and
all three app packages pass in `{mac,iphone,simulator}-captain-data-app-build.log`.
`git diff --check` passes. Remaining roster/stages/modes and physical-device,
keyboard/controller and broader gameplay validation remain outstanding.

Captain Falcon now passes native results and rematch validation. The new
`--captain-rematch` variant uses the shortened-timer transition fixture and
scripted PAD menu controls. It requires results, return to character selection,
a second active match and a live Captain Falcon at submission 6000. The
separate `--captain` combat probe retains its Falcon Punch requirement.
`captain-rematch.log` exits zero after 6,000 submissions: first match at 1588,
results at 2305, CSS at 4014, stage selection at 4403 and second match at 4587.
The final `captain-rematch.png` was inspected and shows Captain/Ness in the
second Onett match. This exercises cached fighter descriptors and results
motions across scene teardown/reload, not an unshortened CPU match.

Retail Captain/Ganon results archives now have registered fixture checks.
The first tests rejected an absent script in result record 4; both retail
tables contain that NULL pointer, which the production decoder preserved.
The fixture now records the retail presence of every script before destroying
the source and compares every decoded slot with it, rather than assuming all
ten records have scripts. `captain-results-tests.log` passes all seven results
motion tests (Fox/Falco/Ness/Mario/Dr. Mario/Captain/Ganon), including source
disposal, lazy animation-tree decoding, absent slots and invalid ranges.
`captain-rematch-build.log`, `captain-results-test-build.log` and
`git diff --check` pass. Only test code/registration changed this step; the
previous production app packages remain current. Ganon gameplay, Captain's
other specials, remaining content and physical-device/controller testing are
still outstanding.

Captain Falcon's combat probe now runs 4,000 submissions and requires all four
special-move families using their original motion-state enums. Additional
normal PAD input requests Raptor Boost, Falcon Dive (with a later retry) and
Falcon Kick. The separate rematch variant still verifies its second active
match at 6,000 submissions without requiring combat observations there.

`captain-specials.log` exits zero after observing Falcon Punch at frame 2622,
Falcon Kick at 2722, Raptor Boost at 3001 and Falcon Dive at 3401. The final
framebuffer in `captain-specials.png` was visually inspected and shows the
Captain/Ness match still running on Onett. This proves move-state execution
through the native runtime; it does not prove every grounded/aerial variation,
Raptor Boost hit follow-up, Falcon Dive grab/throw, or competitive accuracy.
`captain-specials-build.log` and `git diff --check` pass. Only the test harness
changed; production app packages from the Captain data step remain current.

Donkey Kong now has native attributes and an assembled fighter-data owner
connected to the game loader. Its 0x74-byte special/carrying parameter block
contains four integer fields and 25 floats. The owner retains 337 motion
records, 14 demo-dynamics entries, five costume part tables and three 4/4/3
part-animation groups, with the original NULL item table. The original OnLoad
can update its writable carrying-animation parameters from motions 296–298.
`donkey-attributes-tests.log` passes scalar-type and all truncation checks plus
Captain/Ganon regression cases. The assembled-data test frees the source
buffers and decodes 289 nonempty animation trees/319 scripts, then checks HSD
cleanup.

Donkey's effect archive has seven records (effects 1222..1228), followed by
texture payload before its first referenced joint. The initial effect test
rejected that payload as an extra record. `effects.c` now uses the known retail
count for Donkey, alongside the existing Ness count. `donkey-final-tests.log`
passes the Donkey owner and all six registered effect archive tests (common,
Ness, Mario, Captain, Ganon, Donkey), including all seven Donkey effect models.

The new `--donkey` probe selects Donkey Kong through normal PAD menu input,
requires a live Donkey fighter and entry into Giant Punch charging, and runs
2,800 submissions. `donkey-combat.log` passes, observing charging at frame
2628 and exiting zero. The final framebuffer in `donkey-combat.png` was
inspected and shows Donkey/Ness combat on Onett. This verifies match entry,
combat and charging, not completed punch release, carrying interactions,
other specials, results/rematches or CPU behavior.

All three runtime builds pass in
`aurora-{integration,iphoneos,iphonesimulator}-donkey-data-build.log`, and all
three app packages pass in `{mac,iphone,simulator}-donkey-data-app-build.log`.
`git diff --check` passes. Remaining content and physical-device/controller
validation remain outstanding.

Donkey Kong's native combat probe now covers all four special-move families.
`--donkey` runs 4,000 submissions and requires a live Donkey, Giant Punch
charging and release, Headbutt, Spinning Kong and the Hand Slap loop. Inputs
are ordinary PAD samples; no fighter motion or position is forced.
`donkey-specials.log` exits zero, observing charging at frame 2628, release at
2722, Headbutt at 3002, Spinning Kong at 3082 and Hand Slap loop at 3665.
The final `donkey-specials.png` was inspected and shows the match still active
on Onett. This verifies move-state execution, not every charge strength,
aerial variation, carrying interaction or hit outcome.

The retail Donkey results archive is also registered in the results-motion
fixture. `donkey-results-tests.log` passes all eight registered fighter cases,
including original script presence/absence, preserved indices, invalid ranges,
source-buffer disposal and HSD cleanup. This is results-data validation;
Donkey's live results/rematch transition remains to be exercised.
`donkey-specials-build.log`, `donkey-results-tests-build.log` and
`git diff --check` pass. Only tests and test registration changed; the previous
Donkey production app packages remain current.

Donkey Kong now passes live results/rematch validation. `--donkey-rematch`
uses the existing shortened-timer transition fixture and normal PAD menu
input, then requires results, a return to CSS, a second active match and a
live Donkey fighter at submission 6000. Its combat-only variant continues to
require all four special-move families at submission 4000.

`donkey-rematch.log` exits zero after 6,000 submissions. Scene observations
record the first match at 1587, results at 2304, CSS at 4013, stage selection
at 4403 and second match at 4587. The final framebuffer in
`donkey-rematch.png` was visually inspected and shows Donkey and Ness at 0%
in that second Onett match. This exercises results rendering and cached
fighter/motion/effect data across scene teardown and re-entry. It does not
verify an unshortened CPU Donkey match or carrying interactions.
`donkey-rematch-build.log` and `git diff --check` pass. Only the test harness
changed, so the previous Donkey app packages remain current. Remaining
content, physical devices and real controller validation are still pending.

The private Mac VM was rechecked with `guest.sh windows`; it still exposes
loginwindow rather than an unlocked app session. No host GUI was inspected or
controlled. End-to-end Mac keyboard/window testing therefore remains pending.

Local review found and fixed a stuck-key case in the Mac app: a movement key
pressed before Command could have its release discarded by the event monitor's
shortcut filter. The monitor now forwards eligible key events with their
Command-modified status to `InputModel.key`. Command key-downs remain excluded
from gameplay and binding capture; Command key-ups release an existing held
key. Both event types still propagate to AppKit for normal shortcuts.

`command-key-release-tests.log` passes the windowless Apple integration suite,
including W down followed by Command-modified W up returning both raw PAD and
HSD stick values to zero, Command-modified key-down not moving the fighter,
and Command shortcuts leaving binding capture pending. Existing remapping,
persistence, synthetic controller-slot/disconnect and focus checks also pass.
This validates the model and event-routing implementation; real Mac keyboard
UI validation is still pending on the locked VM.

All three app packages pass in
`{mac,iphone,simulator}-command-key-release-app-build.log` and
`git diff --check` passes. The shared Swift input implementation changed;
the game runtime C/C++ binaries are unchanged.

Bowser/Giga Bowser conversion now includes the shared 0xa0 special-move
parameter block and a native owner for their single flame article slot.
`melee_koopa_attributes_decode` converts 36 floats and four integer fields
(offsets 0x4/0x20/0x2c/0x50). `koopa-attributes-tests.log` passes both retail
character archives, every scalar classification and all truncation boundaries,
plus Donkey/Mario regression cases.

`melee_koopa_items_decode` owns the flame article with six floating-point
attributes and one command/animation record. The retail flame is drawn through
effects: its article model and joint/material/shape animations are NULL, while
its command script is present. The fixture preserves and checks this layout
after overwriting/freeing the archive, and verifies Bowser's 28-frame versus
Giga Bowser's 32-frame lifetime, 20-frame hitbox lifetime, speed range and
angles. `koopa-items-tests.log` passes both cases with HSD pool cleanup;
`koopa-items-regressions.log` passes Mario/Dr. Mario and Ness item cases.
These checks validate owned data, not live flame behavior.

All three runtime builds pass in
`aurora-{integration,iphoneos,iphonesimulator}-koopa-prerequisites-build.log`,
and `git diff --check` passes. Full Koopa fighter-data assembly and its runtime
loader connection remain to be implemented before either Bowser variant can
enter native gameplay. App packages were not rebuilt for these currently
unreferenced prerequisites; their existing gameplay paths are unchanged.

Bowser/Giga Bowser now have assembled native fighter-data owners and runtime
loader connections (`koopa_data.c`, `ftdata.c`). Both retail archives contain
316 main motion records; their demo counts are 14 and 15 respectively, with
four Bowser costumes and one Giga Bowser costume. Both use part-animation
groups of 4/4/3 entries. The owner composes the common and special attributes,
collision, dynamics, models, motions, parts, sounds, wait data and flame item;
it releases every component on failure. Runtime descriptors are cached for
the process lifetime, as with the other converted fighters.

The assembled-owner tests overwrite/free both source archives, resolve all
nonempty motion trees, load/remove the skeleton and verify HSD pool cleanup.
Bowser has 269 nonempty animations and Giga Bowser 267; both preserve all 316
command scripts. The shared `EfKpData.dat` effect fixture passes all four
models. `koopa-data-regressions.log` passes all ten selected cases, including
Mario/Dr. Mario, Captain/Ganondorf, Donkey, both flame articles and both new
owners. These are owned-data checks; neither Bowser variant has yet been
verified in a live native match, and Giga Bowser results/selection behavior
remains unverified.

Mac, iPhone and Simulator runtimes and app packages build successfully in
`koopa-runtime-build.log`, `{iphoneos,iphonesimulator}-koopa-data-build.log`
and `{mac,iphone,simulator}-koopa-data-app-build.log`. iPhone packaging remains
unsigned and requires device provisioning. `git diff --check` passes.

Bowser now passes a live 4,000-frame native Onett match against Ness in the
windowless Metal/ASan probe (`koopa-specials-fixed.log` and `.png`). The
`--koopa` input path selects Bowser through the original CSS, navigates to
Onett and checks actual fighter motion states: Fire Breath at frame 2643,
Bowser Bomb at 2722, Koopa Klaw at 3002 and Whirling Fortress at 3082. The
viewed final framebuffer shows Bowser at 60% and Ness at 92%. This verifies
entry into all four special families, not every hit/grab/throw branch, flame
hit behavior, Giga Bowser gameplay or results/rematch transitions.

The initial run (`koopa-specials.log`) crashed in Whirling Fortress at frame
3081. `efSync_Spawn` drained the already-native pointer animation queue with
an obsolete four-byte stride. Its native path now indexes the typed pointer
array directly; the original target expression remains unchanged. The same
live input sequence passes after the fix.

Bowser's `unk1` side-special state overlay also contained two placeholder
pointer types for scalar words. They are now s32, preserving the shared
0/4/8/12 offsets with the named special-state view on ARM64. The character
attribute test asserts those offsets and verifies that clearing x0 or x8
preserves adjacent flags (`koopa-state-build.log`, `koopa-state-tests.log`).
Both retail Bowser parameter fixtures pass. All three runtime builds and
app packages pass in `koopa-state-runtime-build.log`,
`{iphoneos,iphonesimulator}-koopa-state-build.log` and
`{mac,iphone,simulator}-koopa-state-app-build.log`. `git diff --check` passes.

The Bowser results-motion fixture is now registered as
`hsd_results_motions_Kp`, using the user's `GmRstMKp.dat` and the original
`ftDemoResultMotionFileKoopa` symbol. `koopa-results-tests.log` passes its
source-disposal, script-presence and HSD cleanup checks. The startup probe
adds `--koopa-rematch`, reusing the original menu inputs and requiring results,
a return to character selection, two match entries and a live Bowser in the
second active match. As with other transition probes, the first match timer
is shortened to five seconds; this is not a full-duration match test.

`koopa-rematch.log` passes all 6,000 frames: first match at 1587, results at
2303, character selection at 4013, stage selection at 4403 and second match
at 4587. The viewed `koopa-rematch.png` shows Bowser and Ness at 0% in the
second active Onett match. This establishes the Bowser results/rematch path
in the windowless native runtime. Physical-device and Mac UI checks remain
pending. Only probe/test registration and documentation changed this turn;
the previously packaged runtime apps are unchanged. `git diff --check` passes.

The Bowser special-move probe now requires the original flame article and its
one-time effect callback, in addition to all four move families.
`koopa-flame-final.log` passes 4,000 frames: Fire Breath and its article at
2643, effect callback at 2644, Bomb at 2722, Klaw at 3002 and Fortress at
3082. A second side-special input at 3550 accommodates the first input being
consumed by another movement state, as occurred in `koopa-flame.log` (exit 6).
This verifies article creation and effect callback execution, not flame-hit
damage or a visual capture of the flame itself. The viewed final frame shows
a Bowser KO effect and Ness at 71%; the probe still sees the live Bowser
fighter object in its death state.

A separate retry (`koopa-flame-retry.log`, exit 134) navigated to Event Match
instead of versus and exposed a global-buffer-overflow in that menu's symbol
lookup. `mnevent.c` assumed its strings followed unrelated globals at fixed
GameCube addresses. Native symbol and allocation-diagnostic accesses now
reference `mnEvent_803EF7A0` directly with offsets inside the actual string
table. All eight offsets were checked against their decoded source strings.
This removes the observed out-of-bounds string references; Event Match as a
whole has not been validated and still requires native asset/mode coverage.
The scripted startup menu navigation can vary and remains a test limitation.

All three runtime builds and app packages pass in
`koopa-flame-final-build.log`, `{iphoneos,iphonesimulator}-event-strings-build.log`
and `{mac,iphone,simulator}-event-strings-app-build.log`. `git diff --check`
passes. The goal remains the full native port; roster, stage and mode coverage
and physical Apple-device input validation remain incomplete.

Luigi now has native special-attribute, fireball and assembled fighter-data
owners, connected to `ftdata.c` for `Ft_Kind_Luigi`. The 0x98 move parameter
block contains 36 floats and two integers at 0x88/0x94; the fixture checks
all scalar classifications, rejects every truncated length and preserves the
output on failure (`luigi-attributes-tests.log`, including Bowser regression).
The fireball has four floats (1.2, 50, 0.85, 0.9), one animation record and an
owned model/script. Its fixture overwrites/frees the source, animates frames
0..120 and checks HSD pool cleanup (`luigi-items-tests.log`).

The assembled owner has 312 main motions, 14 demo slots, four costumes and
part-animation groups of 4/4/3. After freeing both retail DAT/AJ inputs, its
fixture resolves 260 animations and 312 command scripts, loads/removes the
skeleton and checks pool cleanup. Both models in `EfLgData.dat` decode with
owned particle banks. All seven selected owner/item/effect regressions pass
in `luigi-data-regressions.log`. These checks establish data ownership and
loader assembly; Luigi has not yet been verified in a live native match.

Mac, iPhone and Simulator runtime and app builds pass in
`luigi-data-build.log`, `{iphoneos,iphonesimulator}-luigi-data-build.log` and
`{mac,iphone,simulator}-luigi-data-app-build.log`. `git diff --check` passes.
Physical-device signing and testing remain outstanding, as does the rest of
the full native port.

Luigi now passes a live 4,000-frame Onett combat test in the windowless native
runtime (`luigi-specials-retry.log`). `--luigi` sets only Luigi's unlock bit
in test-process memory before CSS; it does not alter app defaults or persist
a save. The original CSS and controller input select him. The test requires
an actual Luigi fighter, original fireball article, released Green Missile,
Super Jump Punch and Cyclone. Observed frames are fireball 2638, Cyclone 2722,
Missile 3019 and Jump Punch 3151. The viewed final framebuffer shows the Luigi
HUD at 60% and Ness at 93%; Luigi is near the left stage boundary.

The first run (`luigi-specials.log`) exited 6 because both scheduled up-special
inputs landed during damage/defensive states. The fixture now waits for the
original Wait state between frames 3150 and 3350 before publishing another
up+B input, releasing it six frames later. This changes test input only.
The passing run does not establish every hit branch, misfire outcome or
fireball reflection/absorption behavior.

`hsd_results_motions_Lg` also passes with the user's `GmRstMLg.dat`, preserving
ten result scripts after source disposal (`luigi-results-tests.log`). A live
Luigi results/rematch sequence remains unverified. Only the probe, test
registration and documentation changed; packaged apps retain the prior
Luigi runtime build. `git diff --check` passes.

Luigi's live results/rematch sequence now passes all 6,000 frames in
`luigi-rematch.log`: first match 1588, results 2306, character selection 4013,
stage selection 4404 and second match 4588. The viewed `luigi-rematch.png`
shows Luigi and Ness at 0% in a second active Onett match. `--luigi-rematch`
uses the existing shortened first-match timer and test-only in-memory Luigi
unlock. It requires the original results/selection transitions and a live
Luigi in the second match, while disabling the separate special-move inputs
and assertions. This does not validate a full-duration Luigi match or physical
Apple input. Only the test harness changed; packaged runtime apps are unchanged.

An initial Battlefield asset audit extracts the user's `GrNBa.dat` and passes
`--stage-playback` in `battlefield-models-audit.log`: seven native model records,
61 material descriptors, 17 light overrides, 14 model light identities and
one point mapping, including owned lifetime and HSD cleanup. Battlefield's
archive has the same main stage symbol families as Onett, but its stage
bridge, collision integration and live behavior remain unimplemented or
unverified. This is preparation for expanding stage coverage, not a claim
that Battlefield is playable. `git diff --check` passes.

Battlefield now has an owned native archive bridge (`battle_stage.c`) with
models, collision, ground parameters, item scripts, quake model, lighting and
particle banks. Its two `yakumono_param` entries are relocated color-script
pointers, despite the original stage code declaring them as ints. The bridge
uses native pointer fields and `melee_stage_colors_decode`, which reads a
four-byte pointer table with no metadata. Existing item/fighter color tables
retain their eight-byte pointer/metadata format and decoding behavior.

`hsd_battle_archive` overwrites/frees the disc buffer before checking all ten
public symbols, shared model/light identity and the two owned color scripts.
It verifies fade commands, 200-frame duration and opposite alpha transitions,
then checks HSD pool cleanup. Both Battlefield and Onett archive fixtures pass
in `battle-archive-regressions.log`. The original item-color interpreter
regression passes six scripts and 632 transitions in
`battle-color-regressions.log` (the initial invocation without its required
ItCo.dat argument was corrected before verification).

All three runtime builds pass in `battle-archive-build.log` and
`{iphoneos,iphonesimulator}-battle-archive-build.log`. `git diff --check` passes.
The Battlefield bridge is not yet connected to `grdatfiles.c` or particle
registration. The original Battlefield parameter declaration and stage
color-script dispatch still need native-width integration before live
selection can work. App packages were not rebuilt for this unused bridge;
Battlefield is not yet claimed playable.

Battlefield is now connected to the native stage loader and both particle
registration paths (banks 0x40 and 0x1E). Its background parameter fields use
native color-script pointers; `grMaterial_ApplyColorScript` accepts those
without truncation. Original-target declarations and dispatch remain intact.
The native stage selector exposes read-only Battlefield cursor guidance for
the windowless probe, which selects the original icon using controller input.
`--battlefield` enables stage unlocks only in test memory, writes no save and
requires a live versus scene on `Gr_Kind_Battle` with two fighter objects.

`battlefield-combat-retry.log` passes 2,800 frames. The viewed framebuffer shows
Fox and Ness on Battlefield's platforms with its background. Both are at 0%
at the endpoint, so this does not prove damage exchange or a full match.
Longer background-transition coverage and results/rematch remain unverified.
The initial `battlefield-combat.log` failed an Onett-owner assertion in the
second particle registration path; dispatch at that call was corrected before
the successful rerun. Battlefield and Onett archive regressions pass in
`battle-runtime-regressions.log`.

All three runtime builds and app packages pass in `battle-runtime-build.log`,
`{iphoneos,iphonesimulator}-battle-runtime-build.log` and
`{mac,iphone,simulator}-battle-runtime-app-build.log`. `git diff --check` passes.
The full port and physical-device verification remain unfinished.

Battlefield results/rematch now passes 6,000 frames in
`battlefield-rematch-retry.log`: first match 1559, results 2277, CSS 4013,
SSS 4403 and second match 4559. The native Battlefield archive is loaded
again for the second match. The viewed final framebuffer shows Fox and Ness
at 0% on Battlefield. `--battlefield-rematch` requires both the generic
results/selection/second-match progression and a live Battlefield scene with
two fighter objects. The first match uses the existing shortened timer.

The initial `battlefield-rematch.log` exited 6 on a Random Stage Select prize
notification. Enabling every stage in test memory triggered that original
unlock reward. The fixture now finds Battlefield's bit through the original
unlock-index mapping and enables only that bit. No persistent save or app
unlock rules changed. The successful retry reaches CSS without the prize.

The background transition timer is randomized to 2,400..3,599 game frames;
these short matches do not establish background fade/replacement completion.
That remains a separate longer-playback check, alongside damage interaction
and broader game coverage. Only test harness and documentation changed this
turn; packaged apps retain the prior Battlefield runtime build.
`git diff --check` passes.

Battlefield's natural background transition now passes in a 6,500-frame
native match (`battlefield-background.log`, `--battlefield-background`).
The probe leaves the randomized stage timer unchanged and observes the
original map-3 controller: transition animation at 4914, fade from background
1 to 4 at 5313, then old-model removal and new-model presence at 5509. The
viewed final framebuffer shows the changed background at 00:39.54 remaining.
The test requires all three lifecycle observations and an active Battlefield
match with two fighters at the endpoint; no stage timer/state is forced.

This exercises the native-width stage color-script dispatch and one natural
fade/replacement path. It does not establish all background combinations,
multiple transitions, damage interactions or full match completion. The
runtime implementation needed no further change. Only the probe and its
documentation changed; existing app packages remain current for the game
runtime. `battle-background-build.log` and `git diff --check` pass.

Final Destination audit (`GrNLa.dat`) finds ten stage models, two spline
entries and three shadow entries. Nine models pass the current playback
decoder; model 3 first hit an empty camera-animation rejection, then a
lighting rejection. Two of its light animation records contain nonempty
interest-world animation pointers (0x688/0x518), outside the current
empty-light-animation support. The full header remains unsupported because
its spline/shadow tables are nonempty (`final-models-after-camera.log`, exit 2).

`melee_camera_empty_animation_decode` now owns an explicitly empty three-field
camera animation. The stage model decoder preserves a non-NULL native
empty descriptor and still rejects actual camera animation tracks. The
`hsd_final_empty_camera` fixture isolates Final Destination's camera from its
unsupported lighting: it rejects each invalid field and a truncated record,
overwrites/frees source data, then loads the HSD camera and attaches/plays
its empty animation before cleanup. It does not claim the complete model
loads. Onett, Battlefield and this isolated camera test all pass in
`final-camera-tests.log`.

All three runtime builds pass in `final-camera-build.log` and
`{iphoneos,iphonesimulator}-final-camera-build.log`; `git diff --check` passes.
Final Destination still requires lighting animation, spline/shadow tables,
its four background scripts and runtime integration. No complete native
Final Destination loader or live-playability claim is made. Existing app
packages were not rebuilt for this currently unused capability.

Final Destination's two moving lights now decode through the native environment
owner. The previously noted nonempty light-animation fields are position
animations (field 2), not interest animations; both reference AObjs with
spline-path joints. The environment decoder owns WObj/AObj descriptors,
validated world channels 4..7, FObj streams and optional spline scene owners.
It preserves full-width path descriptor IDs for HSD's original AObj loader.
Linked light-animation records, direct light-property tracks and world-object
constraints remain explicitly rejected.

`hsd_final_light_playback` frees the disc input before creating HSD lights,
then animates 600 frames, checks finite changing positions on both path-driven
lights and verifies HSD cleanup. It and the camera/Onett/Battlefield regressions
pass in `final-light-tests.log`. The broader stage audit now decodes all ten
models, 92 material descriptors and 26 light overrides (24 model-light
identities), but still exits 2 because the full stage header requires spline
and shadow tables (`final-models-after-lights.log`). This is not a complete
Final Destination stage/playability test.

All three runtime builds pass in `final-lights-build.log` and
`{iphoneos,iphonesimulator}-final-lights-build.log`; `git diff --check` passes.
App packages remain at the prior supported-stage build pending complete Final
Destination integration. Background scripts, stage header and live runtime
coverage still remain for this stage.

Final Destination's complete native model header now decodes. The stage-model
owner supports bounded spline and shadow tables, owns standalone spline
resources through the existing validated spline decoder, and resolves shadow
records to the exact light-animation descriptors already owned by model
environments. This identity matters because Ground's shadow lookup compares
animation pointers. Only the original high shadow-flag bit is accepted.
Partial table allocations are released on failure; zero-count tables remain
NULL for existing stages.

`hsd_final_stage_playback` now passes all ten models and the full header after
source disposal. It samples 101 points on each of the two spline paths,
checks finite coordinates, verifies all three shadow flags and shared
animation identities, and checks HSD pool cleanup. It also covers 92 material
descriptors and 26 light overrides. All five selected Final Destination,
Onett and Battlefield fixtures pass in `final-tables-tests.log`.

Mac, iPhone and Simulator runtime builds pass in `final-tables-build.log` and
`{iphoneos,iphonesimulator}-final-tables-build.log`; `git diff --check` passes.
Final Destination still needs its archive bridge/background scripts connected
to the game runtime and live gameplay verification. App packages were not
rebuilt for these currently unused stage capabilities. The full native port
remains unfinished.


### Final Destination runtime bridge and initial live match

Final Destination now uses the shared Battlefield archive owner, with four
owned background color scripts and all ten retail public symbols. The stage
loader recognizes `GrNLa.dat`; both original particle registration paths use
the native bank owner. `grLast` uses typed native color-script pointers.

The first live checks exposed three porting issues: light-animation copies
lost shadow-table identity (both model environments and global `map_plit`),
`StageCallbacks` numeric masks were interpreted with PowerPC byte bitfields,
and `grAnime_801C7C1C` indexed separately allocated native animation descriptors
as contiguous arrays. Shared animation offsets now resolve to canonical model
objects, native callback bitfields preserve the original numeric masks, and
native joint/material/shape animation selection walks depth-first tree order.
The retail Final Destination animation arrays were independently checked to
have that order for every model and variant. Original-target code paths remain.

`--final-destination` drives the original menus, unlocks only this stage in
fixture memory (no save write), and requires the actual Final Destination stage
ID, two live fighters, and an active VS match at frame 2800. The final run
`build/final-live-index.log` exits 0, with no sanitizer error or assertion.
`build/final-live-index.png` shows Fox and Ness on the platform at 01:40.91;
both are at 0%. Stage textures have visible magenta/noisy artifacts, so visual
fidelity is unresolved. This is initial live stage coverage, not proof of damage
exchange, every background transition, results/rematch, or a complete match.
Earlier `final-live*.log` failures record the issues above, not passing runs.

Six selected archive/playback regressions pass in
`build/final-stage-flags-tests.log`. The archive test now checks all model and
global light-animation references against the shadow table, source-buffer
independence, the four fade scripts, and native stage flag masks. Mac, iPhone,
and Simulator packages are refreshed in `*-final-index-app-build.log`.
Physical iPhone installation still needs signing/provisioning, and real device,
controller, and Mac GUI verification remain outstanding. The full port remains
unfinished, including unsupported fighters/stages/modes and rendering issues.


### Final Destination results/rematch and texture triage

`--final-destination-rematch` extends the menu-driven stage fixture to 6000
frames. It requires the existing results -> CSS -> second VS lifecycle checks
and an active Final Destination match with at least two fighters at completion.
`build/final-rematch.log` exits 0: first VS at 1566, results at 2284, CSS at
4014, stage selection at 4403, and second VS at 4564. The stage archive loads
twice. `build/final-rematch.png` was inspected and shows Fox/Ness, both 0%, in
the second match at 01:38.11. No sanitizer error or assertion occurred. The
fixture shortens the first match timer; this does not prove a normal-length
match, damage exchange, or every background transition.

Texture triage decoded all 68 base model texture references in GrNLa into
`build/final-texture-atlas.png` (offsets/formats in `final-textures.json`). They
use I4, I8, RGBA8 and CMPR. The grainy platform pattern exists in the retail
CMPR texture, so its appearance alone is not evidence of corruption. Reference
screenshots also show pink edges (for example
https://dignitas.gg/articles/hunting-the-prince-a-guide-to-dealing-with-marths-defensive-movement-as-fox).
The earlier blanket description of the stage textures as corrupted was too
strong. Full visual fidelity remains unverified; square background particle
artifacts are still visible in the native captures and need investigation.

Only the windowless fixture changed this turn; production app packages remain
those from the preceding `*-final-index-app-build.log` builds. Probe compilation
and `git diff --check` pass. The full native port remains unfinished.


### Native particle shading reads the widened particle structure

`psSetupTev` previously accepted a `u32*` cast of `HSD_Particle` and read or
modified word 1 as the particle kind. On ARM64 that word belongs to the widened
`next` pointer, not `kind`. Consequently TEV texture/lighting selection depended
on unrelated pointer bits and particles could appear as flat squares; the code
could also alter linked-list pointer bits. The native signature now takes
`HSD_Particle*` and accesses the actual kind field. The original-target signature
and word layout remain unchanged.

`hsd_particle_tev` exercises the four modes that clear the secondary-color flag,
checking the updated kind and unchanged linked-list pointer. It passes in
`build/particle-tev-tests.log`. `build/final-particle-tev.log` exits 0 after 2800
native frames on Final Destination without an assertion or sanitizer error.
The inspected `build/final-particle-tev.png` shows properly textured star sprites
where prior captures showed flat background squares. This verifies this defect,
not all particle modes or complete visual fidelity. Both fighters remain at 0%.

Mac, iPhone and Simulator apps were rebuilt successfully in
`{mac,iphone,simulator}-particle-tev-app-build.log`; iOS runtime build logs are
`{iphoneos,iphonesimulator}-particle-tev-build.log`. `git diff --check` passes.
The full native port, remaining roster/stages/modes, normal-length Final
Destination match coverage, and physical-device/controller testing remain
unfinished. Device installation still requires signing and provisioning.


### Final Destination natural background transitions

`--final-destination-background` runs the original menu-driven match for 6500
frames with the normal stage timer. Its observer requires initial state 1,
transition state 2, following state 3, and an active Final Destination match
with two fighters at completion. It does not force stage state or animation
timing. `build/final-background-build.log` and `git diff --check` pass.

`build/final-background.log` exits 0 without an assertion or sanitizer error.
Observed background states/frames: 1/1565, 2/3439, 3/3633, 4/3634, 5/4135,
6/5928, 7/6124. The inspected `build/final-background.png` shows the changed
mechanical backdrop at 00:39.29, Fox at 25% damage and Ness at 0%, with the match
still active. This adds evidence of real damage and several natural background
transitions; states 8 through 17, fade scripts in the later states, the complete
cycle, and a normal full-match finish remain unverified.

Only the windowless test fixture changed. Production packages remain the
preceding `*-particle-tev-app-build.log` builds. The full native port and
physical-device/controller verification remain unfinished.


### Final Destination full two-minute match finish

`--final-destination-finish` runs the original menu-driven Final Destination
match through frame 12000. It retains the normal two-minute timer, requires
an active Final Destination match at frame 2800, and uses the full-match
observer to require results plus at least three distinct common item types.
It observes background state changes without forcing animation timing.

`build/final-finish.log` exits 0 with no assertion or sanitizer error. Background
states 1 through 9 were observed (state 9 at frame 8628). Common item kinds
34, 29, and 5 appeared at frames 3471, 5334, and 7208. Results were reached
at frame 8972, with no sudden-death transition in this run. The inspected
`build/final-finish.png` shows Ness as winner and Fox second, with Fox's single
self-destruct reflected in the results totals. The results screen remains
rendered through frame 12000. This verifies a full normal-length match and
results, not every later background state or all gameplay interactions.

`build/final-finish-build.log` and `git diff --check` pass. This turn changes
only the test fixture; production packages remain the preceding particle-TEV
builds. The full native port, unsupported roster/stages/modes, complete Final
Destination background cycle, and physical-device/controller testing remain
unfinished.


### Marth/Roy special-move parameter decoding

Added `melee_mars_attributes_decode` for the shared 0x98-byte MarsAttributes
schema used by retail `ftDataMars` and `ftDataEmblem`. The decoder converts 30
float words and five integer words, preserves the 12 sword-setting/padding
bytes at 0x80..0x8b, rejects nonfinite float parameters and truncated blocks,
and publishes output only after validation. Compile-time checks protect the
structure size and mixed-field offsets.

`build/mars-attributes-tests.log` passes against PlMs.dat and PlFe.dat extracted
from the supplied disc, plus Luigi and Bowser regressions. Tests check every
field against serialized values, distinguish float/integer/byte behavior on
NaN-shaped input, cover every truncation boundary, preserve prior output on
failure, and confirm independence from source-buffer contents.

Mac, iPhone and Simulator runtime targets build successfully in
`mars-attributes-runtime-build.log`, `iphoneos-mars-attributes-build.log`, and
`iphonesimulator-mars-attributes-build.log`; `git diff --check` passes. These
parameters are not yet connected to complete native Marth/Roy character owners,
so neither fighter is newly playable from this change. App packages were not
rebuilt for this currently unused decoder. Full roster/stage/mode support and
physical-device/controller verification remain unfinished.


### Assembled Marth/Roy normal-match character data

Added `melee_mars_data_decode` and its owned lifetime API for `ftDataMars` and
`ftDataEmblem`. Both use 327 motion records, 14 demo dynamics entries, five
costumes, 4/4/3 part-animation groups, and empty item tables. The owner connects
common and special parameters, collision, dynamics, auxiliary tables, models,
parts, sounds, wait data and AJ-backed normal motions. Demo motion records
remain separate, as with the existing character owners.

`hsd_mars_data_Ms` and `hsd_mars_data_Fe` pass after both DAT and AJ source
buffers are overwritten and freed. The fixtures assemble all required ftData
fields, decode every available animation, load the auxiliary joint tree and
check HSD pool cleanup. Marth has 271 decoded animations and 327 scripts;
Roy has 269 animations and 327 scripts. Five selected tests pass in
`build/mars-data-regressions.log`, including Captain/Ganon and Donkey regressions.

Mac, iPhone and Simulator runtime builds pass in `mars-data-runtime-build.log`,
`iphoneos-mars-data-build.log`, and `iphonesimulator-mars-data-build.log`;
`git diff --check` passes. Fighter selection is not yet hooked to these owners,
and effects/results/live-special-move verification remain outstanding. No app
packages were rebuilt for these currently unconnected loaders. The full port
and physical-device/controller verification remain unfinished.


### Marth/Roy runtime connection, effects and results assets

`ftData` now connects Ft_Kind_Mars and Ft_Kind_Emblem to their native character
owners, using the existing disc/cache loader and process-lifetime descriptor
cache across matches. This preserves the original character roster and unlock
behavior; no production unlock state is changed.

Both retail effect archives decode with the shared effects loader: EfMsData
and EfFeData each contain two supported model records. The effect fixtures
validate owned particle banks, model decoding and HSD cleanup. Results-motion
fixtures pass for GmRstMMs/ftDemoResultMotionFileMars and
GmRstMFe/ftDemoResultMotionFileEmblem. All six selected data/effects/results
checks pass in `build/mars-runtime-hook-tests.log`.

Mac runtime/probe compilation passes in `mars-runtime-hook-build.log`. All
three app builds pass in `{mac,iphone,simulator}-mars-hook-app-build.log`, and
`git diff --check` passes. These are asset and build checks: live Marth/Roy
selection, sword trails, specials, counters, damage, results and rematches
remain unverified. iPhone signing/provisioning and physical-device/controller
verification remain outstanding, along with the broader unfinished port.


### Marth initial native match and geometry capacity

`--marth` uses test-only in-memory unlocking and the original CSS/controller
path to select Marth, then enters Onett. At frame 2800 it requires player one's
actual Ft_Kind_Mars identity and an active VS match. It does not yet assert all
special moves or counters.

The initial `build/marth-live.log` reached Marth gameplay but exhausted Aurora's
16 MiB storage buffer at frame 2295 (requested 16,781,646 bytes). Increased the
capacity to 32 MiB in both the checked-in Aurora patch and dependency source.
This addresses the observed scene capacity; larger/four-player scenes remain
unverified and the larger buffer has a corresponding memory cost.

`build/marth-live-retry.log` exits 0 after 2800 frames with no assertion or
sanitizer error. The inspected `build/marth-live-retry.png` shows Marth on the
Onett rooftop at 30% and Ness at 34%, at 01:41.59. This adds live selection,
rendering and damage evidence. Marth specials, sword-trail fidelity, counters,
results/rematch and Roy gameplay remain unverified.

All three packages build successfully in
`{mac,iphone,simulator}-marth-buffer-app-build.log`; `git diff --check` passes.
The full port and physical-device/controller verification remain unfinished.


### Roy initial native match

`--roy` unlocks only Roy in fixture memory, selects him through the original
CSS/controller path, and enters Onett. At frame 2800 it requires player one's
actual Ft_Kind_Emblem identity and an active VS match. No save file or production
unlock behavior changes.

`build/roy-live-build.log` passes and `build/roy-live.log` exits 0 after 2800
frames without an assertion or sanitizer error. The inspected
`build/roy-live.png` shows Roy at 30% and Ness at 35%, at 01:41.74. This adds
live selection, rendering and basic damage evidence. Specials, counter hits,
sword-trail fidelity, results/rematch, and full-match coverage are not proven.

`git diff --check` passes. Only the test fixture changed, so app packages remain
the preceding `*-marth-buffer-app-build.log` builds. The full port and physical
iPhone/controller verification remain unfinished.


### Marth results and rematch

`--marth-rematch` extends the live Marth fixture to 6000 frames, requiring the
results -> CSS -> second VS lifecycle and actual player-one Marth identity in
the final active match. The first timer is shortened by the existing rematch
fixture; this is lifecycle coverage, not a full normal-length match.

`build/marth-rematch.log` exits 0 without an assertion or sanitizer error:
first VS at 1587, results at 2300, CSS at 4014, stage selection at 4403, second
VS at 4587. The inspected `build/marth-rematch.png` shows Marth and Ness on
Onett at 01:38.49 with both damage totals reset to 0%. Probe compilation in
`build/marth-rematch-build.log` and `git diff --check` pass.

Only the test fixture changed; app packages remain the preceding Marth-buffer
builds. Specials, counters, sword-trail fidelity, normal-length Marth matches,
Roy results/rematch, and physical-device/controller verification remain
outstanding along with the broader unfinished port.


### Roy results and rematch

`--roy-rematch` exercises the original results -> CSS -> second VS lifecycle
for 6000 frames and requires player-one Roy in the final active match. The
first match timer is shortened by the rematch fixture; this is not a normal
full-length match test.

`build/roy-rematch.log` exits 0 without an assertion or sanitizer error: first
VS at 1587, results at 2304, CSS at 4014, stage selection at 4403, second VS
at 4587. The inspected `build/roy-rematch.png` shows Roy and Ness on Onett at
01:38.49 with both damage totals reset to 0%. Probe compilation in
`build/roy-rematch-build.log` and `git diff --check` pass.

Only the test fixture changed; production packages remain the preceding
Marth-buffer builds. Specials, counter hits, sword-trail fidelity, full-length
Marth/Roy matches and physical-device/controller testing remain outstanding
along with the broader unfinished port.


### Marth special-move state coverage

`--marth-specials` runs 5000 frames and observes original motion states for
Shield Breaker release, Dancing Blade first strike, Counter stance and Dolphin
Slash. After frame 3000 it issues PAD special inputs only while Marth is in
Wait, retrying missing moves as needed; it does not force motion states.

`build/marth-specials.log` exits 0 without an assertion or sanitizer error.
Observed states: Shield Breaker release 343 at frame 2633; Counter stance 369
at 2722; Dancing Blade first strike 349 at 3001; Dolphin Slash 367 at 3092.
The final inspected `build/marth-specials.png` shows an active Onett match,
Marth respawning at 0% and Ness at 70%, at 01:04.93. This does not prove a
successful counter hit, Dancing Blade follow-ups, all charge/air variants,
move-specific damage attribution, or sword-trail visual fidelity.

Probe compilation in `build/marth-specials-build.log` and `git diff --check`
pass. Only the fixture changed; production apps remain the preceding
Marth-buffer builds. Roy specials and the broader native port, including
physical-device/controller verification, remain unfinished.


### Roy special-move state coverage

`--roy-specials` runs 5000 native frames, requiring Flare Blade release,
Double-Edge Dance first strike, Counter stance and Blazer motion states.
The fixture uses PAD inputs, waits for Wait before retrying missing moves,
and never forces motion states. Roy uses the original shared Mars state IDs.

`build/roy-specials.log` exits 0 without an assertion or sanitizer error.
Observed states: Flare Blade release 343 at frame 2632; Counter stance 369 at
2722; Double-Edge Dance first strike 349 at 3002; Blazer 367 at 3093. The
inspected `build/roy-specials.png` shows Roy at 90% and Ness at 95% in an active
Onett match at 01:04.89. This does not prove successful counter hits, combo
follow-ups, full-charge/air variants, move-specific damage attribution or
complete effects fidelity.

Probe compilation in `build/roy-specials-build.log` and `git diff --check`
pass. Only the fixture changed; production apps remain the Marth-buffer builds.
The broader port and physical-device/controller verification remain unfinished.


### Pikachu/Pichu parameters and fresh-runtime packaging

Added the shared 0xf8-byte Pikachu/Pichu attribute decoder, including the six
collision-box floats at 0xe0. It converts 53 float words and nine integer/item
ID words, rejects nonfinite floats and truncated input, and publishes output
only on success. Retail PlPk/PlPc tests check every field, every truncation
boundary, integer handling, output preservation and source-buffer independence;
Marth/Roy regressions also pass in `build/pikachu-attributes-tests.log`.
The parameters are not yet connected to complete Pikachu/Pichu owners, so
neither character becomes playable from this change alone.

The iOS build audit found that the Aurora patch's resources.hpp index hash
had not been refreshed after the 32 MiB buffer edit. The only difference from
the dependency's actual patch was that hash; it is now corrected and
prepare_aurora.py validates the checkout. All three runtime targets now build
successfully in `pikachu-attributes-runtime-build.log` and
`{iphoneos,iphonesimulator}-pikachu-attributes-build.log`.

Earlier packaging scripts copied the existing runtime without rebuilding it.
Consequently prior packaging success alone did not prove the newest runtime
changes were included, notably the recent geometry-capacity change on iOS.
Both scripts now build the matching melee_game_runtime target before default
packaging and stop on build failure. An explicit --runtime continues to package
the caller-supplied library. All three complete packaging paths pass in
`{mac,iphone,simulator}-fresh-runtime-app-build.log`, after the successful
runtime rebuilds. `git diff --check` passes. These refreshes do not constitute
new iOS gameplay/device verification; the full port remains unfinished.


### Pikachu/Pichu projectile data owners

Added `melee_pikachu_items_decode` for the three article slots shared by Pikachu
and Pichu: Thunder, ground Thunder Jolt and air Thunder Jolt. They contain
1/2/1 animation records respectively. The special-attribute decoder now supports
both fighters' Thunder (three floats), ground jolt (four floats), and unused
air-jolt scalar word. Compile-time checks protect the known attribute layouts.

Five tests pass in `build/pikachu-items-tests.log`, including both retail
character archives plus Luigi and Bowser/Giga Bowser regressions. The new
fixtures overwrite and free the source DAT before checking every article and
animation script, verify the differing Thunder parameters (60 versus 40, with
shared 40/38 values), then free owners and check HSD pool cleanup.

Mac, iPhone and Simulator runtime targets build successfully in
`pikachu-items-runtime-build.log`, `iphoneos-pikachu-items-build.log`, and
`iphonesimulator-pikachu-items-build.log`; `git diff --check` passes. These
projectile owners still need integration into complete Pikachu/Pichu character
loaders and live gameplay tests. App packages were not refreshed for this
currently unused support. The full port and physical-device/controller
verification remain unfinished.


### Pikachu/Pichu assembled owners and runtime hookup (2026-09-10)

Added the normal-match Pikachu/Pichu owners and connected both fighter kinds to
process-lifetime native data caches. Each owns 320 motion records, 14 demo
animation pairs, four costumes, the shared attribute schema, collision,
dynamics, models, parts, sounds, wait records and three projectile articles.
The two part-animation groups have four and three entries. The next relocated
word after the second group belongs to another descriptor, so counting all
adjacent relocations as animation entries was incorrect.

Pichu's retail action script at 0x56e0 contains opcode 7 with an unrelocated
null target. The original Command_07 assigns that null to the cursor and the
fighter interpreter stops. The native importer now preserves null call/jump
targets instead of rejecting them; a relocated offset zero still points to the
first command. Sanitized action tests cover null calls, null jumps, relocated
zero loops and malformed nonzero unrelocated targets. Temporary line-number
diagnostics were removed.

`pikachu-data-tests.log`: seven assembled-owner regression tests passed for
Pikachu, Pichu, Marth, Roy, Luigi, Bowser and Giga Bowser.
`pikachu-hook-tests.log`: seven Pikachu/Pichu article, owner, effect and results
motion tests passed. Pikachu has 265 available animations and Pichu 267; both
have 320 scripts. Both source DAT/AJ buffers are overwritten and freed before
animation traversal. The shared effect archive decodes six models, and each
results archive decodes ten records/scripts.

`pikachu-hook-build.log`, `iphoneos-pikachu-hook-build.log` and
`iphonesimulator-pikachu-hook-build.log`: actual runtime builds passed.
`mac-pikachu-hook-app-build.log`, `iphone-pikachu-hook-app-build.log` and
`simulator-pikachu-hook-app-build.log`: all three updated app packages passed.
The iPhone package still requires device provisioning. Pikachu/Pichu live
matches, special moves, electrical effects and rematches have not yet been
verified. This milestone establishes owned asset conversion and build
integration, not full gameplay correctness for these fighters.


### Pikachu live match and Thunder pointer correction (2026-09-10)

Added `--pikachu` and `--pichu` startup probes with final checks for the actual
player-one fighter kind in an active VS match. Pichu's unlock is applied only to
the probe's in-memory save state. Pikachu occupies the top-row second visible
slot in the initial locked roster; the first bottom-row attempt selected an
empty slot. Confirm inputs now wait until frame 1140 to allow selection to
settle. Failed selection attempts are retained in `pikachu-live.log` and
`pikachu-thunder.log`; neither is gameplay evidence.

`pikachu-live-retry.log` reached Pikachu gameplay and reproduced a sanitizer
crash in `it_802B1FE8` during Thunder. Down-special code wrote through the
SpecialHi motion overlay, setting a 32-bit state at offset four within a
64-bit Thunder item pointer. The down-special code now consistently uses
`speciallw` for both the pointer and state, including initialization and item
callbacks. Both overlays retain equivalent offsets on the 32-bit console.

`pikachu-thunder-retry.log` passed 2800 submissions. It verifies Pikachu kind 12
in an active Onett match, including Thunder states 359 at frame 2722, 360 at
2739, 362 at 2757 and Wait at 2793. The final image was inspected: Pikachu 30%,
Ness 45%, timer 01:41.73. This is limited scripted combat, not full special-move
coverage or a complete match/rematch test.

Actual runtime builds passed in `aurora-integration-thunder-build.log`,
`aurora-iphoneos-thunder-build.log` and `aurora-iphonesimulator-thunder-build.log`.
All updated app packages passed in `mac-thunder-app-build.log`,
`iphone-thunder-app-build.log` and `simulator-thunder-app-build.log`.

`pichu-live.log` also passed 2800 submissions, confirming player-one kind 23
in an active Onett match. Thunder progressed through loop 360, recovery 362
and Wait 14 at frame 2792. The final image was inspected: Pichu 31%, Ness 32%,
timer 01:41.69. Neither live probe verifies all four specials, all electrical
article variants, results/rematches, physical controllers, or actual iOS
execution for these fighters. Those checks remain outstanding.


### Pikachu/Pichu special-state coverage (2026-09-10)

Added `--pikachu-specials`, `--pichu-specials`, `--pikachu-rematch` and
`--pichu-rematch` to the silent native startup probe. Special runs require
neutral special, Skull Bash travel, Thunder recovery and Quick Attack travel
states, plus the correct fighter in an active match at frame 5000. Missing
moves are attempted through the controller backend while the fighter is idle.

`pikachu-specials-retry.log` passed: states 341 at 2621, 362 at 2758, 350 at
3036 and 357 at 3166. Final image inspected: Pikachu 60%, Ness 122%, 01:05.00.
`pichu-specials.log` passed: states 341 at 2622, 362 at 2759, 350 at 3035 and
357 at 3391. Final image inspected: Pichu 63%, Ness 122%, 01:04.94. These tests
establish limited move execution, not all charge levels, air variants,
second-zip directions, hit interactions, or effect fidelity.

The first special probe entered Events because of frame-timed menu input and
crashed in `gm_801BEBF8` before reaching VS. The event preview read a pointer
at hard-coded console offset 0x14. Native builds now read the typed
`entry->player_init[0]->c_kind`; the original target retains its old access.
The crash is recorded in `pikachu-specials.log`. A direct live retest of the
Events preview remains outstanding. The existing sanitized retail event-data
test passes all 51 records (`event-preview-data-test.log`), but does not execute
that preview function.

Actual runtime builds passed in `aurora-integration-event-preview-build.log`,
`aurora-iphoneos-event-preview-build.log`, and
`aurora-iphonesimulator-event-preview-build.log`. Updated app packages passed
in `mac-event-preview-app-build.log`, `iphone-event-preview-app-build.log`, and
`simulator-event-preview-app-build.log`.

`pikachu-rematch.log` passed 6000 submissions: first match at 1588, results at
2305, character selection at 4013, stage selection at 4404 and second match at
4588. The final image was inspected: Pikachu/Ness both 0%, timer 01:38.51.
`pichu-rematch.log` also passed 6000 submissions: first match at 1587, results
at 2304, character selection at 4013 and stage selection at 4403. Both probes
require the expected player-one fighter in the second active match. They use
a shortened first-match timer and do not establish full two-minute matches or
all results poses, nor do they exercise the actual iOS shell or physical input.


### All 51 event previews live (2026-09-10)

Added `--event-preview`, a silent controller-driven regression that uses the
current menu state to enter 1-P Mode / Event Match and scroll the list. A native
read-only selected-row observer verifies each preview index. It does not
launch an event match. The first run (`event-preview-live.log`) showed all ten
fresh-save previews without the previous pointer crash but failed its twelve
preview expectation because only ten were unlocked. Its final image was
inspected at Event 10.

The full probe enables developer event visibility in memory at frame 720,
only inside the startup test process. `event-preview-all.log` passed all 51
selected indices and 4500 submissions. Index 48 was observed at 3721, 49 at
3782 and 50 at 3842. The final image was inspected at Event 51. This directly
retests the prior `gm_801BEBF8` pointer crash and exercises scrolling/preview
updates across the full list. It does not establish that all event matches
are playable; their roster, stage and scenario support remains incomplete.

Runtime builds with the read-only observer passed for macOS, iPhone and
Simulator (`aurora-*-event-observer-build.log`). The previously packaged apps
already contain the event-preview pointer correction; packages were not
rebuilt for this test-only observer.


### Jigglypuff attributes (2026-09-10)

Added `melee_purin_attributes_decode` for PlPr.dat's 256-byte special-attribute
block. The first 0x34 bytes also serve as `Fighter_x2D0_t` multi-jump settings:
x0 is an integer and x14..x24 are five floating-point jump impulses, despite
legacy labels in the character-specific declaration. The decoder validates
48 finite float words, preserves eight integer words and two unused scalar
words, and copies six unused/padding words as bytes. Native xE8/xEC are now
32-bit scalars rather than host pointers: neither has a relocation in the
retail archive and neither is dereferenced by Jigglypuff code. This preserves
xF0 at offset 0xF0 and the total 0x100 size on ARM64; the console declaration
is unchanged.

`purin-attributes-tests.log` passes Jigglypuff plus Pikachu/Pichu regressions.
It checks every word, finite-float rejection versus integer/raw preservation,
every truncated block length, unchanged outputs on failure and source-buffer
disposal. Actual runtime builds pass for macOS, iPhone and Simulator in
`aurora-integration-purin-attributes-build.log`,
`aurora-iphoneos-purin-attributes-build.log`, and
`aurora-iphonesimulator-purin-attributes-build.log`.

Jigglypuff's complete data owner, costume hat ownership, runtime loader hook
and live gameplay remain outstanding. This attribute decoder is not yet
used by a Jigglypuff runtime owner. App packages were not rebuilt for it.


### Jigglypuff assembled data owner and hat visibility (2026-09-10)

Added `melee_purin_data_decode`: 327 motion records, 14 demo dynamics pairs,
five costumes, and two part-animation groups with 2/3 entries. It assembles
the attribute block, auxiliary descriptors, collision, dynamics, models,
animation archives, visibility, sounds and wait records. The item-table entry
is a hat-visibility descriptor, not a projectile article. Slot zero is null;
slot one owns a null joint placeholder followed by the costume visibility
descriptor. Costume archives supply the actual hat joints separately.

The shared parts importer now supports standalone visibility descriptors;
its existing full fighter-parts path retains texture selectors and bone IDs.
`purin-data-tests.log` passes six assembled-owner tests (Jigglypuff, Luigi,
Marth, Roy, Pikachu, Pichu). `purin-hat-tests.log` additionally traverses hat
visibility choices for all five costumes after overwriting/freeing both
source archives. Jigglypuff has 272 available animation trees and 327 scripts;
all load successfully and the tested HSD pools return to zero after cleanup.

Actual runtime builds passed for all three Apple targets in
`aurora-integration-purin-data-build.log`, `aurora-iphoneos-purin-data-build.log`
and `aurora-iphonesimulator-purin-data-build.log`. The new owner is not yet
connected to fighter selection. Live hat joint loading, Jigglypuff gameplay,
effects/results integration and rematches remain outstanding; app packages
were not rebuilt for this unused owner.


### Jigglypuff runtime hookup and alternate costume (2026-09-10)

Connected Jigglypuff's assembled owner to fighter loading through a
process-lifetime cache. `purin-hook-tests.log` passes its data, effect archive
(one model) and results archive (ten records/scripts). `--jigglypuff` uses a
probe-only in-memory unlock and checks the actual player-one kind in active
VS. `purin-live.log` passed 2800 submissions; final image inspected at Onett,
Jigglypuff 30%, Ness 33%, timer 01:41.84.

`--jigglypuff-hat` selects costume 1 with the original X-button input and
requires a live hat joint plus nonempty drawable list. The first run
(`purin-hat-live.log`) reproduced a null HSD_Archive lookup: native costumes
retain owned scene descriptors, not the original archive handle. Added
`ftData_NativeCostumeJoint`, caching additional named joint roots by fighter,
costume and symbol. It decodes the requested root into an owned scene,
releases validation instances and retains descriptors for the process
lifetime. Jigglypuff now uses this path for costume hats in native builds;
the original archive lookup remains for the console target.

`purin-hat-retry.log` passed 2800 submissions. Its final image was inspected:
the red bow renders on Jigglypuff during combat, Jigglypuff 30%, Ness 41%,
timer 01:41.56. This covers costume 1, not all alternate hats, and does not
verify hat animation/visibility in every move or across rematches.

Actual runtime builds pass for all Apple targets in
`aurora-integration-purin-hat-build.log`, `aurora-iphoneos-purin-hat-build.log`
and `aurora-iphonesimulator-purin-hat-build.log`. Updated packages pass in
`mac-purin-app-build.log`, `iphone-purin-app-build.log` and
`simulator-purin-app-build.log`. Full Jigglypuff special-move coverage,
multi-jumps, results/rematches and actual iOS gameplay remain outstanding.


### Jigglypuff specials and five aerial jumps (2026-09-10)

Added `--jigglypuff-specials` and `--jigglypuff-rematch` to the silent native
probe. The special test requires Rollout release, Pound, Sing and Rest motion
states, then all five aerial-jump states. Inputs are sent through the
controller backend; no fighter motion state is assigned by the probe.

`purin-specials.log` passed 7000 submissions. It observed Rollout release state
350 at frame 2336, Pound 363 at 3001, Sing 367 at 3152 and Rest 371 at 3514.
Aerial states 341..345 occurred at frames 3607, 3638, 3669, 3700 and 3731,
with the original jumps-used counter increasing from 2 through 6. The final
image was inspected at Onett: Jigglypuff 0%, Ness 46%, timer 00:31.61. This
establishes limited move/jump execution and continuing gameplay, not all
charge levels, hit effects, aerial special variants or a complete match.
Only the test executable changed in this milestone; production apps were not
rebuilt.

`purin-rematch.log` passed 6000 submissions. The first match began at frame
1588, results at 2305, character selection at 4013 and stage selection at
4403. The second active match passed the Jigglypuff identity check. Its final
image was inspected: Jigglypuff/Ness both 0%, timer 01:38.51. This uses the
probe's shortened first-match timer and default costume; all alternate hats
and full-length matches remain unverified.


### Yoshi shared attribute views (2026-09-10)

Added `melee_yoshi_attributes_decode` for the 0x138-byte PlYs.dat special
attribute block: 68 finite float words, seven integer words and twelve
preserved tail bytes. The EC..110 range is described as padding by the full
struct but contains egg-throw floats used by the alternate `ftYs_DatAttrs`
view, so it must be endian-converted rather than copied as opaque bytes.
Native alternate-view x1C/x20 are now floats rather than 64-bit pointers,
matching both the retail unrelocated scalar words and the full declaration.
Static assertions enforce both struct sizes, the egg-angle offset 0xF8 and
star-spawn offset 0x118. The console declaration is unchanged.

`yoshi-attributes-tests.log` passes retail Yoshi, Jigglypuff and Pikachu
attribute tests, covering each word, invalid floats versus integer/raw
preservation, every truncation length, unchanged failure outputs and source
independence. Actual runtime builds pass for all Apple targets in
`aurora-integration-yoshi-attributes-build.log`,
`aurora-iphoneos-yoshi-attributes-build.log`, and
`aurora-iphonesimulator-yoshi-attributes-build.log`.

Yoshi's thrown egg, star and Egg Lay articles, assembled owner and runtime
hook are still outstanding. The attribute decoder is not yet used by a Yoshi
owner; neither live gameplay nor updated app packages were claimed here.


### Yoshi item article owners (2026-09-10)

Added `melee_yoshi_items_decode` for the three PlYs.dat article slots: thrown
egg (two animation entries), star (one), Egg Lay (none). Thrown egg has two
float attributes, 54 and 20; star has speed/acceleration 0.8 and -0.02. Egg
Lay has no special-attribute block or animation table, but retains a model
and hurtbone data. The shared article importer explicitly permits this null
special block for Egg Lay, alongside its existing Porygon2 case; other kinds
still require the appropriate special data.

`yoshi-items-tests.log` passes Yoshi, Luigi, Pikachu and Pichu article owners.
The Yoshi test overwrites/frees the source archive, verifies the scalar
parameters and three owned animation scripts, preserves Egg Lay's null
fields and hurtbones, and checks HSD pool cleanup. Actual runtime builds
pass for macOS, iPhone and Simulator in
`aurora-integration-yoshi-items-build.log`,
`aurora-iphoneos-yoshi-items-build.log`, and
`aurora-iphonesimulator-yoshi-items-build.log`.

These item owners are not yet connected to a complete Yoshi fighter owner.
Live egg throws, stars, Egg Lay captures and their effects remain unverified.
App packages were not rebuilt for this unused loader.


### Yoshi assembled owner, runtime hookup and first combat (2026-09-10)

Added `melee_yoshi_data_decode`: 314 motion records, 14 demo dynamics pairs,
six costumes and five part-animation groups (4/4/3/4/4 entries), plus the
previously decoded attributes and three item articles. Yoshi's guard
descriptor has a null shield model and zero scale. The shared model importer
now preserves a null guard joint while still requiring the auxiliary model.
The assembled test explicitly verifies that null guard descriptor.

`yoshi-data-tests.log` passes four assembled-owner tests including Yoshi,
Jigglypuff and Pikachu/Pichu. `yoshi-hook-tests.log` passes Yoshi's articles,
assembled owner, one effect model and ten results records/scripts. Yoshi's
259 available animations and 314 scripts survive source-buffer disposal.
The owner is connected to the runtime fighter cache.

The first live run (`yoshi-live.log`) crashed during material-animation
setup. `S_UNK_YOSHI2` aliases two consecutive TempS visibility choices; its
unused x4 field was declared as an integer, shifting the second choice on
ARM64. Native x4 is now a pointer, with static assertions against TempS
size and both second-choice offsets. The console layout is unchanged.

A replay (`yoshi-visibility-retry.log`) failed character selection because
fixed-frame cursor movement overshot the icon. The Yoshi probe now steers
through controller input using a read-only selection-token/icon delta,
confirming only after settling at the target. `yoshi-guided.log` passed
2800 submissions after confirming Yoshi at frame 1102. The final image was
inspected: Yoshi 30%, Ness 39%, Onett, timer 01:41.58. This is limited combat,
not comprehensive special-move, egg-shield, Egg Lay capture or rematch proof.

Actual runtime builds passed for all targets in
`aurora-*-yoshi-visibility-build.log`. Updated app packages, each rebuilding
its runtime first, passed in `mac-yoshi-app-build.log`,
`iphone-yoshi-app-build.log`, and `simulator-yoshi-app-build.log`.


Yoshi rematch and shared Egg Lay skeleton follow-up:

`--yoshi-rematch` uses the guided CSS selection and checks Yoshi is active
at submission 6000 after results, CSS and stage selection. The first run
(`yoshi-rematch.log`) terminated with a GPU completion timeout at frame 1591.
A repeat (`yoshi-rematch-retry.log`) passed: first match frame 1586,
results 2301, CSS 4013, stage selection 4403, second match 4587. The final
capture was inspected: Yoshi/Ness, Onett, both 0%, timer 01:38.51. The
intermittent GPU timeout is unresolved.

A controller-driven `--yoshi-specials` probe exposed a crash in Egg Lay's
captured-fighter animation (`yoshi-specials.log`, ftPartsRemap). Retail
PlCo has 34 skeleton tables, including the shared Egg Lay skeleton at
index 33, whereas the native owner had only decoded the 33 fighter-kind
tables. Both owned skeleton and accessory tables now include that extra
entry. The sanitizer test `yoshi-egg-parts-test.log` validates all 34 maps,
2321 joints, 16 accessories and 78914 original consumer remaps after
source disposal. The first replay survived but did not complete the side
special because its test stick input was below the directional threshold;
that input was corrected to full strength. State observations alone do not
prove complete move, hitbox, capture/release or visual fidelity.

The updated runtime was built into all three app packages:
`mac-yoshi-egg-app-build.log`, `iphoneos-yoshi-egg-app-build.log`,
`iphonesimulator-yoshi-egg-app-build.log`. This does not establish actual
iPhone gameplay or physical-controller verification.

The corrected `yoshi-specials-full.log` replay passed 7000 submissions:
egg shield state 342 at frame 2488, neutral special 346 at 2622,
Egg Roll loop 361 at 3699, Egg Throw 364 at 3861, and down-special
landing 367 at 4311. Final image inspected: Yoshi 60%, Ness 0%, Onett,
timer 00:31.47. This verifies the observed states and sustained native
execution; targeted Egg Lay capture/release and article-level checks
remain to be added.

Screenshot export correction: the prior `game_startup` PNG writer preserved
XFB texture alpha, which is zero in the shared gameplay captures. Their
RGB channels contain gameplay, but alpha-aware viewers show black or
transparent images. The exporter now writes RGB PNGs because XFB scanout
is opaque. `*-opaque.png` copies of yoshi-guided, purin-hat-retry and
pichu-specials preserve every RGB pixel exactly while omitting alpha.
The probe rebuild passed in `opaque-capture-build.log`. Prior visual
inspection did not detect this viewer-dependent transparency issue.


Targeted Yoshi Egg Lay capture/release:

`--yoshi-capture` drives ordinary controller input toward Ness, attempts
neutral special, then requires the victim's YoshiEgg state, owned accessory
joint, hidden fighter mesh and shared skeleton 33. It requires a later
visible Wait state before passing. The first run (`yoshi-capture.log`)
reached the corrected skeleton but exposed a second missing entry:
ftYs_SpecialN_8012CDD4 reads item-table slot 3, a standalone capture joint,
while the native owner held only the first three article entries. The
owner now decodes and retains this fourth entry as an owned scene joint.
The extended source-disposal test loads and removes that joint after
freeing the original archive. Both Yoshi item/data tests passed in
`yoshi-capture-model-tests.log`.

`yoshi-capture-model.log` passed 7000 submissions. Ness entered the egg at
frame 3137 (timer 199, skeleton 33), then returned visibly to Wait at 3414.
The final capture was inspected: Yoshi 0%, Ness 40%, Onett, timer 00:31.53.
The fresh PNG is RGB without an alpha channel, also validating the export
correction against actual framebuffer readback. Capture/release fidelity
for other opponents, mash/damage escape variants, and mid-capture visual
inspection remain unverified.

All runtime-building app packagers passed with this fix:
`mac-yoshi-capture-app-build.log`, `iphoneos-yoshi-capture-app-build.log`,
`iphonesimulator-yoshi-capture-app-build.log`. Device gameplay remains
unverified for this change.


Actual iOS Simulator touch session for Yoshi/Bowser:

`ios-control-session-yoshi.log` and `.xcresult` record a successful
`GameUITests.testControlSession` on the booted iPhone 17 Pro / iOS 27
Simulator using the rebuilt native runtime framework and actual SwiftUI
app/touch controls. The app was launched silently with the local CISO.
Touches selected Yoshi, enabled slot 2 CPU (the game assigned Bowser),
selected Onett, and started a normal two-minute match. Inputs included
A, B, X, movement, R and Start. This is app-level touch delivery and visual
coverage, not per-motion validation of every command.

Reviewed screenshots in `native/build/ios-control-session-yoshi/`:
frame-5 Yoshi selected; frame-8 CPU Bowser; frame-13 active match at
01:51.41 with Yoshi 5% / Bowser 0%; frame-15 combat at 01:32.39 with
73% / 12%; frame-19 actual P1 Pause overlay; frame-21 resumed gameplay at
00:24.91 with 21% / 72%; frame-23 full time-match results (Bowser 2 KOs,
Yoshi 2 falls, totals +2/-2). Touch Start progressed through results,
CSS and stage selection; frame-29 shows a second active Yoshi/Bowser
Onett match at 01:54.51 with both at 0%. The session was explicitly quit
and XCTest exited successfully. Screenshots came from the Simulator
screen, not host desktop capture or the windowless runner.

No physical iPhone, iPad, Bluetooth controller, simultaneous multi-touch
combo, or audio verification is established by this silent Simulator run.


macOS binding display follow-up:

The private VM window inventory was rechecked and still contained
loginwindow, so no host/guest macOS GUI interactions were attempted.
The gameplay keyboard hint previously hard-coded the default keys even
after remapping. It now reads the current InputModel bindings, matching
the controls editor. Both surfaces share labels for letters, numbers,
punctuation, navigation, function and keypad keys; missing actions display
Unassigned. Labels describe the existing physical key-code bindings.

`apple-binding-label-tests.log` passed the actual Apple InputModel test:
remapping attack to X, persistence through a new model, current binding
labels, input edges, Command-modified release, focus clearing, synthetic
controller slot stability and disconnect. Synthetic controller objects do
not establish physical/Bluetooth controller behavior. All three app
packagers rebuilt successfully in `*-binding-label-app-build.log`.
Live macOS keyboard/editor GUI verification remains unavailable until the
private test VM is logged in.


Dr. Mario live native coverage:

The new `--doctor-mario` test unlocks Dr. Mario in test memory, steers the
CSS token with read-only icon-position feedback, and uses normal controller
inputs. Its observer requires Ft_Kind_DrMario, the DrMario_Vitamin and
DrMario_Sheet articles, active reflection, Super Jump Punch and Tornado.
`doctor-live.log` passed 4000 submissions: selected at 1109, pill spawned
2634, Tornado 2722, sheet 3001, reflection 3002, Super Jump Punch 3082.
The final RGB capture was inspected: Dr. Mario 90%, Ness 56%, Onett,
timer 01:21.29. This expands live coverage of the previously connected
native loader; no production runtime behavior was changed for this test.
Move variants, exact hit/physics fidelity, alternate costumes and actual
iOS execution for Dr. Mario remain unverified.

`--doctor-mario-rematch` also passed 6000 submissions in
`doctor-rematch.log`: first match 1587, results 2304, CSS 4014,
stage selection 4403 and second match 4587. The second-match capture
was inspected: Dr. Mario/Ness, Onett, both 0%, timer 01:38.51.


Ganondorf live native coverage and part-animation fix:

The new `--ganondorf`/`--ganondorf-rematch` probes unlock Ganondorf only in
test memory and guide selection through controller input with read-only
icon feedback. Live verification exposed a null part-animation group
(`ganon-live.log`, ftAnim_ApplyPartAnim at frame 1751). Ganondorf's retail
data has five groups, with counts 4/4/3/6/6; the shared Captain/Ganon owner
previously decoded only the first three. Ganondorf now receives all five,
while Captain Falcon retains three. The extended data test traverses the
12 additional animation trees after source disposal. Both Captain/Ganon
data tests passed in `ganon-parts-tests.log`.

`ganon-parts-live.log` passed 4000 submissions: Warlock Punch frame 2322,
Gerudo Dragon 3002, Dark Dive 3202, Wizard Foot 3652. Final image inspected:
Ganondorf 65% in the left offscreen indicator, Ness 0%, Onett, 01:21.51.
These are special-state observations and sustained execution, not complete
move/hitbox/physics or every animation variant validation. The rebuilt app
packages passed in `mac-ganon-parts-app-build.log`,
`iphoneos-ganon-parts-app-build.log`, and
`iphonesimulator-ganon-parts-app-build.log`. Actual Ganondorf iOS gameplay
and physical-device/controller behavior remain unverified.

`ganon-rematch.log` passed 6000 submissions: first match 1587, results
2302, CSS 4014, stage selection 4403, second active match 4587. The
final capture was inspected: Ganondorf/Ness on Onett, both 0%, 01:38.51.


Zelda attribute conversion groundwork:

Added `melee_zelda_attributes_decode` for the 0xA8 special-attribute block
in the user's extracted PlZd.dat. It converts 32 finite float parameter
words and 9 integer words, and preserves the reflector behavior byte and
three trailing bytes. Static assertions check the total layout and the
ReflectDesc offset 0x84. Legacy x28/x30 slots retain their scalar bits;
the retail values are floating-point parameters. Output is assigned only
after successful validation.

`zelda-attributes-tests.log` passed the retail Zelda decode, every scalar
word/byte check, invalid float cases, all truncation lengths and source
independence, alongside Ganondorf and Yoshi regressions. All three actual
runtime targets built in `aurora-*-zelda-attributes-build.log`. Zelda's
assembled fighter owner, items, effects and Zelda/Sheik transformation
are not connected or verified by this attribute-only step.


Zelda Din's Fire item ownership:

Added `melee_zelda_items_decode` owning the projectile and explosion
articles at Zelda's two item-table slots, including their two/one state
scripts. Their special parameter schemas are 48 bytes (12 floats) and
20 bytes (5 floats), respectively. Both retail item model descriptors
have null joints and zero bones; the original gameplay renders these
through effects. The owner preserves that representation.

`zelda-items-tests.log` passed source-overwrite/free checks for both
articles, three scripts, representative projectile/explosion parameters,
null joint descriptors and HSD pool cleanup, alongside Yoshi and
Pikachu/Pichu item regressions (4 tests). All three actual runtime targets
built in `aurora-*-zelda-items-build.log`. This supplies item ownership
for the upcoming Zelda fighter owner; it is not live projectile/effect
or Zelda/Sheik transformation verification.

Zelda normal-match data now has an assembled native owner (`zelda_data.c`):
311 motion records, 14 demo dynamics pairs, five costume visibility tables,
and three part-animation groups (4/4/3), with independently owned Din’s Fire
articles. Counts were checked against the retail archive tables and costume
list. `hsd_zelda_data` overwrites and frees both input archives, then traverses
all 261 available animation trees, checks 311 script pointers, and loads the auxiliary
joint before checking HSD pool cleanup. This is data integration only; Zelda
is not yet connected to live gameplay and Sheik transformation is outstanding.

Sheik's native data owner now assembles 317 motion records (264 animations),
14 demo dynamics pairs, five costume visibility tables and two part-animation
groups (4/4). The 0x74 fighter attributes decode 27 floats and two integers.
`sheik-attributes-tests.log` covers every word, truncation, nonfinite rejection
and retained values after source overwrite, alongside Zelda regression checks.
The item owner includes thrown/held needles, Vanish and chain. Chain scalar
parameters preserve raw padding and relocate its two embedded joint pointers
into owned native scenes; Vanish has no special attribute block. Item tests
load all three article models and both chain segment descriptors after source
disposal and check seven scripts and HSD pool cleanup. The assembled owner
test traverses all 264 animation trees after releasing both inputs.
These loaders are not yet wired into live gameplay; Zelda/Sheik transformation,
effects and results integration remain outstanding.

Zelda and Sheik are now hooked into the native runtime's process-lifetime
fighter-data cache. Their shared effect archive passes all seven model
decodes; both results archives pass the ten-motion ownership test
(`zelda-runtime-data-tests.log`). All three native runtime builds pass
(`aurora-*-zelda-hook-build.log`). The windowless `--zelda` probe selects
Zelda through ordinary CSS input, enters Onett, and sends the regular combat
sequence including down-special. `zelda-active-live.log` reaches 4,000 GPU
submissions with Zelda active at frame 1,588 and Sheik active at frame 2,765.
The observer checks `Player_GetEntity(0)` so preloaded dormant fighters cannot
satisfy transformation coverage. Its opaque screenshot shows ongoing combat.
This verifies Zelda-to-Sheik transformation only; the reverse direction,
all specials, live results/rematch and iOS gameplay for these fighters remain
unverified. The earlier `zelda-live.log` observer counted dormant entities
and must not be used as transformation evidence.

The `--zelda` probe now verifies both transformation directions in order using
the active player entity, and submits a second down-special via ordinary PAD
input when Sheik is waiting. `zelda-roundtrip.log` passes 5,000 submissions:
Zelda at frame 1,587, Sheik at 2,765, Zelda again at 3,038, with continued
combat and a visible framebuffer at the end. No forced motion-state changes
are used. All three app packages were rebuilt with the hooked Zelda/Sheik
runtime (`mac-zelda-app-build.log`, `iphoneos-zelda-app-build.log`,
`iphonesimulator-zelda-app-build.log`). This supersedes the earlier reverse
transformation limitation; per-special, live results/rematch and iOS gameplay
coverage for the pair still remain outstanding.

`--zelda-rematch` now separately verifies results and re-entry without requiring
a transformation in the shortened first match. `zelda-rematch.log` passes
6,000 submissions: first match 1,587, results 2,303, returned CSS 4,014, SSS
4,403 and second match 4,588. The final screenshot shows Zelda and Ness at
0% in the second Onett match. This proves Zelda results/rematch and reloading
the paired fighter data; finishing a match as Sheik remains unverified.
The new observer mode does not send transformation inputs during rematches.
No production runtime changes were needed for this check.

The Zelda/Sheik special-move probe exposed a Sheik chain heap overrun at
frame 3,216 (`zelda-specials.log`). The fighter item table has six entries:
four articles plus two full pose skeletons used by the chain motion blending.
The native owner now retains both pose skeletons and frees them with the
articles. `ftSk_SpecialS_80110610` accesses the typed joint `child` field
instead of treating the descriptor as an array of pointers. The expanded
item test loads both poses after overwriting/freeing the input archive; all
four targeted data/item tests pass (`sheik-chain-poses-tests.log`).
All three runtime and app builds pass (`aurora-*-sheik-chain-build.log` and
`*-sheik-chain-app-build.log`). The first fixed live run observed all six
article types and both fighters' neutral/side/up specials and transformations,
but reached the default 120-second runner watchdog at frame 6,152. It must
not be reported as a completed 8,000-frame test. The extended scenario now
uses the existing 300-second allowance for long special-move probes.

`zelda-specials-complete.log` now passes all 8,000 submissions with the chain
fix: Zelda and Sheik each enter neutral, side and up specials; the item scan
observes Din's Fire projectile/explosion, held/thrown needles, chain and Vanish;
the active player transforms to Sheik at 2,886 and back to Zelda at 3,599.
The final opaque capture shows both fighters still on Onett. This is basic
move-path coverage, not every aerial/grounded variant, collision, reflect or
chain-input combination. Physical devices and live iOS verification of this
fighter pair remain outstanding.

The updated actual iOS Simulator app passes the interactive touch session
`ios-control-session-zelda.log` (509.183 seconds, XCTest exit 0), with the
Sheik chain fix packaged in its runtime. Touch input selects Zelda, enables
a Yoshi CPU and selects Onett. Frame 12 visibly captures Nayru's Love after
touch B; the session also delivers X, movement, A and R, and verifies pause
(frame 17) and resume. The full two-minute match ties, enters Sudden Death
(frame 20), shows Zelda's winner/results screen (frame 22), and starts a
second Onett match with both fighters at 0% (frame 27). The session quits
explicitly and terminates the app. Screenshots and xcresult are stored under
`native/build/ios-control-session-zelda*`. This is Simulator touch evidence,
not physical-device/controller validation. The driver currently supports
one touch gesture at a time; transformation and other simultaneous
stick-plus-button combinations have not been verified through iOS touch.

Link/Young Link shared fighter attributes now decode at their retail 0xDC
size: 28 finite float words, 23 integer words and 16 raw bytes (sword colors/
padding and the unused xC0 slot). The legacy pointer labels at x94/x9C/xA0
were corrected to signed integer fields: neither retail archive relocates
these slots, and their values are small integers. This prevents ARM64
pointer expansion from shifting subsequent parameters. Compile-time checks
cover the full size, sword block and absorb descriptor offsets.
`link-attributes-tests.log` passes both characters, every word's endian/raw
handling, every truncation length, nonfinite float rejection, unchanged output
on failure and source overwrite independence, plus Zelda/Sheik regressions.
Link/Young Link item ownership and assembled data integration remain pending;
this decoder is not yet connected to playable fighters.

Link and Young Link bomb/bow articles and Young Link milk now decode through
the native item owner. Bomb attributes are corrected to the actual 52-byte
retail block: five integer words and eight floats, removing an unused trailing
velocity array and correcting x4/x8 integer labels. Bow/milk unused scalar
blocks are retained. `link-basic-items-tests.log` passes both variants plus
Zelda/Sheik item regressions: overwrite/free the source, instantiate each
model, check state/script tables, verify bomb lifetime and parameters, and
check HSD pool cleanup. Link exposes two scripts across bomb/bow; Young Link
exposes four across bomb/bow/milk. Boomerang, arrow, hookshot embedded
models/animations and the full fighter item table remain to be integrated.

Link/Young Link arrows and hookshots now relocate their embedded model
descriptors into independently owned native scenes: two arrow variants and
three hookshot parts per character. The arrow declaration's unused trailing
float was removed to match the 44-byte retail block (nine scalar floats and
two relocated pointers). Hookshot scalar prefixes decode 19 floats and two
integer segment counts before assigning native pointers. The regression
`link-tether-items-tests.log` passes both variants plus bomb/bow/milk and
Sheik regressions, including six model instantiations per character after
source overwrite/free, variant-specific parameters, and HSD pool cleanup.
Young Link's arrow parameter x0 is 55, versus Link's 60. Boomerang animation
bundles and the complete fighter item-table owner remain pending.

Link/Young Link boomerangs now own both embedded joint models and their
three-pointer animation bundles. The complete seven-entry item owner includes
bomb, boomerang, hookshot, arrow, bow, optional Young Link milk and the fighter
accessory joint. `link-items-tests.log` passes both variants and regressions;
boomerang bundles are instantiated and animated at frames 0 and 10 after
source overwrite/free, with HSD pool cleanup verified.
The assembled native `link_data` owner now covers both fighters: 314 motion
records, 14 demo dynamics pairs, five costume visibility tables and 4/4/3
part-animation groups, with counts checked against the retail tables.
`link-data-tests.log` traverses Link's 261 and Young Link's 264 available
animation trees and checks 314 script pointers after source disposal; all
six targeted tests pass. All three native runtime builds pass in
`aurora-*-link-data-build.log`. Runtime hookup, effects, results and live
gameplay for these fighters are still outstanding.

Link and Young Link now use the native fighter-data cache in `ftdata.c`.
Their shared effects archive has four records ending at offset 0x58, before
texture payload at 0x60; the effects decoder now uses that verified count
instead of scanning the texture bytes. `link-runtime-data-tests.log` passes
all four effect models, both ten-motion results archives and effects regressions.
`--link` selects Link through ordinary CSS input and verifies his active
player entity. `link-live.log` passes 4,000 native GPU submissions on Onett,
with Link at 64% and Ness at 40%; the opaque screenshot is visibly rendered.
All three runtime builds pass (`aurora-*-link-hook-build.log`). This is initial
Link combat evidence; full specials, hookshot, results/rematch, Young Link
live gameplay and iOS gameplay for these fighters remain unverified.

Young Link's initial live match now passes as well: `young-link-live.log`
completed 4,000 native GPU submissions with the active player entity checked
as Young Link (kind 20). The opaque `young-link-live.png` shows Onett with
Young Link at 96% and Ness at 99%. The probe unlocks Young Link in memory
and selects him through ordinary CSS input; it does not modify a save file.
This covers initial combat, not his complete moveset or match lifecycle.

`--link-specials` and `--young-link-specials` drive neutral/side/up/down B
and Z through normal PAD input, waiting for the active fighter's idle state.
They require all four special action families plus a grab, and independently
observe the correct variant's arrow, bow, boomerang, bomb and hookshot in
the live item list. They fail if coverage is incomplete at 7,000 submissions.
`link-specials-live.log` passes: bow/arrow at 1982/1986, boomerang at 2166,
spin attack at 2342, hookshot at 2528 and bomb at 2716, followed by continued
gameplay to 7,000 submissions. The opaque capture shows Link at 94% and Ness
at 0%. This verifies basic move execution and equipment creation, not every
ground/air variant, projectile collision, tether recovery or results/rematch.

`young-link-specials-live.log` also passes 7,000 submissions with the same
requirements for Young Link's distinct item kinds: bow/arrow at 1982/1985,
boomerang at 2166, spin attack at 2341, hookshot at 2682 and bomb at 2870.
The final opaque screenshot was inspected and shows Young Link and Ness
rendering on Onett. Young Link's milk taunt and both fighters' airborne
tether behavior, results/rematch and iOS gameplay remain unverified.
Only the windowless test runner and this evidence log changed for these
special-move runs; no production fix was needed for the exercised paths.

`--link-rematch` and `--young-link-rematch` use the existing shortened-timer
lifecycle probe, requiring results, a return to CSS and a second active match,
plus the correct active fighter kind at submission 6,000. The test changes
the first match's remaining timer to five seconds; subsequent menu navigation
uses ordinary PAD input. `link-rematch.log` passes: initial match at 1588,
results at 2304, CSS at 4014, stage selection at 4403 and second match at
4587. The final opaque capture shows Link and Ness at 0%, with 01:38.53
remaining in the second match. This is lifecycle evidence, not a full-length
first-match or repeated long-session stability test.

`young-link-rematch.log` also passes to submission 6,000: first match 1588,
results 2305, CSS 4013, stage selection 4404 and second match 4588. The
active entity is Young Link (kind 20), and the inspected opaque capture
shows both fighters at 0% with 01:38.53 remaining. Both lifecycle probes
exit zero; no production change was required. Milk taunt, aerial tether
recovery and physical-device gameplay remain outstanding for these paths.

`--link-extras` / `--young-link-extras` extend the special-move probe with
a normal jump followed by Z. They require a live hookshot article while
the active fighter is in AirCatch, then a return to idle. Young Link also
uses D-pad Up and must create the milk article and return to idle with
the fighter's milk attachment cleared. `young-link-extras.log` passes
7,000 submissions: airborne hookshot at 3257, idle at 3300, milk at 3425
and taunt completion/detachment at 3608. The final opaque capture was
inspected. This checks airborne deployment, not attachment to a ledge
or recovery from below the stage; those require separate coverage.

`link-extras.log` also passes all requirements at 7,000 submissions. Link's
airborne hookshot is observed at 3477 and his return to idle at 3545; the
final opaque capture shows both fighters rendering normally on Onett.
Both extras runs exited zero without a production change. These tests
exercise one taunt facing direction and basic airborne hookshot deployment,
not all attachment/collision variants or long-session memory stability.

Samus's special attribute decoder now converts the retail 0xD4-byte block
(39 finite float words and 14 integer words), including the collision box
at 0x84. In the user's PlSs.dat, the block begins at 0x35E8 and the next
referenced object begins at 0x36BC. Its xD0 slot contains non-relocated
integer 180, and has no known pointer consumer; the legacy UNK_T declaration
is corrected to s32 so ARM64 retains the retail layout. Static assertions
check the total size, collision box and last scalar offsets.
`samus-attributes-tests.log` passes every decoded word, every float's
nonfinite-input rejection, every truncated block length and source overwrite
independence, alongside Link, Young Link and Sheik regressions. All three
runtime builds pass (`aurora-*-samus-attributes-build.log`). Samus's native
item ownership, assembled fighter data and runtime hookup remain pending;
this decoder does not yet make her playable.

Samus's bomb, charge-shot and missile articles now decode through the native
item owner. Their special blocks are 16 bytes (four floats), 32 bytes (seven
floats and integer x4) and 56 bytes (14 floats). The bomb's formerly declared
trailing three floats and missile's trailing two floats overlap item scripts
in retail PlSs.dat; those unused fields are removed, with static size checks.
The three articles have 2/9/4 state descriptors. `hsd_samus_basic_items`
compares every scalar word to the retail source, erases/frees that source,
then instantiates and animates all 15 state descriptors through frames 0–120.
It verifies 15 script pointers and HSD pool cleanup. The Samus test and both
Link regressions pass in `samus-basic-items-tests.log`; all three runtime
builds pass in `aurora-*-samus-basic-items-build.log`. The grapple article,
complete Samus fighter owner and live gameplay integration remain pending.

Samus's grapple article now owns its 100-byte scalar prefix (23 floats and
two integer segment counts), four embedded joint models and five animation
bundles. The fifth bundle animates the article's base model. Each bundle
relocates the first entries of the original joint/material/shape pointer
tables and binds to the model's child, matching the runtime consumers.
`hsd_samus_grapple` checks all 25 scalar words, erases/frees the source DAT,
then loads all five models and applies their child animations for frames
0–120 before checking HSD pool cleanup. It passes together with the Samus
projectile and both Link tether regressions in `samus-grapple-tests.log`.
This is data/animation ownership evidence; the fighter's fifth item-table
entry (arm-cannon attachment), assembled owner and live match remain pending.

The native Samus item-table owner now assembles all five entries: four
articles and the throw attachment, including its base joint/material
animations and four directional throw tracks. That attachment exposed
previously unsupported instance joints. The scene decoder now preserves
their borrowed references, requires each referenced joint to have an ordinary
owner, rejects duplicate ordinary ownership and excludes instance children
from the runtime object enumeration, matching HSD's load/resolve/free rules.
`hsd_samus_items` verifies the 25 instance joints resolve to ordinarily owned
siblings, rejects a mutated duplicate-ownership tree, erases/frees the DAT,
and runs all four throw animation combinations for frames 0–120 before
checking pool cleanup. All 20 targeted Samus/Link/Yoshi/Zelda/Sheik tests
pass in `samus-items-tests.log`. Samus still needs the assembled fighter
owner and runtime hookup; these checks do not prove her live playability.

Samus's assembled owner and runtime hookup are now implemented. The retail
motion table spans 0x79A8–0x9700 (313 records); her data uses 14 demo dynamics
pairs, five costumes and one four-entry part-animation group. The assembled
test traverses 265 animation trees and 313 script pointers after overwriting
and freeing both DAT and AJ source buffers. All four Samus data/item tests
pass in `samus-data-tests.log`. Her effects archive's four models also pass
in `samus-effects-tests.log`. All three native runtime builds pass in
`aurora-*-samus-hook-build.log`.
`--samus` selects her through ordinary CSS input and checks the active player
entity. `samus-live.log` exits zero at 4,000 native GPU submissions with
Samus at 30% and Ness at 63% on Onett. The opaque capture was inspected and
shows both fighters rendering. This is initial combat evidence; focused
special moves, grabs/throws, results/rematch and iOS gameplay remain to test.

`--samus-specials` now verifies charge hold/fire, regular and smash missile
states, Screw Attack, bomb creation, ground grab and airborne grapple followed
by idle. It independently observes all four Samus item kinds. The first
`samus-specials.log` run failed coverage because a 40-unit stick input was
below the side-special threshold and triggered neutral B. The corrected
probe holds the stick sideways for eight submissions before B for a regular
missile and presses stick+B together for a smash missile.
`samus-specials-complete.log` exits zero at 7,000 submissions: charge fire
2174, regular/smash missiles 2652/2824, Screw Attack 3003, bomb 3541,
ground grapple article 3719 and airborne grapple state 3911, returning to
idle at 3997. The opaque final capture was inspected. Only test input changed;
no production fix was needed for these paths. Full charge storage/cancel,
projectile collision variants, throws, ledge tether recovery and rematches
still need dedicated coverage.

Samus's results data and rematch now pass. `samus-results-data-tests.log`
checks ten owned results motion records and scripts, including invalid ranges
and source disposal; this is registered as `hsd_results_motions_Ss`.
`--samus-rematch` uses the existing five-second first-match timer fixture,
then ordinary PAD input to leave results and select the next stage. It
requires results, return to CSS, a second active match, and an active Samus
entity at submission 6,000. `samus-rematch.log` exits zero: first match 1587,
results 2303, CSS 4014 and stage selection 4403, followed by a second match.
The inspected opaque capture shows Samus and Ness at 0%, 01:38.53 remaining.
This proves the shortened-match lifecycle; full-length Samus matches,
extended repeated sessions and iOS gameplay remain unverified.

Samus now also passes an actual iOS Simulator UI session with the packaged
runtime. `ios-control-session-samus.log` / `.xcresult` cover selection through
the onscreen stick/A/Start controls, a full two-minute Onett match against
CPU Ness, pause/resume, results and entry into a second match. Screenshot
`ios-control-session-samus/frame-13.png` shows the charge-shot effect after
onscreen B; frame 18 shows P1 Pause, frame 19 resumed play, frame 22 Ness's
results screen, frame 24 returned CSS and frame 28 the second live match
(01:53.27, Samus 30%, Ness 0%). The session also delivered X, movement, A,
R and Z inputs, but those screenshots do not individually establish every
move variant. Explicit quit command 29 ended the test cleanly; XCTest and
xcodebuild exited successfully. This tests the iOS app in Simulator, not a
physical iPhone/iPad, Bluetooth controller, audio or simultaneous multitouch.

Peach's native special-parameter decoder now converts the retail 0xC0-byte
block (37 finite floats and 11 integer words), including the three-entry
turnip substitute-item chance table and the absorb descriptor at 0xAC.
The existing table offset comment was corrected from 0x1C to 0x18; static
assertions verify that offset, the descriptor offset and total size. In
PlPe.dat the block spans 0x3A1C–0x3ADC. `peach-attributes-tests.log` checks
every decoded word, float nonfinite rejection, all truncated lengths and
source independence, with Samus/Zelda regressions. Peach's item ownership,
assembled fighter loader and live gameplay are still pending.

Peach's five-entry native item owner now decodes explosion, turnip, parasol,
Toad and spores in retail registration order. The scalar layouts are absent,
72, 4, 4 and 16 bytes respectively; the turnip table length must be eight.
`peach-items-tests.log` verifies all 24 scalar words against the archive,
then erases/frees that archive and exercises all seven model-backed state
animations through frame 120, four scripts and HSD pool cleanup. Zelda and
Samus item regressions also pass. The ten total state descriptors include
three without a model.

The native turnip selector avoids the retail loop's final out-of-bounds odds
read. Its regression calls the actual gameplay function with 65,536 seeds,
compares each selection to the retail cumulative odds and covers all eight
variants under address/undefined-behavior sanitizers. The original GameCube
build keeps its matching loop. macOS, iPhoneOS and iPhone Simulator runtime
builds pass (`peach-items-build.log`, `aurora-iphoneos-peach-items-build.log`,
`aurora-iphonesimulator-peach-items-build.log`). Peach's assembled fighter
loader and live gameplay remain pending; these changes are not yet packaged
in the app bundles.

Peach's assembled normal-match fighter owner is now connected in ftdata.c.
It owns 318 motion/script records (265 animation trees), 14 demo dynamics
pairs, five costume part tables, three part-animation groups (4/4/3), common
and special parameters, auxiliary data and the five items. The retail normal
motion table spans 0x8680–0xA450. `peach-data-tests.log` covers archive/AJ
source disposal, lazy animation decoding, auxiliary model loading and pool
cleanup, with Samus regressions. The effect loader recognizes Peach's single
record before texture payload at 0x20; its model/animation test passes.
`peach-results-data-tests.log` verifies ten results motion/script records.

`peach-guided.log` reaches a live Peach/Ness Onett match at 4,000 submissions;
its opaque capture shows Peach holding a turnip, with Peach at 30% and Ness
at 63%. The initial `peach-live.log` failed coverage because the generic CSS
input sequence interfered and selected Bowser; the guided Peach path now
excludes that sequence. All three runtime and app-package builds pass
(`aurora-*-peach-hook-build.log`, `mac-peach-app-build.log`,
`iphoneos-peach-app-build.log`, `iphonesimulator-peach-app-build.log`). Device
installation still requires signing/provisioning. Peach's full special-move
coverage and physical-device controls remain unverified.

`peach-rematch.log` passes 6,000 submissions: first match at 1587, results at
2303, returned character selection at 4013, stage selection at 4403, and a
second live Peach/Ness Onett match at 4587. The first match uses the existing
five-second timer fixture; this is transition coverage, not a full-duration
match test. `peach-rematch.png` is the verified opaque second-match capture.

`peach-specials.log` passes 7,000 native submissions using ordinary PAD inputs
with generic combat inputs disabled. Observed neutral special/Toad at 1981,
side special at 2162, up special/parasol at 2615, down special/turnip at 2795,
and return to idle at 2824. The final opaque capture is
`peach-specials.png`. Coverage requires all four motion-state activations and
all three directly spawned items. It does not yet prove the collision-triggered
side-special explosion or Toad counter-hit spores. This turn changes only the
native test runner, so the existing Peach app packages remain current for
production code.

`peach-float.log` passes 7,000 submissions after the four-special sequence:
normal controller inputs discard the held turnip, hold down+jump to enter
float at 2924, perform a neutral floating attack at 2926, release, and land
back in idle at 2975. No direct motion-state or position writes are used.
`peach-float.png` is the verified opaque final capture. This covers one float
attack, not all five directions, float duration/ledge cases, or counter-hit
collision effects.

Mewtwo now has an owned native fighter loader connected to ftdata.c. The
0x88-byte special block has 27 finite floats, six integer words, and four raw
reflector behavior/padding bytes; it spans 0x38B4–0x393C in PlMt.dat.
`mewtwo-attributes-tests.log` covers every field, all truncated lengths,
nonfinite rejection and source independence with Peach/Samus regressions.
Shadow Ball's legacy unused trailing four floats were removed: its actual
scalar block is 48 bytes (11 floats and one integer), followed by script data.
Disable uses eight bytes/two floats. `mewtwo-items-tests.log` and
`mewtwo-data-tests.log` verify both items, ten model-backed state animations,
eleven scripts, source disposal and pool cleanup.

The fighter owns 314 motion/script records, 263 animation trees, 14 demo
dynamics pairs, four costume part tables and three part-animation groups
(4/4/3). All four effect models and ten results motion records pass separate
checks (`mewtwo-effects-tests.log`, `mewtwo-results-data-tests.log`). The native
runtime builds on macOS, iPhoneOS and iPhone Simulator. `mewtwo-live.log`
passes 4,000 submissions after normal character selection input; Mewtwo and
Ness reach an active Onett match. `mewtwo-live.png` is the inspected opaque
capture. Unlocking Mewtwo is a test fixture; no production unlock behavior was
changed. Full special-move and rematch coverage remain pending.

All three Mewtwo app packages also build successfully:
`mac-mewtwo-app-build.log`, `iphoneos-mewtwo-app-build.log` and
`iphonesimulator-mewtwo-app-build.log`. The device build remains unsigned;
physical-device installation/testing requires development provisioning.

`mewtwo-specials.log` passes 7,000 native submissions using normal PAD inputs.
The probe observes Shadow Ball charging at 1999 and its release state at
2162, Confusion at 2603, Teleport start at 2783 and exit at 2800, Disable at
2964 with its item at 2978, and return to idle at 3003. Both character item
kinds are required. The inspected opaque final frame is
`mewtwo-specials.png`. This proves the activation/animation paths and item
creation; full-charge storage/cancel, Confusion reflection/grabs, projectile
hit effects and directed ledge recovery are still unverified. Only native
test input/coverage code changed this turn; production app packages remain
those from the Mewtwo loader build.

`mewtwo-rematch.log` passes 6,000 submissions using the existing shortened
first-match timer fixture: results, character selection, stage selection and
a second active Mewtwo/Ness Onett match all occur in order. The inspected
opaque final capture is `mewtwo-rematch.png`. This is transition coverage,
not a full-duration match or physical-device/controller test.

Ice Climbers groundwork: both PlPp.dat and PlNn.dat special-parameter blocks
are 0x15C bytes. The native codec converts 59 declared float fields and two
integers, preserving the 104 bytes represented as opaque arrays by the
legacy type. Those opaque regions are not claimed as decoded AI parameters.
`iceclimbers-attributes-tests.log` covers every declared field, raw-byte
preservation, nonfinite rejection, all truncated lengths and source disposal
for both archives, with Mewtwo regression coverage.

The Popo item owner decodes Ice Shot (52 bytes), Blizzard (24 bytes), and
recovery rope (36-byte scalar prefix plus two owned joint references).
`iceclimbers-items-tests.log` verifies scalar bytes, source disposal, the
Ice Shot animation through frame 120, two scripts, both rope models and pool
cleanup. Nana's item archive refers externally to Popo's Ice Shot joint,
animation and rope models; standalone decoding intentionally fails and its
cleanup is tested. That failure is not Nana item support. Nana's original
OnLoad copies attributes without registering items; Popo registers all three
shared item kinds. The paired fighter loader and live gameplay are still
pending. macOS/iPhoneOS/iPhone Simulator runtime builds pass
(`iceclimbers-items-build.log`, `aurora-iphoneos-iceclimbers-items-build.log`,
`aurora-iphonesimulator-iceclimbers-items-build.log`). App packages have not
been updated with this unconnected groundwork.

Ice Climbers paired fighter owners are now connected. Popo and Nana each
have 321 motion/script records; Popo owns 260 animation trees and Nana owns
eight unique trees. The original ftData_80085FD4 fallback selects Popo's
normal animation for Nana when she has no unique one. Runtime loading now
initializes both caches in Popo-first order, preserving that dependency.
Each has four costume part tables, 14 demo dynamics pairs and three part
animation groups (4/4/3). Nana's unused x48_items remains null: her original
OnLoad only copies attributes, while Popo registers the shared item kinds.

`iceclimbers-data-tests.log` verifies both complete owners after DAT/AJ
source disposal and HSD cleanup, with Mewtwo regression coverage.
`iceclimbers-effects-tests.log` passes the shared effect model. All three
native runtime builds pass. `iceclimbers-live.log` completes 4,000 submissions
with both Popo and Nana present in the player slot; its final opaque image
is partly obscured by Onett traffic. Special moves, partner recovery/loss,
results/rematches and physical-device controls still need verification.

`iceclimbers-pair.log` also passes 4,000 submissions. Its additional frame-1700
capture, `iceclimbers-pair.png.pair.png`, shows both partners at match start
before traffic reaches them (the GO overlay is still present). Mac, iPhoneOS
and Simulator app packaging passes in `mac-iceclimbers-app-build.log`,
`iphoneos-iceclimbers-app-build.log` and
`iphonesimulator-iceclimbers-app-build.log`; the device app remains unsigned
pending development provisioning.

`iceclimbers-specials-full.log` passes 7,000 native submissions with normal
PAD inputs: Ice Shot state at 2042/item at 2048, Blizzard at 2281/item at
2296, paired side special (Popo 344/Nana 359) at 2626, up special at 2866,
recovery-rope item at 2873, and both partners back in idle at 3011. Coverage
requires all four Popo special families, Nana's paired side-special state,
all three item kinds and simultaneous idle recovery. The inspected opaque
final capture is `iceclimbers-specials-full.png`.

The first `iceclimbers-specials.log` ends with coverage failure because a
40/80 horizontal test input did not reach the side-special threshold. The
corrected run uses 80/80. This is only a test-input correction; production
code/app packages are unchanged. Partner loss/respawn, directed ledge recovery,
full-match results/rematches and physical-device control testing remain
unverified.

The Ice Climbers rematch probe exposed a native results-loader omission:
`iceclimbers-rematch.log` aborted before results because Nana's x14 demo
motions had not been installed. The original Player_80036E20 loads both
members of paired characters; the native results path now explicitly loads
Nana alongside Popo from shared GmRstMPn.dat before creating demo fighters.
Both ten-record results decoders and Mewtwo/Peach regressions pass in
`iceclimbers-results-regressions.log`.

`iceclimbers-rematch-fixed.log` passes 6,000 submissions: first match at
1587, results at 2302, character selection at 4014, stage selection at 4404,
and a second active match at 4588. Final coverage requires both Popo and Nana
in the player slot. The opaque capture `iceclimbers-rematch-fixed.png` was
inspected. The first match uses the existing five-second timer fixture;
this is transition coverage rather than a full-duration match test.
Mac/iPhoneOS/Simulator runtime and app builds pass, including
`mac-iceclimbers-results-app-build.log`,
`iphoneos-iceclimbers-results-app-build.log`, and
`iphonesimulator-iceclimbers-results-app-build.log`. Device installation still
requires signing/provisioning; partner loss/respawn and physical-device
controls remain unverified.

Mr. Game & Watch groundwork: the native special-parameter codec handles the
0x94-byte block at 0x3778 (22 floats, ten integers and twenty raw costume/
outline color bytes). `gamewatch-attributes-tests.log` verifies all words,
color-byte preservation, nonfinite rejection, truncation and source disposal,
with Mewtwo regressions.

All ten attack articles now have owned descriptors. Their special pointers
refer to two outline part-index lists, decoded into native pointer-bearing
storage. Chef additionally owns 28 floats: three shared parameters and five
five-float food records. The legacy Chef entries[1] declaration is corrected
to the retail five-entry table. `gamewatch-items-tests.log` compares all twenty
index lists and all Chef values, erases/frees the archive, runs nineteen model
state animations through frame 120, checks three scripts and HSD pool cleanup.
Mewtwo and Ice Climbers regressions pass (Nana standalone rejection remains
a negative test, not standalone asset support).

macOS/iPhoneOS/iPhone Simulator runtime builds pass in
`gamewatch-items-build.log`, `aurora-iphoneos-gamewatch-items-build.log` and
`aurora-iphonesimulator-gamewatch-items-build.log`. The assembled fighter
loader, actual outline rendering and live gameplay are still pending; these
new decoders are not yet connected or packaged in the app bundles.

Mr. Game & Watch's assembled fighter loader is now connected: 323 motion/
script records, 269 animation trees, four costume part tables, 14 demo
pairs and three part-animation groups (4/4/3). The motion table spans
0x7C58–0x9AA0. An eleventh item-table entry supplies the fighter's own outline
visibility lookup (eleven groups, 26 choices), now owned alongside the ten
articles. Decoder tests exercise that lookup after source disposal.
`gamewatch-data-tests.log` passes with Mewtwo regressions, and the ten results
motions decode in `gamewatch-results-data-tests.log`.

The first live run (`gamewatch-live.log`) found a Chef update crash: assigning
the shared 64-bit outline pointer overlapped Chef's food index because the
legacy item-state prefix declared the pointer as s32. Chef and Rescue now
use pointer-typed outline fields; a static assertion and union-alias test
protect the Chef index. `gamewatch-chef-fixed.log` completes 4,000 native
submissions without sanitizer errors. Its inspected opaque frame shows an
active Onett match, Game & Watch 72%/Ness 63%; Game & Watch is in the offscreen
indicator at the final frame. This does not verify every outline/color mode.
All three runtime builds pass. Full special-move, rematch and physical-device
control coverage remain pending.

Updated Game & Watch app packages also pass:
`mac-gamewatch-app-build.log`, `iphoneos-gamewatch-app-build.log` and
`iphonesimulator-gamewatch-app-build.log`. The physical-device build remains
unsigned until development provisioning is supplied.


Mr. Game & Watch follow-up coverage: `gamewatch-specials-complete.log`
reaches 7,000 submissions after activating Chef, Judge, Oil Panic's stance
and Fire recovery. It observes Chef food (122), Judge (120), Rescue (124),
and a return to idle at frame 2910. Oil Panic absorption and stored-charge
discharge are not covered by the stance test. The initial
`gamewatch-specials.log` incorrectly required the discharge article merely
from entering the stance; correcting that test expectation required no
production change. `gamewatch-rematch.log` reaches its 6,000-submission
success endpoint after results at 2304, character select at 4013, stage
select at 4403 and a second active match at 4587. The inspected opaque
`gamewatch-rematch.png` shows both fighters at 0% on Onett.

Kirby parameter groundwork: the 0x424-byte special block at 0x4E6C now
has a native decoder covering 214 finite floats, 49 integer words, a signed
16-bit aerial-jump parameter with preserved padding, and four raw reflector
behavior/padding bytes. This includes the parameter sets for copied neutral
specials, without yet implementing their asset loaders. Static layout
assertions cover the halfword, Marth/Roy copy blocks and Zelda reflector.
`kirby-attributes-tests.log` checks all values against the retail archive,
nonfinite rejection for every float, all truncation boundaries, unchanged
output on rejected input, distinctive signed-halfword/padding/behavior
bytes and source independence, with Game & Watch/Mewtwo regressions.
The integer schema was also compared against the source struct declarations.
Runtime builds pass in `kirby-attributes-macos-build.log`,
`kirby-attributes-iphoneos-build.log` and `kirby-attributes-simulator-build.log`.
Kirby's assembled fighter, articles and copied-ability asset loading remain
pending; this groundwork does not make Kirby playable yet.


Kirby base articles and assembled data now have owned decoders. The four
initialization articles are Cutter beam (four finite floats), hammer (no
special block), Unk1 star (one lifetime float), and Unk2 star (no special
block). Each has one model state. `kirby-items-tests.log` passes with
Mewtwo/Game & Watch regressions; the direct Kirby probe reports four model
animations and two scripts, exercised through frame 120 after source
archive erasure/free, followed by HSD pool cleanup.

The assembled owner covers 479 motion records at 0xB280–0xDF68, 433
animation trees, 468 scripts, six costume part tables, three partial-body
animation groups (3/3/3), and 18 demo dynamics pairs. Eleven retail motion
entries have no script (4, 5, 18, 19, 48, 54, 56, 61, 63, 65, 218); the test
compares script presence for every record against archive relocations.
`kirby-data-tests.log` passes Kirby, Mewtwo and Game & Watch after source
disposal, including all lazy animation trees and auxiliary joint cleanup.
macOS, iPhoneOS and iPhone Simulator runtime builds pass in
`kirby-items-build.log`, `kirby-data-iphoneos-build.log` and
`kirby-data-simulator-build.log`. These owners are not yet connected to
ftData loading; live gameplay, copied-ability assets and results motions
remain pending. Existing app bundles have not been replaced for this
unconnected groundwork.


Kirby's normal fighter owner is now connected in ftData loading, and
`--kirby` guides character selection through ordinary PAD input and checks
for an active Kirby fighter at the end of a 4,000-submission run. All three
runtime builds pass (`kirby-hook-build.log`,
`kirby-hook-iphoneos-build.log`, `kirby-hook-simulator-build.log`).
The first live run, `kirby-live.log`, terminates with exit 134 while entering
Onett: `ftKb_SpecialN_800EED50` attempts to load the opponent's copied-ability
archive through the legacy big-endian HSD parser. This is a real runtime
failure, not a successful playable Kirby test. No updated app packages or
Kirby gameplay screenshot are claimed.

The next required decoder is `PlKbCpNs.dat`, public root
`ftDataKirbyCopyNess` at 0x298: joint at 0x2128, one-group visibility
metadata at root+4 (table 0x1F8), and nonnull entries at root+0xC (0x248)
and root+0x10 (0x280). Those entries must be interpreted from their actual
consumers; `KirbyHatStruct.hat_dynamics` is a legacy heterogeneous pointer
view that also contains articles. The native loader must preserve those
roles and eventually support all opponent copy archives, plus any separate
costume assets, without bypassing the copy preloads.


Kirby's Ness copy archive now has an owned native decoder and preload hook:
`kirby-copy-regressions.log` passes the hat, visibility metadata and both
PK Flash articles (four model-state animations and four scripts), with
source erasure/free and pool cleanup. The copied articles use the same
44/20-byte scalar schemas as Ness's originals. Other opponent copy archives
and actual acquisition/use of the Ness ability still need coverage.

Live integration found and fixed two more issues. First,
`kirby-copy-live.log` hits a global-buffer-overflow in Stone: eight paths
cast the flag array to read a vector in the adjacent retail global.
Native paths now reference `ftKb_Init_803CB4EC.vec` directly, preserving
the matching GameCube path. Next, `kirby-stone-fixed.log` reaches inhale
but crashes loading the swallowed opponent's star model. The item table
contains a fifth joint entry in addition to the four attack articles.
The item owner now decodes and owns this joint; the native accessor uses
entry index four instead of the legacy 0x10 byte offset (which selects
entry two with 64-bit pointers). `kirby-star-tests.log` verifies the
fifth joint after archive disposal along with assembled and copy owners.

`kirby-star-fixed.log` completes 4,000 native GPU submissions, exit zero,
with Kirby active on Onett and no sanitizer report. Its visually inspected
opaque `kirby-star-fixed.png` shows Kirby at 90% and Ness at 41%. This is
baseline controller-driven gameplay, not comprehensive special-move or
copy-ability coverage. Results/rematch and other opponent copy archives
remain pending. Runtime builds pass in `kirby-star-fix-build.log`,
`kirby-star-iphoneos-build.log` and `kirby-star-simulator-build.log`.

Updated app packages pass in `mac-kirby-app-build.log`,
`iphoneos-kirby-app-build.log` and `iphonesimulator-kirby-app-build.log`.
The iPhoneOS package still requires development signing/provisioning for
physical-device installation. These builds do not establish physical
controller, touch or device runtime validation.


Kirby results/rematch coverage now passes. `kirby-results-data-tests.log`
validates ten owned results records with nine scripts and the intentional
null script slot from GmRstMKb.dat (`ftDemoResultMotionFileKirby`).
`kirby-results-regressions.log` passes with Game & Watch and Mewtwo.
`--kirby-rematch` runs the ordered results-to-character-select-to-second-match
sequence and requires an active Kirby fighter at the final frame.
`kirby-rematch.log` completes 6,000 submissions, exit zero, without a
sanitizer report: results at 2303, character select at 4013, stage select at
4403 and second active match at 4587. The inspected opaque
`kirby-rematch.png` shows Kirby and Ness at 0% on Onett. This uses the
short-timer transition fixture; it does not establish a full-length Kirby
match, all copied abilities, or all copy-cache entries across resets. No
production code changed in this follow-up, so existing app packages remain
current.


Kirby's directed base-special test (`--kirby-specials`) requires Inhale,
Hammer, Final Cutter and Stone activations, both Hammer/Cutter articles,
and return to idle. Ordinary PAD inputs drive it; generic combat inputs
are disabled for this mode. The initial `kirby-specials.log` fails in
Final Cutter effect 0x494: hard-coded fighter offset 0x5E8 and pointer
indices 0xB0/4 assume retail layout. Native effect setup now accesses
Fighter.parts[44].joint and parts[1].joint (retail bone stride 0x10). Its
callback also now reads Fighter.facing_dir directly instead of aliasing a
fighter as HSD_JObj to read scale.x at the coincident retail 0x2C offset.
The GameCube paths remain unchanged.

`kirby-specials-fixed.log` passes 7,000 submissions with the bone fix.
After the callback correction, `kirby-specials-complete.log` also passes
7,000 submissions, exit zero without sanitizer errors: Hammer article 51
at 2599, Cutter beam 50 at 3420, Stone at 3641 and idle at 3865. Final
Cutter is retried when interrupted before emitting its beam. The inspected
opaque final frame shows Kirby 60% and Ness 8% on Onett. This establishes
base activation/article coverage, not all hit interactions, directional
variants, copy acquisition/use or Stone transformations.

All three runtimes and app packages pass with both effect fixes:
`kirby-cutter-facing-build.log`, `kirby-cutter-facing-iphoneos-build.log`,
`kirby-cutter-facing-simulator-build.log`, `mac-kirby-specials-app-build.log`,
`iphoneos-kirby-specials-app-build.log`, and
`iphonesimulator-kirby-specials-app-build.log`. Physical-device testing is
still pending. The 25 copy archives named by the retail table were also
extracted from the supplied disc and indexed in
`native/build/kirby-copy-archives/inventory.json`; extraction and structural
inventory do not imply native support beyond the Ness copy decoder.


Kirby's Ness copy acquisition/use is now verified by `--kirby-copy`.
The first observation was interrupted (`kirby-copy-action.log`), with the
process handle subsequently missing and a privileged process check
confirming no test remained. The retry was explicitly terminated after
finding its stop condition could end before the required copy checks.
`kirby-copy-checked.log` then failed coverage on Onett: Ness remained on
a high roof outside the approach sequence's reach. None of these runs
is counted as a passing copy test.

The focused fixture now selects Final Destination, disables preliminary
combat, approaches via PAD movement, inhales via B, swallows via B, and
uses the copied neutral special. Its 7,000-submission endpoint occurs
before match expiry and requires an active Kirby plus capture, a nonnull
Ness hat, both copied article kinds and a return to idle.
`kirby-copy-flat.log` completes exit zero without sanitizer errors:
capture at 2627, Ness hat at 2707, copied PK Flash (145) at 2806,
explosion (146) at 2850, and idle at 2894. The opaque final capture was
visually inspected. This covers Ness acquisition/use, not every copy
ability, losing/reacquiring a hat, all hat color variants or copy-cache
entries across resets. Production code and app packages are unchanged
from the prior Final Cutter fixes; this follow-up changes the test runner.


Kirby's copy owner now supports Mario, Dr. Mario and Luigi alongside Ness,
selected explicitly by FighterKind and the matching public symbol. Their
hats and visibility tables are owned together with one projectile each:
Mario fireball (20 scalar bytes, one animation state), Dr. Mario capsule
(20 scalar bytes, six states), and Luigi fireball (16 scalar bytes, one
state). The copy item kinds use the corresponding original scalar schemas.
`kirby-copy-fireballs-tests.log` passes with the Ness regression; the direct
probe validates three hats, eight article animations and three scripts
after source erasure/free, checks scalar values against the archive, rejects
unsupported/mismatched kinds and verifies HSD pool cleanup.

All four owners are connected to the copy preload path with process-lifetime
caching and the original leading Mario table slot preserved. Runtime builds
pass in `kirby-copy-fireballs-hook-build.log`,
`kirby-copy-fireballs-iphoneos-build.log` and
`kirby-copy-fireballs-simulator-build.log`. Mario/Doctor/Luigi copy
acquisition/use still needs directed gameplay tests; current app bundles
have not yet been replaced for this extension. Other copy archives remain
unsupported by the native copy owner.


Directed acquisition/use tests now pass for Mario, Dr. Mario and Luigi
copies. The runner variants `--kirby-copy-mario`, `--kirby-copy-doctor`
and `--kirby-copy-luigi` guide the second controller to the selected
opponent, unlock Doctor/Luigi in the test fixture, use Final Destination
and require capture, the correct hat kind with a model, the corresponding
copied projectile and return to idle. Each completes 7,000 submissions
with exit zero and no sanitizer report:

- `kirby-copy-mario.log`: capture 2627, hat kind 0 at 2707, fireball
  kind 130 at 2805, idle 2831.
- `kirby-copy-doctor.log`: capture 2627, hat kind 21 at 2708, capsule
  kind 131 at 2804, idle 2825.
- `kirby-copy-luigi.log`: capture 2627, hat kind 17 at 2708, fireball
  kind 132 at 2807, idle 2829.

All three opaque final captures were visually inspected and show the
respective cap/head-mirror models. This does not cover losing/reacquiring
hats, every capsule variant, projectile collision responses or multiplayer
copy-cache reset combinations. App packaging passes with the extended
loader in `mac-kirby-fireball-copies-app-build.log`,
`iphoneos-kirby-fireball-copies-app-build.log` and
`iphonesimulator-kirby-fireball-copies-app-build.log`. Device signing and
physical-device runtime/control checks remain pending.


Kirby's native copy-cache reset now clears pointer fields directly. The
retail reset casts the pointer table to s32 and zeros Ft_Kind_Max words;
on a 64-bit host that leaves a partially cleared pointer and subsequent
slots untouched. The regression fills every used slot with a real pointer
and invokes the original reset function. `kirby-cache-reset-before.log`
fails with retained pointer slot 16 (exit 6); after the native-only fix,
`kirby-cache-reset-after.log` passes as CTest `kirby_copy_cache_reset`.
The GameCube reset path is preserved. Native owned asset caches retain
their intended process lifetime; the runtime lookup table is reset.

Runtime builds pass in `kirby-cache-fix-build.log`,
`kirby-cache-iphoneos-build.log` and `kirby-cache-simulator-build.log`.
This regression proves lookup-pointer clearing, not every copied ability
across a full multiplayer rematch sequence.

App packaging also passes with this reset fix: `mac-kirby-cache-app-build.log`,
`iphoneos-kirby-cache-app-build.log`, and
`iphonesimulator-kirby-cache-app-build.log`. Physical iPhone installation
still requires signing/provisioning and device runtime verification.


Fox's copy hat, laser and blaster are now owned and connected. Both
articles have 40-byte float parameter blocks; their state counts are
two and nine. `kirby-copy-fox-tests.log` passes with the earlier copy
owners, after archive disposal and animation/script exercise. The first
live run (`kirby-copy-fox.log`) fails in copy-effect loading: EfKbFx.dat
contains one effect followed by texture payload at 0x20, so inferring
its table length from the first referenced joint reads past the table.
The known one-record boundary for effKirbyFoxDataTable is now explicit.
`kirby-copy-fox-effect-tests.log` validates it and all connected copy
owners.

`--kirby-copy-fox` selects Fox with the second controller and requires
Fox's hat plus both copied article kinds. `kirby-copy-fox-fixed.log`
completes 7,000 submissions, exit zero with no sanitizer report: capture
at 2628, hat kind 1 at 2708, blaster 138 at 2792, laser 136 at 2803, and
idle at 2828. The final opaque capture was inspected and shows the Fox
ear/headset model. This does not establish all rapid-fire, airborne,
reflection or hat-loss paths. Falco remains pending: its public root uses
a different part/costume layout and must not use the simple Fox header.

Runtime builds pass in `kirby-copy-fox-effect-build.log`,
`kirby-copy-fox-effect-iphoneos-build.log` and
`kirby-copy-fox-effect-simulator-build.log`. Updated app packages pass in
`mac-kirby-fox-app-build.log`, `iphoneos-kirby-fox-app-build.log` and
`iphonesimulator-kirby-fox-app-build.log`. Physical-device verification
and signing/provisioning remain pending.


Falco copy is connected to live loading through an owned composite
costume descriptor. It preserves the visibility tables, texture selectors,
replacement mask, shared joint, six costume models/material animations,
and laser/blaster articles. Native attachment uses pointer-sized slots;
removal reads typed bone flags instead of GameCube byte offsets.
`kirby-copy-falco.log` completes 7,000 frames with capture, copy acquisition,
both articles and return to idle. `kirby-copy-falco-lifecycle-fixed.log`
additionally verifies taunt-driven costume removal, reacquisition, both
articles a second time and return to idle, with exit zero. The initial
lifecycle run failed because the controller script faced away from Falco;
the corrected script turns toward the opponent before inhaling.

The latest packaged apps include this integration:
`mac-kirby-falco-app-build.log`, `iphoneos-kirby-falco-app-build.log`, and
`iphonesimulator-kirby-falco-app-build.log`. Physical-device verification
and signing/provisioning remain pending.

The owner is now `melee_kirby_composite_copy_decode(archive, kind)` and
supports Falco and Donkey Kong. Donkey Kong has the same six-costume
layout, a zero replacement mask and no item articles. The
`hsd_kirby_donkey_costumes` test loads the shared model and animates all
six costumes after archive erasure/free, rejects a mismatched character
and costume, and checks HSD pool cleanup. All eight focused Kirby copy
tests pass. macOS, iPhoneOS and Simulator runtime builds pass in the
`kirby-donkey-costumes*build.log` files. Donkey Kong's copy assets are now connected to live loading through the
shared composite owner table. The first run exposed an effect-table
boundary bug; `effKirbyDonkeyDataTable` has two records before payload at
0x30. The decoder now preserves that boundary, and the two-effect
animation test passes with all eight existing copy tests.
`kirby-copy-donkey-fixed.log` completes 7,000 frames with capture at 2628,
costume acquisition at 2708, punch charging at 2799, release at 3032,
and return to idle at 3079. The final opaque image was inspected and shows
the Donkey Kong costume, with the opponent at 26% damage. The extended `kirby-copy-donkey-charge-fixed.log` completes 7,000 frames
with a partial punch at 2851, shield cancellation preserving five charge
swings at 2978, resumed full charge stored at 3098, full punch at 3099,
and return to idle at 3143. The screenshot was inspected; the opponent
finishes at 28%. The first extended run failed its coverage check because
the previous 240-frame delay had already reached full charge; the input
script now uses 60-frame spacing to cover the partial/cancel paths.
Airborne and copy-loss paths remain unverified.
Runtime builds pass in `kirby-donkey-effects*build.log`; updated packages
pass in `mac-kirby-donkey-app-build.log`,
`iphoneos-kirby-donkey-app-build.log`, and
`iphonesimulator-kirby-donkey-app-build.log`.

Requested follow-up after core gameplay is complete: optional MetalFX
upscaling on supported Apple devices, with a user-facing opt-in setting
and native rendering as the default. This feature is not implemented yet.
Expose the setting on macOS, iPhone and iPad when supported, and preserve
the user's choice between launches. Validate image quality and frame pacing
before treating the feature as ready.

Captain Falcon copy now uses the standalone native hat owner, with no
article slots. `hsd_kirby_copy_captain` checks the helmet, visibility,
wrong-kind rejection, source erasure/free and pool cleanup.
`hsd_kirby_captain_effects` checks two effects and their particle bank.
All eleven focused copy tests pass in `kirby-captain-tests.log`.
`kirby-copy-captain.log` completes 7,000 frames with capture at 2627,
helmet acquisition at 2708, grounded Falcon Punch activation at 2792,
and return to idle at 2879. The final opaque image was inspected.
This does not establish punch hit detection, airborne use or copy loss.
Runtime builds pass in `kirby-captain*build.log`. Current app packages
pass in `mac-kirby-captain-app-build.log`,
`iphoneos-kirby-captain-app-build.log`, and
`iphonesimulator-kirby-captain-app-build.log`. Physical-device testing
and the remaining native port work are still pending.

Ganondorf copy now uses the standalone native hat owner with no articles.
The shared helmet test also covers Ganondorf, and its effect test verifies
two effects with a particle bank. All thirteen focused copy tests pass
in `kirby-ganon-tests.log`. `kirby-copy-ganon.log` completes 7,000 frames
with capture at 2540, helmet acquisition at 2617, grounded Warlock Punch
activation at 2701 and return to idle at 2809. The opaque screenshot was
inspected. Punch hit detection, airborne use and copy loss are not yet
verified by this run. Runtime builds pass in `kirby-ganon*build.log`.
The latest app packages pass in `mac-kirby-ganon-app-build.log`,
`iphoneos-kirby-ganon-app-build.log` and
`iphonesimulator-kirby-ganon-app-build.log`. The full port remains incomplete.

Grounded hit detection is now verified for Kirby's copied Falcon Punch
and Warlock Punch. The controller script approaches and turns toward the
opponent, records damage immediately before the special input, and requires
an increase during the corresponding punch state followed by return to idle.
`kirby-copy-captain-hit.log` completes 7,000 frames: Falcon Punch activates
at 2821, hits at 2873 (8% to 35%), and returns to idle at 2931.
`kirby-copy-ganon-hit.log` completes 7,000 frames: Warlock Punch activates
at 2822, hits at 2891 (8% to 38%), and returns to idle at 2952.
Both runs exit zero without an AddressSanitizer error; both opaque final
captures were inspected. Airborne use, shields and copy loss remain
unverified. This change extends test input/coverage only; production app
packages remain the Ganondorf integration builds above.

Marth/Roy copy asset groundwork extends the standalone copy owner with
an owned sword scene and native dynamic-bone descriptors. The sword is
borrowed through hat slot zero; dynamics through slot one. Marth has three
chains rooted at bones 10/4/7; Roy has four at 3/6/9/12. Every chain has two
parameter records. Unsupported collision/selector extensions are rejected.
`hsd_kirby_copy_swords` verifies both models and accessories after source
erasure/free, chain metadata, wrong-kind rejection and HSD pool cleanup.
All fourteen focused tests pass in `kirby-copy-swords-tests.log`; macOS,
iPhoneOS and Simulator runtime builds pass in
`kirby-copy-swords*build.log`. These two copy kinds are not yet connected
to live loading; sword animation, dynamic motion and gameplay remain
unverified. Current packaged apps remain the Ganondorf integration builds.

Marth and Roy copies are now connected to live loading. Their effect
archives each contain two records before payload at 0x30; explicit
boundaries prevent reading payload as more effects. Sixteen focused copy
tests pass in `kirby-swords-effects-tests.log`. Marth's first effect-fixed
run exposed a native union-layout bug: the charge transition read the
copy kind through Game & Watch's `x2238_panicCharge`, selecting Roy's
loop. Both ground/air transition helpers now use `u.kb.hat.kind` on native.
`kirby-copy-marth-kind-fixed.log` completes 7,000 frames with acquisition
at 2708, sword attachment at 2882 (three dynamics), correct charging at
2893, release at 2971 and idle at 3011. `kirby-copy-roy.log` completes
7,000 frames with acquisition at 2708, sword at 2882 (four dynamics),
charging at 2890, release at 2972 and idle at 3012. Both final opaque
captures were inspected. Airborne use, full charge and copy loss still
need direct verification. Runtime builds pass in
`kirby-swords-kind*build.log`; current app packages pass in
`mac-kirby-swords-app-build.log`, `iphoneos-kirby-swords-app-build.log`,
and `iphonesimulator-kirby-swords-app-build.log`.

Zelda copy asset support now extends the standalone owner. Its dynamics
occupy hat slot zero, with no sword or item articles. Three chains root
at bones 9/15/3, each with four parameter records. The decoder owns the
model, visibility and dynamic parameters; `hsd_kirby_copy_zelda` validates
loading after archive erasure/free, chain metadata, wrong-kind rejection
and HSD pool cleanup. All seventeen focused copy tests pass in
`kirby-zelda-data-tests.log`. Runtime builds pass in
`kirby-zelda-data*build.log` for macOS, iPhoneOS and Simulator. Zelda's
copy still needs live-loader connection, effect loading and gameplay
verification. Packaged apps remain the Marth/Roy integration builds.

Zelda copy is connected to live loading. Its two effect records now have
an explicit boundary. Zelda match setup also preloads Sheik's copy, so
the standalone owner now handles Sheik's two needle articles (5/1 states)
and two dynamic chains in slot two. Copied needle scalar schemas match
the original needle schemas. Nineteen focused copy tests pass in
`kirby-zelda-sheik-tests.log`, including six needle animations after source
disposal. Sheik copy gameplay itself has not yet been exercised.
`kirby-copy-zelda-fixed.log` completes 7,000 frames: capture 2628, copy 2708,
Nayru's Love 2882 with three dynamics, reflector enabled 2884, disabled
2923, idle 2941. The final opaque screenshot was inspected. Projectile
reflection, airborne use and copy loss remain unverified. The first
`kirby-copy-zelda.log` failed on the legacy Sheik preload and is not a pass.
Runtime builds pass in `kirby-zelda-sheik*build.log`; current packages pass
in `mac-kirby-zelda-app-build.log`, `iphoneos-kirby-zelda-app-build.log`,
and `iphonesimulator-kirby-zelda-app-build.log`.

Sheik copy gameplay now passes in `kirby-copy-sheik-fixed.log` for 7,000
frames: opponent transforms from Zelda using down-special at 1800,
Kirby captures at 2582, acquires Sheik at 2662, charges at 2783 with two
dynamics and held needle 153, throws needle 152 at 2868, and returns to
idle at 2905. The final opaque capture was inspected. The initial
`kirby-copy-sheik.log` failed during inhalation: captured X scale was
read through `mv.co.guard.x2C`, which no longer aliases
`mv.co.capturekirby.scale.x` on ARM64. Native capture now uses the actual
saved scale field. This is a shared capture-path fix, not a Sheik-only
workaround. Runtime builds pass in `kirby-capture-scale*build.log`;
current app packages pass in `mac-kirby-sheik-app-build.log`,
`iphoneos-kirby-sheik-app-build.log` and
`iphonesimulator-kirby-sheik-app-build.log`. Airborne needle use, charge
cancellation, reflection and copy loss remain unverified.

Peach copy asset support now owns the crown, visibility, Toad and spore
articles. Copied scalar schemas preserve Toad's one integer word and the
spore's four floats. `hsd_kirby_copy_peach` compares all parameter words
against disc data, animates both Toad states after source erasure/free,
and verifies the model-free spore state has its script. The initial test
incorrectly required a spore joint and failed; archive inspection showed
that the spore deliberately has no model or animation tree. All twenty
focused tests pass in `kirby-peach-data-fixed-tests.log`. Runtime builds
pass in `kirby-peach-data*build.log` on macOS, iPhoneOS and Simulator.
Peach copy is not yet connected to live loading; Toad activation and
counter-triggered spores remain unverified. Packaged apps remain the
Sheik integration/capture-scale-fix builds.

Peach copy is connected to live loading. There is no separate Kirby Peach
effect archive (effect slot 45 is empty). `kirby-copy-peach.log` completes
7,000 frames: capture 2627, crown 2707, Toad state/article 134 at 2881,
opponent jab input 2890, counter state 2892, spore article 135 at 2893,
and idle 2956. All interaction uses ordinary controller input. The final
opaque screenshot was inspected and shows Peach at 26% with Kirby at 0%.
Airborne counters, copy loss and other attack/projectile interactions
remain unverified. Runtime builds pass in `kirby-peach-live*build.log`;
current app packages pass in `mac-kirby-peach-app-build.log`,
`iphoneos-kirby-peach-app-build.log` and
`iphonesimulator-kirby-peach-app-build.log`.

Ice Climbers copy asset support now owns the parka, separate hammer
accessory in hat slot one and ice-shot article in slot zero. Copied ice
uses the original 52-byte scalar schema, preserving pad_20 as raw bytes.
`hsd_kirby_copy_ice` loads the parka/hammer and animates the projectile
through frame 120 after archive erasure/free, checks all numeric fields
and raw bytes, rejects the wrong character and checks HSD pool cleanup.
The initial test compared pad_20 as a number; its corrected byte comparison
passes. All twenty-one focused tests pass in
`kirby-ice-data-fixed-tests.log`. Runtime builds pass in
`kirby-ice-data*build.log` on macOS, iPhoneOS and Simulator. Live copy loading,
hammer motion and Ice Shot gameplay remain unverified; current packages
remain the Peach copy integration builds.

Ice Climbers copy is connected to live loading. Its effect table has a
particle bank and one empty model record, verified by
`hsd_kirby_ice_effects`. All twenty-two focused tests pass in
`kirby-ice-live-tests.log`. `kirby-copy-ice.log` completes 7,000 frames
with both Climbers present: capture at 2537, parka at 2617, Ice Shot and
hammer at 2731, copied ice article 133 at 2734, and idle at 2785. The final
opaque screenshot was inspected. Airborne shots, reflection and copy loss
remain unverified. Runtime builds pass in `kirby-ice-live*build.log`;
current packages pass in `mac-kirby-ice-app-build.log`,
`iphoneos-kirby-ice-app-build.log`, and
`iphonesimulator-kirby-ice-app-build.log`.

Samus copy asset groundwork now owns the helmet, visibility and Charge Shot
article. Its copied 32-byte schema preserves the integer word alongside
seven floats. `hsd_kirby_copy_samus` checks all eight parameter words,
source erasure/free, helmet loading, nine article animation states and HSD
pool cleanup. All twenty-three focused copy tests pass in
`kirby-samus-data-tests.log`. Runtime builds pass on macOS, iPhoneOS and
Simulator in `kirby-samus-data*build.log`. This does not yet connect Samus
copy to live loading or verify charging/firing gameplay. App packages
remain the Ice Climbers copy integration builds.

Samus copy live integration uncovered a separate effect-bank issue before
loader activation. Extracted `EfKbSs.dat` has one model record (lifetime
32), a particle command bank at 0x20 (start 34000, eleven commands), and
seven texture groups at 0x460. The first group is a 64x64 C8 image with one
palette entry, but its palette offset is 0x80a8812a, outside the 0x9f1c-byte
remaining texture span. Temporary decoder diagnostics identify the bounds
check in particle_bank.c, not model-table discovery, as the failure.
`kirby-samus-effects-test.log` and `kirby-samus-effects-debug.log` record
that rejection. The first two commands reference texture group zero;
whether those commands are used as particles versus model effects still
requires tracing. Do not replace this palette with invented colors or
weaken its bounds check. Diagnostics were removed after isolation; Samus
copy remains disconnected pending a faithful resolution of the resource.

Samus copy is now connected to live loading. The palette investigation
found that its first C8 image matches the base Samus image exactly, but
no replacement palette is used. Instead, the native particle owner keeps
out-of-range palette entries as an explicit unresolved marker; image
bounds checks still reject invalid images. The sole particle TLUT render
path asserts that a selected palette is resolved before passing it to GX.
This permits unused retail entries without fabricating colors or forming
out-of-range host pointers. Regression coverage checks the marker, valid
images and source NULL distinction in `test_particle_bank.c`.

The initial temporary null-palette diagnostic completed 7,000 frames
without selecting the bad palette. The final production run
`kirby-copy-samus.log` also exits zero after 7,000 frames: capture 2627,
helmet 2708, Charge Shot article 151 at 2833, charging 2834, firing 3075,
and idle 3105. The inspected opaque screenshot shows Samus at 33% and
Kirby at 0%. Airborne shots, explicit full-charge/cancel coverage and copy
loss remain unverified. The screenshot still shows the outstanding stage
rendering artifacts; this test does not establish overall visual fidelity.
All twenty-four focused tests pass in `kirby-samus-live-tests.log` and
palette unit tests pass in `kirby-samus-palette-unit-tests.log`. Runtime
builds pass in `kirby-samus-live*build.log`; app packaging passes in
`mac-kirby-samus-app-build.log`, `iphoneos-kirby-samus-app-build.log`, and
`iphonesimulator-kirby-samus-app-build.log`.

Samus copy charge lifecycle now has stronger normal-input coverage.
`kirby-copy-samus-charge.log` exits zero after 7,000 frames: capture 2628,
helmet 2708, charging 2834, partial shot 2881 with charge level 2,
cancellation 3061 retaining level 2, full charge stored 3228, full shot
3243, idle 3273. The test requires each milestone and rejects a supposed
partial shot at full charge. No unresolved-palette assertion fired. The
opaque final capture was inspected. Only the startup fixture changed;
production packages remain the preceding Samus integration builds.
Airborne shots, copy loss and broader match coverage remain unverified.

Visual triage correction: the source texture atlas again confirms the
platform grain is original artwork; bright outlines alone also do not
prove corruption. The earlier screenshot wording about stage artifacts
should not be read as a diagnosed defect. Whole-scene visual fidelity
remains unverified and requires a controlled reference comparison before
changing the renderer on that basis.

Pikachu/Pichu copy asset ownership now supports both hats and both Thunder
Jolt articles per character. Ground attributes are four floats; air uses
one integer word. `hsd_kirby_copy_electric` compares every parameter word,
rejects the opposite character archive, erases/frees source bytes, loads
both hats, animates both air projectiles and verifies model-free ground
states plus HSD pool cleanup. Its initial assumption of a ground model
failed and was corrected against both archives. All twenty-five focused
copy tests pass in `kirby-electric-data-fixed-tests.log`. Runtime builds
pass in `kirby-electric-data*build.log` for macOS, iPhoneOS and Simulator.
These copies are not yet connected to live loading; effect loading,
projectile transitions and copied Pichu recoil remain unverified.
Packaged apps remain the Samus copy integration builds.

Pikachu/Pichu copy live loading is wired, and their shared EfKbPk effect
archive passes decoding. The first Pikachu live run exposed missing hat
dynamics at root+20 (slot two): both archives have three chains. The owner
now decodes those chains through the existing native dynamics path and
`hsd_kirby_copy_electric` requires them after source disposal. All twenty-six
focused tests pass in `kirby-electric-dynamics-tests.log`; all three Apple
runtime builds pass in `kirby-electric-dynamics*build.log`.

Live Thunder Jolt coverage remains incomplete. `kirby-copy-pikachu.log`
crashed during acquisition before the dynamics fix. The fixed run acquired
the hat but exited 6 without projectile coverage. Shortening the turn input
in `kirby-copy-pikachu-spacing.log` exposed that backing away to a 50-unit
gap walked Kirby off the stage and lost the copy; later neutral specials
were base inhalation. The fixture now requests a 25-unit gap and builds in
`kirby-electric-spacing-safe-build.log`, but that revised run is not yet
executed. No Pichu live run has passed. Packages remain the last verified
Samus integration packages. Do not infer copy gameplay support from the
passing archive tests or acquired-hat milestone alone.

Pikachu and Pichu copied Thunder Jolt now pass separate 7,000-frame live
runs with the 25-unit spacing fixture. `kirby-copy-pikachu-safe.log` exits
zero: capture 2628, hat 2708, ground article 147 at 2958, air article 148
at 2959, idle 2993. `kirby-copy-pichu.log` exits zero: capture 2627, hat
2708, ground article 149 at 2959, air article 150 at 2961, idle 2999.
Both opaque screenshots were inspected and show the correct hats. The
observed order is ground then air; this is evidence of both forms, not a
claim that a separately initiated airborne special was tested. Explicit
airborne input, copy loss and full-match variants remain unverified.

All twenty-six focused tests remain passing in
`kirby-electric-dynamics-tests.log`, and all Apple runtime builds passed
before packaging. Updated packages pass in `mac-kirby-electric-app-build.log`,
`iphoneos-kirby-electric-app-build.log`, and
`iphonesimulator-kirby-electric-app-build.log`. These are local packages;
physical iPhone installation and hardware controller testing remain pending.

Bowser copy asset ownership now supports the hat, its one dynamics chain
in slot one, and the model-free Fire Breath article in slot zero. The
copied flame schema contains six floats. `hsd_kirby_copy_koopa` checks
all six parameter words, the dynamics owner, hat loading, absent flame
model/animation trees and present flame script after source erasure/free,
plus HSD pool cleanup and wrong-kind rejection. All twenty-seven focused
copy tests pass in `kirby-koopa-data-tests.log`. Runtime builds pass in
`kirby-koopa-data*build.log` on macOS, iPhoneOS and Simulator. Bowser copy
is not yet connected to live loading; sustained breath, angle control,
effects and cooldown remain unverified. Packages remain the Pikachu/Pichu
copy integration builds. Giga Bowser copy is not covered by this owner.

Bowser copy is connected to native loading, including EfKbKp's effect
model and particle bank. All twenty-eight focused tests pass in
`kirby-koopa-live-tests.log`. `kirby-copy-koopa.log` exits zero at 7,000
frames: capture 2627, hat 2707, breath state, flame article 154 and hit
at 2840, fuel recovery and idle 3033. Input holds B for 180 frames. The
fixture observes fuel below its maximum during breath and rising from
199.0 to 199.4 after release; it does not yet prove complete depletion or
recharge timing. The inspected opaque capture shows Kirby at 0% and
Bowser at 42%. Airborne breath, angle control, copy loss, full depletion
and Giga Bowser copy remain unverified. Apple runtime builds pass in
`kirby-koopa-live*build.log`; updated packages pass in
`mac-kirby-koopa-app-build.log`, `iphoneos-kirby-koopa-app-build.log`, and
`iphonesimulator-kirby-koopa-app-build.log`.

Mewtwo copy asset ownership now extends the composite owner used by Falco
and Donkey Kong. It owns the shared model, mask 0x7f0, six costume models
and material animations, visibility/texture selectors, and one Shadow Ball
article (temporarily exposed through the descriptor's first article field,
`laser`). The copied schema has twelve words, with word eight an integer.
`hsd_kirby_mewtwo_costumes` checks all parameter words, animates all ten
Shadow Ball states and all six costume variants after source disposal,
rejects wrong archives/slots and verifies HSD pool cleanup. All twenty-nine
focused tests pass in `kirby-mewtwo-data-tests.log`; runtime builds pass
in `kirby-mewtwo-data*build.log` on macOS, iPhoneOS and Simulator. Live
composite loading, item registration, Shadow Ball gameplay and copy loss
are not connected or verified yet. Packages remain Bowser copy builds.

Mewtwo composite live loading and native Shadow Ball registration are now
connected. The initial `kirby-copy-mewtwo.log` run failed on acquisition
in ftCo_8009DB50: the composite contains a dynamics chain at root+28,
accessed through legacy hat slot four. The composite owner now owns that
chain and the callback accesses its typed dynamics field on native builds.
The chain's indices refer to Kirby's 46-part base skeleton. The costume
regression now requires the dynamics owner after source disposal.
All twenty-nine focused tests pass in `kirby-mewtwo-dynamics-tests.log`,
and all Apple runtime builds pass in `kirby-mewtwo-dynamics*build.log`.
The corrected live run remains pending; model acquisition, Shadow Ball
charging/release and copy loss must not be claimed verified. Packaged apps
remain the last verified Bowser copy builds.

Mewtwo copy now passes the corrected live run. `kirby-copy-mewtwo-fixed.log`
exits zero after 7,000 frames: capture 2627, composite acquisition 2708,
Shadow Ball article 144 at 2834, charging 2839, release 3061 and idle
3085. The final opaque screenshot was inspected and shows the copied
Mewtwo model, Kirby at 0% and Mewtwo at 33%. This covers one grounded
charge/release sequence; partial/full charge distinction, cancellation,
airborne input, copy loss and all costume gameplay remain unverified.
The twenty-nine focused regressions passed before this run, and all Apple
runtime builds passed. Updated package builds succeed in
`mac-kirby-mewtwo-app-build.log`, `iphoneos-kirby-mewtwo-app-build.log`, and
`iphonesimulator-kirby-mewtwo-app-build.log`.

Link/Young Link copy asset ownership now supports both hats, each one's
single dynamics chain in slot two, arrows and bows in slots zero and one.
Copied arrows use nine float words; bows use two integer words. The new
`hsd_kirby_copy_links` checks every parameter word, opposite-kind rejection,
source erasure/free, both hats, dynamics presence and all fourteen item
animation states, with HSD pool cleanup. All thirty focused tests pass in
`kirby-links-data-tests.log`; Apple runtime builds pass in
`kirby-links-data*build.log`. These copies are not yet connected to live
loading. Charging, firing, effects, airborne input and copy loss remain
unverified. Packages remain the Mewtwo copy integration builds.

Link/Young Link copy live loading is connected. The first Link run
`kirby-copy-link.log` acquired the hat, bow and arrow and entered charging,
but failed on release in itLinkArrow_802A850C: the copied arrow had only
its 36-byte numeric prefix, omitting the native pointer tail. The article
owner now routes copied arrows through the same two-model descriptor
extension as normal Link/Young Link arrows. The focused test loads both
extra model descriptors after source disposal. All thirty focused tests
pass in `kirby-links-arrow-tests.log`; all Apple runtime builds pass in
`kirby-links-arrow*build.log`. Corrected live Link and Young Link runs
remain pending. Packaged apps remain Mewtwo copy integration builds.

Both corrected Link copy gameplay runs now pass 7,000 frames.
`kirby-copy-link-fixed.log`: capture 2628, hat 2708, bow 142 at 2882,
arrow 140 at 2886, charge 2941, release 2972, idle 2996.
`kirby-copy-young-link.log`: capture 2627, hat 2707, bow 143 at 2882,
arrow 141 at 2885, charge 2927, release 2972, idle 2996. Both opaque
screenshots were inspected. These runs cover one grounded charge/release
per copy; airborne input, charge extremes, reflection and copy loss remain
unverified. The thirty focused tests and Apple runtime builds passed with
the shared arrow descriptor fix. Updated packages pass in
`mac-kirby-links-app-build.log`, `iphoneos-kirby-links-app-build.log`, and
`iphonesimulator-kirby-links-app-build.log`.

Jigglypuff copy asset support now extends the composite owner with six
costume variants, shared geometry, replacement mask 0xf and one dynamics
chain at root+24. It has no item articles. `hsd_kirby_purin_costumes` checks
all six model/material variants, visibility, texture selectors, mask,
dynamics presence, wrong archive/slot rejection and source disposal with
HSD pool cleanup. All thirty-one focused tests pass in
`kirby-purin-data-tests.log`; runtime builds pass in
`kirby-purin-data*build.log` on macOS, iPhoneOS and Simulator. Live copy
loading and the dynamics callback still need integration; Rollout charge,
release, steering, hit response and copy loss remain unverified. Packages
remain the Link/Young Link copy integration builds.

Jigglypuff composite live loading and typed dynamics setup are connected;
Apple runtime builds pass in `kirby-purin-live*build.log`. The first live
run `kirby-copy-purin.log` exits zero but is NOT sufficient passing evidence:
charge 2837, hit 2912, later release observation 5042, idle 5518, and the
final screenshot shows Kirby without the copy after death. This exposed
a fixture allowing cross-attempt milestones and accepting idle after loss.
Completion now requires the target copy still present. Rollout's hit
callback accepts only released rolling states (PrSpecialN1/Turn/AirN/N0),
so the hit observation records release too, accommodating same-frame
release/contact. This should stop further attempts after the first hit
and actual recovery. `kirby-purin-coverage-build.log` passes; its corrected
live run remains pending. Packages remain verified Link copy builds.

The stricter Jigglypuff copy fixture completes in
`kirby-copy-purin-coverage.log` with exit zero at 7,000 frames: capture
2627, composite acquisition 2707, Rollout charge 2836, released hit 2912,
and idle 2993 while still holding the copy. The input driver stops further
special attempts after that first recovery. This verifies that sequence,
not a whole-match survival claim: the inspected final capture shows Kirby
at 25%, knocked beyond the right edge later in the run, and Jigglypuff at
18%. Steering, airborne starts, copy loss and recovery from later hazards
remain unverified. All thirty-one focused regressions and Apple runtime
builds passed before this run. Updated packages pass in
`mac-kirby-purin-app-build.log`, `iphoneos-kirby-purin-app-build.log`, and
`iphonesimulator-kirby-purin-app-build.log`.

Yoshi copy asset ownership now includes the hat, captured-fighter model,
four separate joint-animation trees and the Egg Lay article in slot five.
The copied Egg Lay article uses the existing no-special-attributes path.
The first test exposed that KirbyHatStruct declares only five tail slots,
while Yoshi needs six and Game & Watch needs seven. Native builds now
allocate seven slots; the original GameCube declaration remains unchanged.
This prevents Yoshi's sixth slot from overwriting the native owner's next
field. `hsd_kirby_copy_yoshi` checks source disposal, all four animation
bindings, both model roles, egg-model loading and HSD pool cleanup.
All thirty-two focused tests pass in `kirby-yoshi-slots-tests.log`; Apple
runtime builds pass in `kirby-yoshi-slots*build.log`. Yoshi copy is not yet
connected to live loading. Capture, egg transformation, escape and copy
loss remain unverified. Packages remain Jigglypuff copy builds.

Yoshi copy live loading is connected; all Apple runtime builds pass in
`kirby-yoshi-live*build.log`. `kirby-copy-yoshi.log` acquired the hat and
repeatedly captured the opponent but exited 6 because the fixture watched
normal YoshiEgg instead of KirbyYoshiEgg (state 332). The original copied
egg callback builds a fighter accessory rather than a live item, so item
spawn coverage was also an invalid expectation. The corrected fixture
requires state 332 with accessory and hidden victim, followed by visible
Wait, and stops further Egg Lay attempts once captured. Build passes in
`kirby-yoshi-capture-build.log`; the corrected live run remains pending.
Packages remain Jigglypuff copy builds. This correction is based on the
original ftCo_KirbyYoshiEgg callback and observed states, not relaxed
capture/escape requirements.

The corrected Yoshi copy capture run now passes. `kirby-copy-yoshi-capture.log`
exits zero after 7,000 frames: initial inhale capture 2627, hat 2707,
copied Egg Lay capture 2865, victim escape and both fighters idle 3146,
with Kirby retaining the copy at recovery. The test requires a hidden
victim and accessory while in KirbyYoshiEgg, then visible Wait. The final
opaque screenshot was inspected and shows Kirby at 0%, Yoshi at 16%,
and the copied hat. This covers one grounded capture/timeout escape;
airborne capture, mashing, damage to the egg and copy loss remain unverified.
The thirty-two focused regressions and Apple runtime builds passed before
this run. Updated packages pass in `mac-kirby-yoshi-app-build.log`,
`iphoneos-kirby-yoshi-app-build.log`, and
`iphonesimulator-kirby-yoshi-app-build.log`.

Mr. Game & Watch copy groundwork now supports Chef food and pan articles
through the native Game & Watch item decoder, retaining outline-part
ownership and Chef's 28 numeric parameters. `hsd_kirby_copy_chef_articles`
checks both models and both food animation states after source disposal,
parameter equality and HSD pool cleanup. All thirty-three focused copy
tests pass in `kirby-chef-articles-tests.log`; Apple runtime builds pass in
`kirby-chef-articles*build.log`. The composite copy itself remains
unsupported: it has no shared model, a separate outline lookup at root+24,
color metadata at root+28 (a valid offset-zero reference), and articles at
root+32/+36. Costume ownership, outline/color integration and live Chef
still require work. Packaged apps remain Yoshi copy builds.

Game & Watch copy costume ownership now passes all six slots using the
shared `PlKbNrCpGw.dat` archive. The composite descriptor owns the outline
lookup, depth and RGBA metadata, including the valid offset-zero color
reference, and both Chef article descriptors. The source-disposal test
checks animation, visibility, outline indices, colors and pool cleanup.
All thirty-four focused tests pass in `kirby-gamewatch-data-tests.log`.
macOS, iPhoneOS and Simulator runtime builds pass in
`kirby-gamewatch-data*build.log`; both mobile targets were subsequently
confirmed up to date with successful no-op builds. The live loader,
outline/color callbacks and Chef registration still need integration;
this checkpoint does not establish playable copied Chef. Packaged apps
remain the Yoshi copy builds. MetalFX remains the requested follow-up
after core gameplay, disabled by default with a saved opt-in preference.


Game & Watch copy now runs through the native composite loader, including
outline/fill colors and Chef food/pan registration. The original item color
callbacks use misleading pointer-typed fields for four RGBA bytes; native
callbacks copy exactly four bytes rather than writing an eight-byte pointer.
The first live run (`kirby-copy-gamewatch.log`) crashed in outline visibility
at acquisition. `kirby-copy-gamewatch-outline.log` confirmed base Kirby has
two model groups while the copied outline provides one. Native outline
storage now includes empty remaining groups through the visibility limit,
so traversal cannot read adjacent color metadata as another lookup. The
source-disposal test verifies those entries remain empty.

`kirby-copy-gamewatch-fixed.log` exits zero after 7,000 windowless Metal
frames: inhale capture 2627, copy 2707, pan 156 at 2822, food 155 at 2839,
and idle with the copy retained at 2938. The final opaque screenshot was
inspected: Kirby has the black Game & Watch appearance at 25%, opposite
Game & Watch at 21%. This verifies grounded acquisition, Chef articles and
recovery; airborne Chef, all food variants and copy discard remain unverified.
All thirty-four focused regressions pass in
`kirby-gamewatch-outline-fixed-tests.log`; all three runtime builds pass in
`kirby-gamewatch-outline-fixed*build.log`. Updated app packages pass in
`mac-kirby-gamewatch-app-build.log`, `iphoneos-kirby-gamewatch-app-build.log`
and `iphonesimulator-kirby-gamewatch-app-build.log`. Device and interactive
Mac verification, remaining stages/modes and the opt-in MetalFX follow-up
remain incomplete.


Dream Land stage archive groundwork now reuses the owned stage models,
collision, environment, quake, scripts and particle bank infrastructure.
Its `yakumono_param` is a 52-byte wind/timing record, not the color-script
pointer table used by Battlefield and Final Destination. The native owner
converts four signed halfwords, two integer words and nine float words,
retaining all bits in native byte order. `hsd_dreamland_archive` verifies
eight stage models, ten public symbols, exact parameter values, source
disposal and HSD pool cleanup; `hsd_dreamland_stage_playback` exercises the
stage model animations. All twelve stage regressions pass in
`dreamland-archive-tests.log`. macOS, iPhoneOS and Simulator runtime builds
pass in `dreamland-archive*build.log`. The live loader and stage-selection
fixture are not connected yet, so this is asset support, not verified Dream
Land gameplay. Packaged apps remain the Game & Watch copy builds.


Dream Land is now connected to the live native stage loader. The new
`--dreamland` windowless fixture unlocks only the stage in memory, guides
the existing selection cursor and requires a live two-fighter match on
OldPupupu at frame 2800. `dreamland-live.log` exits zero, with Fox at 0%
and Ness at 15%; `dreamland-live.png` was inspected and shows the stage,
platforms, Whispy, both fighters and HUD. This is a startup/basic combat
check, not verification of a full wind cycle, background character cycles,
match completion or rematching. The original stage callbacks remain in use.
All three runtime builds pass in `dreamland-live*build.log`. Updated app
packages pass in `mac-dreamland-app-build.log`,
`iphoneos-dreamland-app-build.log` and
`iphonesimulator-dreamland-app-build.log`. Earlier twelve archive/playback
regressions passed before live integration. Physical device testing and
interactive Mac testing remain outstanding, as do other stages/modes and
opt-in MetalFX.


Dream Land wind and rematch coverage now pass. `dreamland-wind.log` exits
zero after 6,500 frames: wind activates at 2765, the original fighter wind
query returns x=-0.2 for Ness at that frame, and wind stops at 3039.
The fixture observes the live stage state and calls the original aggregate
wind-offset query for actual fighters; it does not force wind state,
fighter positions or hazard timing. This verifies one direction and cycle,
not an exhaustive physics/timing comparison. The final screenshot was
inspected and shows Fox at 0% and Ness at 15% on Dream Land.

`dreamland-rematch.log` exits zero after 6,000 frames: results at 2293,
character select at 4014, stage select at 4403 and a second Dream Land match
at 4576. This fixture shortens the first match timer to five seconds and
uses normal menu inputs afterward. The second match remains active at 6000;
its screenshot was inspected. Builds pass in `dreamland-wind-build.log`
and `dreamland-rematch-build.log`. Only probe coverage changed in this
checkpoint; packaged apps remain the verified Dream Land builds. Other
wind directions, background character cycles, physical devices, remaining
stages/modes and MetalFX still require work.


Fountain of Dreams archive support now owns its 21 scalar hazard words
(20 floats and one integer) alongside the shared stage assets. Initial
archive/playback tests in `fountain-archive-tests.log` exposed unsupported
RGB light animation tracks in models 1 and 3. The shared environment owner
now decodes and owns light color AObj/FObj tracks (types 9–12), retaining
existing world-position animation support and rejecting unsupported track
kinds or linked light-animation records. The initial color fix passed model
playback but archive loading still rejected independent particle-light
animations; the archive owner now preserves those owned animation records
when no canonical model animation has the same source offset.

All sixteen stage regressions pass in `fountain-color-fixed-tests.log`,
including five Fountain models, exact scalar parameter conversion after
source disposal, model animation playback and changing light colors in both
models 1 and 3 over 600 frames, with HSD pool cleanup. Runtime builds pass
for macOS, iPhoneOS and Simulator in `fountain-color-fixed*build.log`.
Fountain is not connected to the live stage loader yet; moving platforms,
reflections and actual matches remain unverified. Packaged apps remain
Dream Land builds. The broader port and opt-in MetalFX remain incomplete.


Fountain is now connected to live stage selection and the native archive
loader. Scene material images are shared by source descriptor offset, and
stage public image lookup returns the descriptor referenced by the water
material. The Fountain archive test verifies this identity for model 3
after source disposal. Initial `fountain-live.log` crashed calling a heap
address: `IzumiUnkCC` used retail byte padding to write Ground.x18, which
instead overwrote Ground.xC_callback on ARM64. The native branch now uses
GET_GROUND(...)->x18. The first identity test also checked the wrong model
(1 instead of 3); that test was corrected from the stage callback table.

`fountain-fixed.log` exits zero at 2,800 frames with Fox and Ness active on
Fountain. The opaque screenshot was inspected: fighters, platforms,
lighting and water are visible. The water reflection looks questionable
(possible stale scene/HUD imagery), so reflection fidelity remains open.
The next investigation should trace grIzumi_801CCEA0's reflection pass and
lb_800122C8/GX texture-copy behavior. Moving-platform collision, extended
play and rematch still need explicit checks; this is a live startup check.
All sixteen stage regressions pass in `fountain-callback-fixed-tests.log`,
and all Apple runtime builds pass in `fountain-callback-fixed*build.log`.
Updated packages pass in `mac-fountain-app-build.log`,
`iphoneos-fountain-app-build.log` and
`iphonesimulator-fountain-app-build.log`. The broader port, device tests,
remaining stages/modes and opt-in MetalFX remain incomplete.


Fountain reflection diagnostics: the windowless probe now supports
`MELEE_STARTUP_REFLECTION=1` to export its unique 80x60 copy texture at the
final gameplay capture (frame >=2800), with 256-byte-aligned readback rows
and opaque RGB output. Early menu captures remain ordinary frame captures.
The first diagnostic (`fountain-reflection.log`) exited 5 at frame 1320
because it queried the texture before stage creation; that scope error was
corrected. `fountain-reflection-fixed.log` exits zero at 2800 and exports
`fountain-reflection.png`. Direct inspection shows reflected stage geometry
against white, without visible HUD imagery. This narrows, but does not
resolve, the questionable water appearance: investigate projective texture
mapping/sampling and capture a matching full frame before concluding that
the copy is wrong. No Aurora backend behavior was changed. Build passes in
`fountain-reflection-capture-fixed-build.log`; packaged apps remain the
Fountain callback-fix builds. Reflection fidelity remains unverified.


Paired reflection diagnostic now preserves the full framebuffer at OUTPUT
and additionally writes OUTPUT.reflection.png when
MELEE_STARTUP_REFLECTION=1 at the final gameplay capture. This avoids
comparing frames from different runs. `fountain-paired.log` exits zero at
2800, producing `fountain-paired.png` and its reflection companion; both
were inspected. A temporary draw diagnostic observed the water image and
reflection image pointers equal, 80x60, with no descriptor change during
the run. That diagnostic was removed afterward. The direct reflection
contains stage geometry over white, whereas the displayed water remains
inconsistent with it. Material descriptor replacement is therefore not
supported as the cause; next trace renderer texture binding and draw state,
including which copy handle each water draw samples. This is diagnostic
progress, not a rendering fix. `fountain-paired-build.log` passes; packaged
apps remain the Fountain callback-fix builds.


Water binding diagnostics (`fountain-binding.log` and
`fountain-bindings-all.log`) both complete 2,800-frame runs. At reflection
copy revisions 1, 501 and 1001, water slot 1 binds exactly the copy map's
80x60 GPU texture handle. Slot 0 is a static 256x256 CMPR image and is not
an EFB copy. Archive traversal identifies that image as
GrdIzumiNwatera_CMPR_image at data offset 0x1a400. Its decoded diagnostic
image, `fountain-retail-water.png`, was inspected and contains water-swirl
artwork without HUD imagery. Neither descriptor replacement nor binding a
wrong-sized cached texture is supported by these observations. Remaining
investigation: TEV/texture-coordinate combination and render/copy execution
ordering; reflection fidelity is still unresolved. Temporary Aurora gx.cpp
logging was restored away from the saved pre-diagnostic source, and
`fountain-binding-clean-build.log` confirms the clean probe builds. No
production renderer behavior was changed in this checkpoint. App packages
remain the Fountain callback-fix builds.


Reflection assessment correction: `fountain-no-hud.log` completes 2800
frames using the fixture-only MELEE_STARTUP_NO_HUD=1 option, which calls the
original HUD-hide function at frame 1800. The paired framebuffer and
reflection captures were produced; the full frame was inspected. The large
circular graphic previously described as possible HUD imagery inside the
water disappears with the HUD itself. That description conflated an actual
HUD overlay with the water underneath; there is no demonstrated stale-HUD
reflection bug. The water remains coarse, but its source reflection is
80x60, and this observation alone is insufficient to establish incorrect
rendering. Descriptor identity, sampled texture handles and advancing copy
revisions have been verified. A controlled retail reference comparison is
still needed before claiming visual fidelity or changing the renderer.
Continue moving-platform gameplay and rematch checks rather than treating
this unproven visual hypothesis as a blocker. The no-HUD switch affects only
the windowless Fountain fixture; default gameplay/HUD behavior is unchanged.
Build passes in `fountain-no-hud-build.log`. App packages remain the prior
Fountain builds, and the overall native port remains incomplete.


Fountain rematch checkpoint: `--fountain-rematch` now exercises the shortened
match timer and results/character-select/stage-select return path. Two
initial runs (`fountain-rematch.log`, `fountain-rematch-delayed.log`) reached
Sudden Death and results but failed to return to gameplay: the fixture only
sent one two-player Start pulse after results, which first opens the score
details. The fixture now allows another pulse for both players before
starting the next match. No production results logic changed.
`fountain-rematch-confirm.log` exits zero at 8,000 frames: results at 2,269,
character select at 5,614, stage select at 5,803, second match at 6,051,
and two live fighters at the final capture. This passing run did not enter
Sudden Death, so the delayed Sudden Death return remains unverified.
`fountain-rematch-confirm.png` was visually inspected. All 16 stage-focused
archive/playback checks pass in `fountain-rematch-stage-tests.log`, and the
fixture build passes in `fountain-rematch-confirm-build.log`.

`--fountain-platforms` observes both side-platform joints through the live
stage object and requires finite heights with at least one unit of change
on each platform. `fountain-platforms.log` exits zero at 6,500 frames:
platform 1 moves from 28 to 27 at frame 3,500; platform 0 moves from 20 to
19 at frame 4,236. Two fighters remain in the active match at the final
capture; `fountain-platforms.png` was inspected. This check observes joint
motion only; fighter carrying/collision and retail visual fidelity remain
unverified. These changes affect the windowless fixture only. The Mac,
iPhoneOS and Simulator packages remain the Fountain callback-fix builds.
Opt-in MetalFX remains pending until core gameplay is working; the broader
native port is not complete.


Yoshi’s Story is now connected through `/GrSt.dat`. The shared owned stage
archive decoder handles its four models, nine scalar hazard parameters and
one stage item entry (Shy Guy / Heiho). The Shy Guy article owns three
animation records, a damage-threshold pointer target (15), and six scalar
attributes. Native physics accesses these through a typed attribute block
so ARM64 pointer width cannot shift the speed values. Retail expressions
remain equivalent through a macro, with the native pointer path guarded.

`hsd_story_archive` releases the source bytes before checking native symbols,
parameters, the Shy Guy descriptor and owned pointer target, then destroys
the archive and checks HSD allocation cleanup. `hsd_story_stage_playback`
also passes. Its first run exposed a test-only material walker that treated
spline joints as DObj lists; the walker now branches on the joint flags.
All 18 stage checks pass in `story-stage-fixed-tests.log`; all 34 focused
Kirby/copy regression checks pass in `story-copy-regression-tests.log`.

The windowless `--yoshis-story` run exits zero at 2,800 frames in
`story-live.log`. `story-live.png` was inspected: Ness is on stage at 4%
and Fox is in a KO/death state. This verifies stage loading and active VS,
not complete stage behavior. Shy Guy spawning/flight, Randall’s motion and
fighter carrying/collision, full match and rematch remain unverified.
No ASan/UBSan report appears in the passing live log.

Mac, iPhoneOS and Simulator packages rebuild successfully with this support
(`mac-story-app-build.log`, `iphoneos-story-app-build.log`,
`iphonesimulator-story-app-build.log`). These are build checks; the new
stage has not been played through the app UI or on a physical device.
The iPhoneOS package remains unsigned and needs development provisioning.
The full native port and disabled-by-default, opt-in MetalFX are incomplete.


Yoshi’s Story extended hazard checkpoint: `--yoshis-story-hazards` observes
normal game behavior without overriding spawn timers or moving actors.
`story-hazards.log` exits zero at 6,500 frames. Randall’s world position
changes from (-95.370, -38.500) to (-85.080, -38.500) at frame 1,642;
a Shy Guy moves from x=304 to 303.250 at frame 1,734 with velocity -0.750.
The fixture compares consecutive observations of the live item, checks
finite coordinates/velocity, and requires both behaviors. Both fighters
remain active at the final capture; `story-hazards.png` was inspected.
This establishes natural spawning/flight and cloud motion, not Shy Guy hit
reactions or fighter carrying/collision on Randall. Fixture builds pass in
`story-hazards-build.log` and `story-rematch-build.log`. Production app
packages remain the prior Yoshi’s Story builds.

`--yoshis-story-rematch` also passes (`story-rematch.log`, exit zero at
6,000 frames). The shortened first match reaches results at frame 2,271,
returns to character select at 4,014, stage select at 4,403, and a second
active Yoshi’s Story match at 4,613. Both fighters remain active at the
final capture. `story-rematch.png` was inspected. No ASan/UBSan report
appears in either passing live log. The test still uses a shortened first
match and scripted controller input; it does not establish full-match UI,
physical controller, touch, or device coverage. Overall port completion
and opt-in MetalFX remain outstanding.


Latest Simulator UI checkpoint: `ios-control-session-story.log` and
`ios-control-session-story.xcresult` report one XCTest passed in 361.267
seconds using the current Yoshi’s Story runtime on the iPhone 17 Pro
Simulator (iOS 27). The existing opt-in file-driven XCTest controlled the
app’s onscreen controls. It selected Fox, enabled CPU Samus, selected
Yoshi’s Story, ran the full unshortened two-minute match, paused/resumed,
reached results (Fox -5, Samus +5), returned to character selection, and
started a second Yoshi’s Story match. The final screenshot shows active
combat and damage after the reload. The session exited cleanly and the
Simulator, initially shut down, was confirmed already shut down afterward.

Inspected evidence in `native/build/ios-control-session-story/`:
`frame-6.png` ready-to-fight; `frame-9.png` stage selection;
`frame-11.png` active combat with Shy Guy; `frame-16.png` paused;
`frame-19.png` resumed gameplay; `frame-25.png` final results;
`frame-27.png` character-select return; `frame-32.png` second active match.
The session delivered A/B/X/Y/L/Start and stick input; these screenshots
verify navigation, gameplay continuity and pause/rematch, not every
individual move’s response or timing. Tests assert app liveness; the
screenshots were separately inspected to verify the actual scenes.

Audio was intentionally muted (`MELEE_TEST_SILENT=1`). This is Simulator
single-touch evidence, not physical iPhone/iPad, simultaneous multitouch,
Bluetooth-controller, or Mac GUI evidence. App packages remain the prior
Yoshi’s Story builds. Other stages/modes, additional behavior fidelity,
physical-device/control checks and opt-in MetalFX remain incomplete.


Mac input verification checkpoint: the private VM was initially offline,
booted headlessly, and responded through `guest.sh windows` with only the
login window and notification windows. Mac gameplay/settings UI verification
therefore remains unavailable until the guest session is unlocked. No host
GUI was inspected or driven. The guest accepted a graceful shutdown through
`guest.sh ssh 'sudo -n shutdown -h now'` after the check.

The current windowless Swift/GameController integration passes in
`apple-input-story.log`. `TestApple.swift` now additionally verifies each
WASD/arrow direction, neutral state after release, retaining movement when
one of two identically bound keys is released, clearing held input, ignoring
key repeat during rebinding, and Escape cancellation without writing saved
preferences. The expanded run passes in `apple-input-layouts-story.log`,
alongside existing rebinding persistence, controller-slot stability,
controller disconnect and focus/input-clearing checks. These use the actual
Swift input model and virtual GameController objects; they do not prove Mac
window event delivery or physical Bluetooth pairing. No production app
code changed and the existing Yoshi’s Story app packages remain current.
The full port, remaining stage/mode coverage, physical-device checks and
opt-in MetalFX remain incomplete.


Pokémon Stadium archive checkpoint: `GrPs.usd` (extracted from the supplied
image) and `GrPs1.dat` through `GrPs4.dat` now decode through an owned native
archive API. All have ten model slots; each archive supplies only a subset.
Stage model decoding now clears unresolved external chains using the same
owned-copy mechanism used by other native archive decoders. Unresolved
light override slots are omitted; actual descriptors retain their flags.
This prepares separate archive ownership; it does not implement live
transformation loading or external resource replacement.

Stadium’s 84-byte hazard block preserves 32-bit scalar words, RGBA bytes at
+28, five 16-bit values at +72, and trailing padding. The base archive owns
its 22-entry embedded `SIS_GrPStadiumData` table via the new explicit-count
text-bank decoder, plus the screen image descriptor, lighting, particles,
and quake model. The transformation schema explicitly requires those base
resources to be absent and retains its own stage models/collision/scripts.
The archive checks poison and free source bytes, compare parameter values,
compare 32-byte prefixes of all text entries, check resource presence and
absence, and destroy owners with HSD allocation checks. The playback tests
exercise each archive’s own models. No rendered text or transformation
fidelity claim follows from these checks alone.

All 28 stage-focused tests pass in `stadium-lights-tests.log`, including ten
new Stadium checks. Earlier `stadium-archive-tests.log` did not run because
of a missing test-target text-bank dependency; the subsequent external/light
failures led to the fixes described above. Runtime builds pass in
`stadium-mac-runtime-build.log`, `stadium-iphoneos-runtime-build.log`, and
`stadium-simulator-runtime-build.log`. Stadium is not connected to live
stage loading yet. Existing packaged apps remain the Yoshi’s Story builds;
remaining stages/modes, physical-device verification and opt-in MetalFX
are still outstanding.


Pokémon Stadium live checkpoint: `/GrPs` is connected to the native archive
loader, and `grDatFiles_801C6478` decodes the transformation archive read
into the original file buffer. Native archive release removes its owner
from the stage registry before destroying it, avoiding duplicate cleanup
and allowing the next form to reuse a registry slot. The original
transformation state machine and timing remain active.

ARM64-specific fixes use `sizeof` for the jumbotron image wrapper and text
wrapper initialization. The transformation controller’s write through
`u.display.xD8` is replaced with its intended `u.stadium.xD8` timer field;
pointer expansion makes those union offsets different. Return-to-base
cleanup releases the owned `xD0` archive handle and clears it, rather than
passing the raw `xCC` file buffer as an archive handle. Retail branches
remain unchanged.

`stadium-live.log` exits zero at 2,800 frames with two fighters in live VS.
`stadium-live.png` was inspected and shows the jumbotron rendering the
fighters. `--pokemon-stadium-transform` then passes in `stadium-transform.log`
at 8,500 frames. It observes normal state transitions and requires an
active transformed model followed by the base model. This run chose rock
(target 6): loading starts at 5,268; rock becomes active at 5,869; the stage
returns to base at 7,765. Both fighters remain active at the final capture,
`stadium-transform.png`, which was inspected. No ASan/UBSan report appears
in either passing log. This is one live rock cycle, not proof of fire,
grass or water behavior, collision fidelity, full results/rematch, or UI.
All 28 archive/playback checks pass in `stadium-live-stage-tests.log`.

Mac, iPhoneOS and Simulator packages build successfully with Stadium support
(`mac-stadium-app-build.log`, `iphoneos-stadium-app-build.log`,
`iphonesimulator-stadium-app-build.log`). The device package remains unsigned.
The latest Simulator UI gameplay evidence remains the earlier Yoshi’s Story
session; these new packages have not yet been tested through the app UI.
The full native port, remaining stages/modes and opt-in MetalFX remain
incomplete.


Pokémon Stadium remaining-form checkpoint: the windowless fixture now accepts
`--stadium-fire`, `--stadium-grass`, and `--stadium-water`. A native test-only
setter overrides the next random form selection once; normal gameplay has no
caller and preserves random selection. The original timers, asynchronous
archive loading, transformation states, and return cleanup remain active.
Each fixture requires the requested active model followed by the base model,
then two fighters in live VS at 8,500 frames. A second capture at frame 6,200
uses the output filename plus `.form.png`.

All three runs exit zero with no ASan/UBSan report:
- `stadium-fire.log`: active form 3 at 5,982, base restored at 7,971.
- `stadium-grass.log`: active form 4 at 5,939, base restored at 8,146.
- `stadium-water.log`: active form 9 at 6,002, base restored at 8,008.

All six intermediate/final captures were inspected. The intermediate images
show the selected terrain and both fighters; final captures show the base
arena. Grass's archive owns the `GrdPSGrass*` textures, including its wooden
tower, and matches the original form-4/GrPs2 mapping. Water shows the windmill,
pond and "Water Mode" display. These passes, together with the earlier natural
rock cycle, cover loading and return for all four forms. They do not establish
collision fidelity, sustained repeated transformations, Stadium results/rematch,
or app UI behavior. The fixtures built in `stadium-forms-build.log`; the latest
production app packages remain the prior Stadium builds. No renderer change
was needed for these captures. The full port and opt-in MetalFX remain pending.


Pokémon Stadium rematch checkpoint: `--pokemon-stadium-rematch` passes in
`stadium-rematch.log` (exit zero, 6,000 frames). The fixture shortens the first
match timer to five seconds, uses PAD input to finish and navigate results,
and requires results, character-select return, and another active VS match.
Observed scenes: results at 2,305, character select at 4,014, stage select at
4,403, second match at 4,587. Both fighters are active at frame 6,000.
`stadium-rematch.png` was inspected and shows the second Stadium match.
No ASan/UBSan report appears. This checks base-arena cleanup; the first match
ends before a transformation. Build: `stadium-rematch-build.log`.


Transformed Stadium cleanup checkpoint: `--stadium-form-rematch` selects rock
for the first natural transformation, requires stable form 6 at frame 6,200,
then shortens the remaining match timer to five seconds. PAD inputs finish
that match and navigate back through results/character select. The fixture
requires rock on the final live VS frame, immediately followed by results,
and a second active Stadium match with two fighters at frame 10,400.

The first run, `stadium-form-rematch.log`, reached the second match but failed
its end-form assertion. That assertion incorrectly expected `match_over`
during a live rendered frame; `gm_Scene_Vs_OnExit` sets it during scene exit.
The corrected observer retains the last live form and requires the next
frame to be results. No production gameplay fix was made for this failure.

`stadium-form-rematch-checked.log` exits zero: rock active at 6,057, last live
rock frame 6,619, results 6,620, character select 8,413, stage select 8,603,
second match 8,825, and two active fighters at 10,400. No ASan/UBSan report
appears. `stadium-form-rematch-checked.png.form.png` and the final `.png` were
both inspected; they show rock terrain and the second base-arena match,
respectively. Build: `stadium-form-rematch-check-build.log`.

This verifies one transformed-stage teardown/reload, not repeated full-length
sessions or detailed collision fidelity. Only windowless fixture code changed
in this checkpoint. Packaged apps remain the prior Stadium builds; app UI,
remaining stages/modes, physical-device checks and opt-in MetalFX still need
work. The full native-port goal remains active.


Great Bay native checkpoint: `/GrGb.dat` now loads through the owned stage
archive API. Its 164-byte hazard block preserves float/scalar words and
converts signed halfwords at +0, +0x44, +0x70 and +0x7c (moon/turtle timers,
direction probabilities and item weights). The stage item table owns one
Tingle article (kind 221), including seven animation records. Tingle's
88-byte special block converts scalar values while preserving the two byte
fields and trailing padding at +84. The scalar schema array now derives its
size from its entries; the old explicit size stopped before stage items.
The initial build failed on that old array bound and was corrected.

The shared stage decoder's optional item argument now identifies the actual
stage item: Shy Guy for Yoshi's Story or Tingle for Great Bay. Existing stages
without an article retain an empty item table. Great Bay's archive test
poisons/frees the source bytes before checking the full parameter block,
all Tingle attribute words/bytes, ten stage model slots, resource symbols and
HSD cleanup. All 30 stage archive/playback tests pass in
`greatbay-archive-tests.log`; build `greatbay-archive-build-fixed.log`.

The `--great-bay` windowless PAD fixture navigates stage select and requires
two fighters in live Great Bay VS. `greatbay-live.log` exits zero at 2,800
frames, Fox 0% and Ness 12%, with no ASan/UBSan report. `greatbay-live.png`
was inspected: both fighters, turtle, water and background render. This is
an initial live combat check, not proof of Tingle behavior, turtle/platform
collision fidelity, the full moon/giants sequence or results/rematch.

All three packages build with Great Bay support:
`mac-greatbay-app-build.log`, `iphoneos-greatbay-app-build.log`, and
`iphonesimulator-greatbay-app-build.log`. iPhoneOS remains unsigned and needs
development signing/provisioning for installation. These updated packages
have not yet been exercised through app UI. Remaining stages/modes, physical
controls/devices and opt-in MetalFX remain incomplete; the goal stays active.


Great Bay hazard checkpoint: `--great-bay-hazards` observes original timers
and requires turtle states 0 -> 1 -> 3 -> 2 -> 0 (idle, diving, submerged,
resurfacing, idle), plus finite continuous travel by the live Tingle item.
Large position jumps reset the travel accumulator; the same item must move
at least ten units through steps no larger than ten units each.

`greatbay-hazards-travel.log` passes at 7,800 frames with two active fighters
and no ASan/UBSan report. Tingle's accumulated travel reaches 10.4 at frame
2,951. Turtle states occur at 1,613 / 3,370 / 3,669 / 5,138 / 5,437, completing
the cycle. `greatbay-hazards-travel.png` was inspected. This exercises the
original collision enable/disable calls during the turtle cycle, but does
not prove fighter carrying/collision fidelity, Tingle hit/pop reactions, or
the complete moon/giants sequence. Match length and hazard timers are not
modified by this fixture.

The first run (`greatbay-hazards.log`) completed a turtle cycle at 5,915 but
failed the original Tingle check, which incorrectly required more than one
unit per frame despite a configured descent speed of 0.8. The corrected
fixture accumulates travel and passes. Build: `greatbay-hazards-travel-build.log`.
Only fixture code changed; packaged Great Bay apps remain current. Broader
stage/mode coverage, app UI/device verification and opt-in MetalFX remain
unfinished.


Great Bay rematch checkpoint: `--great-bay-rematch` passes in
`greatbay-rematch.log` (exit zero, 6,000 frames). The fixture shortens the
first match timer to five seconds and uses PAD input through results and
character select. Observed scenes: results 2,270, character select 4,013,
stage select 4,403, second match 4,613. Both fighters remain active at the
final check. `greatbay-rematch.png` was inspected and shows the second match.
No ASan/UBSan report appears. Build: `greatbay-rematch-build.log`.
This verifies a Great Bay scene teardown and reload, including stage-owned
items, but not Tingle hit/pop reactions or the full moon/giants sequence.
Only fixture code changed; existing Great Bay app packages remain current.

Physical device availability was rechecked with `xcrun devicectl list devices`:
all listed physical iPhones were unavailable, and the Simulators were shut
down. No physical-device test was claimed or attempted on another person's
device. Hardware/app UI verification, remaining stages/modes and opt-in
MetalFX remain outstanding. The full port goal remains active.


Kongo Jungle native checkpoint: `/GrKg.dat` now loads through the owned stage
archive API. Its hazard block contains eight signed halfwords at +0x44,
scalar words, and a relocated color-script pointer at +0x84. The parameter
structure is shared through `grkongo.h`; the native branch gives +0x84 a
pointer type. The decoder owns the color script and copies the suffix into
the expanded native layout. Native call sites use `grMaterial_ApplyColorScript`.
Retail parameter/call behavior remains unchanged.

The stage item table owns one Klaptrap article (kind 218) with four animation
records. Its special block is one relocated pointer to five common parameter
words; the following target belongs to the hurtbox data. `itKlapAttributes`
owns those words and exposes a native pointer to them. Archive checks free
poisoned source data before comparing the hazard prefix/suffix, eight
halfwords, Klaptrap pointer ownership, eleven model entries and resource
symbols. All 32 stage archive/playback tests pass in `kongo-color-tests.log`.

Live testing found and fixed two defects. `kongo-live.log` crashed because
`grKongo_801D77E0` advanced through platform state using a retail 16-byte
stride, overwriting a joint pointer after ARM64 pointer expansion. Native
code now derives the stride from `offsetof(grKongo_GroundVars, xD4)`.
`kongo-stride.log` passed that point but crashed on the undeconverted color
script address, leading to the typed pointer/script ownership fix above.

`--kongo-jungle` now passes in `kongo-color.log` (exit zero, 2,800 frames,
two active fighters, no ASan/UBSan report). `kongo-color.png` was inspected
and shows both fighters, wooden platforms and background. This is initial
live stage coverage, not proof of the full barrel/Klaptrap hazard cycle,
collision fidelity, results/rematch or app UI.

All three packages build with Kongo Jungle support: `mac-kongo-app-build.log`,
`iphoneos-kongo-app-build.log`, `iphonesimulator-kongo-app-build.log`. iPhoneOS
remains unsigned; physical-device testing, remaining stages/modes and
opt-in MetalFX are unfinished. The full port goal remains active.


Kongo Jungle rematch checkpoint: `--kongo-jungle-rematch` passes in
`kongo-rematch.log` (exit zero, 6,000 frames). The fixture shortens the first
match timer and navigates results, character select and stage select using
PAD input. Results occurs at 2,272, character select at 4,014 and stage
select at 4,403. The second Kongo Jungle match reaches the final frame with
two active fighters. `kongo-rematch.png` was inspected and shows both fighters
and the stage. No ASan/UBSan report appears. Build: `kongo-rematch-build.log`.
This verifies a scene teardown/reload with the native platform and color-script
fixes, not a full barrel/Klaptrap hazard cycle or collision fidelity. Only
fixture code changed; packaged Kongo Jungle builds remain current. Remaining
stages/modes, app UI/device verification and opt-in MetalFX remain incomplete.


Jungle Japes native checkpoint: `/GrGd.dat` now uses the owned stage archive
API. Its hazard block is eight scalar words (water height/current, Cranky
animation timing, Klaptrap timing/positions); its article table is empty.
Unlike Kongo Jungle, this stage uses the Klaptrap stage model and a collision
item created by `grMaterial_801C8CFC`, rather than a Klaptrap article table.
The existing shared decoder owns its seven model entries, collision,
parameters, scripts, particles, lighting and quake descriptor.

All 34 stage archive/playback checks pass in `japes-archive-tests.log`,
including source poisoning/disposal and parameter comparisons for Japes.
The `--jungle-japes` fixture navigates the original stage-select screen and
requires two fighters in live Japes VS. `japes-live.log` exits zero at 2,800
frames, Fox 0% and Ness 5%, with no ASan/UBSan report. `japes-live.png` was
inspected and shows both fighters on the rendered stage. This does not prove
the complete Klaptrap/Cranky cycle, river-current collision fidelity,
results/rematch or app UI behavior.

All three packages build with Japes: `mac-japes-app-build.log`,
`iphoneos-japes-app-build.log`, `iphonesimulator-japes-app-build.log`.
iPhoneOS remains unsigned. Remaining stages/modes, hardware/UI verification
and opt-in MetalFX remain unfinished; the full goal is active.


Jungle Japes hazard checkpoint: `--jungle-japes-hazards` observes the original
Klaptrap timer and animation, requiring active ground state 1 with collision
item motion 2, then idle state 0 with collision item motion 0. It also requires
Cranky to advance out of his initial animation. No timing or stage parameters
are changed by the fixture.

`japes-hazards.log` exits zero at 6,500 frames with two active fighters and
no ASan/UBSan report. Klaptrap activates at 2,320, returns to idle at 2,619,
and completes additional cycles during the run. Cranky enters animation 1
at 2,799. `japes-hazards.png` was inspected. This verifies the animation and
collision-item state transitions, not an actual Klaptrap hit on a fighter,
river-current collision fidelity, results/rematch or app UI. Build:
`japes-hazards-build.log`. Only fixture code changed; current Japes packages
remain valid. The full port, remaining coverage and opt-in MetalFX remain
unfinished.


Jungle Japes rematch checkpoint: `--jungle-japes-rematch` passes in
`japes-rematch.log` (exit zero, 6,000 frames). The fixture shortens the first
match timer and navigates using PAD input. Results occurs at 2,318, character
select at 4,014, stage select at 4,403 and the second match at 4,601. Both
fighters remain active at the final check. `japes-rematch.png` was inspected
and shows the second match. No ASan/UBSan report appears. Build:
`japes-rematch-build.log`. This verifies one teardown/reload, not actual
Klaptrap hit reactions or river-current collision fidelity. Only fixture
code changed; current Japes app packages remain valid. Remaining stages,
modes, UI/device verification and opt-in MetalFX are unfinished.


Brinstar native checkpoint: `/GrZe.dat` now uses the owned stage archive API.
The 400-byte hazard block includes 30 four-halfword acid timing/level entries,
scalar parameters and a relocated hit-description pointer at +0x2C. The
native parameter type owns that pointer through the archive and expands the
remaining fields accordingly. The nine-word hit description contains damage
14. The empty article table, ten model entries and shared stage resources
are owned by the decoder. All 36 stage archive/playback tests initially pass
in `brinstar-archive-tests.log`, including poisoned-source disposal and
native parameter/pointer comparisons.

Two live failures exposed additional native layout defects:
`brinstar-live.log` reports a global buffer overflow because bubble code
assumed three separately declared globals were contiguous. Native code now
uses one `grZe_BubbleState` containing all four positions and twenty entries.
Stored stage pointers and the bubble initializer's argument now use pointer
width. The left platform has a separate native layout with an aligned,
expanded embedded state; the right platform preserves the same state layout.
Bubble model animations use the typed third stage-model descriptor rather
than retail byte offsets. The acid controller owns its light pointer directly.
`brinstar-layout.log` reached gameplay but crashed when a platform hit callback
wrote through an unrelated structure and corrupted a joint pointer. Native
hit callbacks now write the actual platform velocity/damage fields, and the
acid activation callback sets the actual acid state.

Shared native stage-damage reads in `ftCo_800C08A0` and `ftColl_80076764`
now read `lbColl_80008D30_arg1.damage`, rather than the misleading retail
`DynamicsDesc.count` alias whose offset expands on ARM64. Retail behavior
is preserved. Actual acid damage/knockback fidelity is not yet verified.

`brinstar-callback.log` passes the 2,800-frame live Brinstar fixture with two
active fighters and no ASan/UBSan report. `brinstar-callback.png` was inspected
and shows Fox, Ness and the stage. Build: `brinstar-callback-build.log`.
The extended `--brinstar-acid` fixture observes the normal controller's
states, level range and entry progression without altering its parameters.
Its result and app package rebuilds are still pending at this checkpoint.
Remaining stages/modes, app UI/device checks and opt-in MetalFX remain
unfinished; the full goal remains active.

Brinstar acid checkpoint: `brinstar-acid.log` passes at 6,500 frames, with
both fighters active and no ASan/UBSan report. The natural controller enters
states 1, 2, 3 and 4 and advances to the next timing entry; the first observed
movement cycle completes at frame 2,818 (level -248.857 to -136.060). Further
cycles rise above the stage before receding. Fox finishes at 42% and Ness
at 14%. `brinstar-acid.png` was inspected. This verifies natural acid movement
and continued gameplay, but the fixture does not isolate each damage source
or establish knockback/collision fidelity. Build: `brinstar-acid-build.log`.

All three Brinstar packages build successfully: `mac-brinstar-app-build.log`,
`iphoneos-brinstar-app-build.log`, and `iphonesimulator-brinstar-app-build.log`.
The iPhoneOS package remains unsigned. These are build results; no new Mac
app-window, Simulator UI, physical-device or Bluetooth test is claimed.

Brinstar rematch checkpoint: `brinstar-rematch.log` passes at 6,000 frames.
The fixture shortens the first match timer, then navigates with PAD input.
Results occurs at 2,304, character select at 4,014, stage select at 4,403 and
the second match at 4,587. Both fighters remain active at the final check.
`brinstar-rematch.png` was inspected. No ASan/UBSan report appears. Build:
`brinstar-rematch-build.log`. This verifies a stage teardown and reload, not
every platform-destruction interaction or acid knockback/collision case.
All 36 stage archive/playback tests pass again after the final native layout
changes (`brinstar-final-stage-tests.log`, 19.06 seconds). `git diff --check`
is clean. Brinstar app packages listed above contain the current production
code; only test-fixture code changed afterward. No test process, Simulator
or Mac VM is left running by this checkpoint. The full port goal and opt-in
MetalFX remain unfinished.


Peach's Castle native archive checkpoint: `/GrCs.dat` now uses the owned
stage archive API. The 324-byte hazard block contains eight leading signed
halfwords, timers/padding at +0x40 and +0x54, nine twenty-byte entries beginning
with a halfword ID, and four halfword timers at +0x12C. Remaining scalar words
are converted normally. The archive owns three flag dynamics descriptors
(`dynamicsdata_flag3/4/6`) with three, four and six sixty-byte parameter records,
their native pointers and position vectors. It also owns the empty item table,
21 model entries and shared collision/scripts/lighting/particle resources.
The shared decoder's last argument now uses a named stage-variant enum.
All 38 stage archive/playback checks pass in `castle-archive-tests.log`,
including source poisoning/disposal and exact flag/parameter comparisons.

`castle-live.log` reached the stage but reported an ASan invalid read in
`grCastle_801CF868`: the satellite controller used a 32-bit pointer-array view
of native object references. Native Castle fields now retain pointer width.
Flag dynamics access uses the expanded controller layout; satellite history,
projectile index/camera/collision fields, and explosion camera/collision
fields use their actual native views. Satellite countdown no longer aliases
an expanded platform record. Block callbacks use `Ground.castle10` rather
than a padded retail-offset structure. Original stage logic remains in use.

`castle-layout.log` passes at 2,800 frames with two active fighters and no
ASan/UBSan report. `castle-layout.png` was inspected. Build:
`castle-layout-build.log`. The results/rematch and extended natural Banzai
Bill fixtures are being checked separately; this initial match does not
establish every switch/block interaction or hazard's collision fidelity.

Castle rematch checkpoint: `castle-rematch.log` passes at 6,000 frames. The
fixture shortens the first match timer and navigates with PAD input through
results, character select, stage select and a second Castle match. Both
fighters remain active at the final check. `castle-rematch.png` was inspected;
no ASan/UBSan report appears. Build: `castle-rematch-build.log`. This covers
one teardown/reload of the native flag and hazard resources.

All three app packages build: `mac-castle-app-build.log`,
`iphoneos-castle-app-build.log`, `iphonesimulator-castle-app-build.log`.
iPhoneOS remains unsigned. These build checks do not establish actual
Mac window input, physical iPhone/iPad, Bluetooth or multitouch behavior.
The full port and opt-in MetalFX remain unfinished.

The extended Castle test found a further defect at the Banzai Bill explosion:
`castle-hazards.log` observed model 9 with camera/collision at frame 6,559,
then ASan reported an invalid read of archive address 0x959AC in the color
command interpreter. Hazard field +0x114 is the block's sole relocated
pointer, not a scalar integer. `grCastle_YakumonoParam` is now shared through
`grcastle.h`; native code owns the color script, expands that pointer field,
copies the parameter suffix to its native offsets, and calls
`grMaterial_ApplyColorScript`. The archive test now checks this native pointer
and prefix/suffix after freeing poisoned source data. All 38 tests pass in
`castle-color-tests.log` (21.74 seconds); build `castle-color-build.log`.

The final color-script fix builds for all three packages:
`mac-castle-color-app-build.log`, `iphoneos-castle-color-app-build.log`, and
`iphonesimulator-castle-color-app-build.log`. These supersede the earlier
Castle package logs. The extended live rerun remains pending at this point.

Castle Banzai Bill checkpoint: `castle-color-hazards.log` passes at 8,200
frames with both fighters active and no ASan/UBSan report. The original
stage timer spawns model 14 with non-null camera/collision objects at 6,913.
An explosion with its own camera/collision appears at 7,718, and both the
projectile and explosion have been removed at 7,814. No stage parameters or
spawn times were overridden. `castle-color-hazards.png` and the automatic
first-explosion-state capture `castle-color-hazards.png.explosion.png` were
inspected. The latter captures the start of the effect near/below the frame
edge, not a clear view of its full expansion. The fixture establishes the
spawn/explosion/cleanup states and continued gameplay; it does not isolate
fighter damage/knockback or establish full visual/collision fidelity.
Build: `castle-color-hazards-build.log`.

The earlier rematch scenes occur at 2,269 (results), 4,013 (character select),
4,403 (stage select) and 4,613 (second match). This rematch predates the final
color-script ownership fix; final ownership/destruction is covered by the
38 passing archive/playback tests and the extended live cycle above.
Final Castle packages contain the current production code. `git diff --check`
is clean. All test/build process handles are terminal; no Simulator or Mac
VM was started during this checkpoint. Remaining stages/modes, app UI and
hardware verification, full hazard fidelity and opt-in MetalFX are unfinished.
The full port goal remains active.

Yoshi's Island native checkpoint: `/GrYt.dat` uses the owned stage archive
API. It contains eight scalar hazard words, an empty item table, two model
entries and no stage-specific `map_ptcl`/`map_texg` bank. The stage variant
explicitly accepts those absent particle symbols while owning models,
collision, parameters, scripts, lighting and quake resources. Archive checks
poison/free source data and compare all parameters and resource presence.
All 40 stage archive/playback tests pass in `yorster-archive-tests.log`
(23.15 seconds); build `yorster-archive-build.log`.

The native block-hit callback in `grYorster_80202428` now retains its Ground
pointer instead of narrowing it through `s32`. `yorster-live.log` passes at
2,800 frames with Fox and Ness active, no ASan/UBSan report, and an inspected
`yorster-live.png`. Build: `yorster-live-build.log`.

All three packages build with Yoshi's Island: `mac-yorster-app-build.log`,
`iphoneos-yorster-app-build.log`, `iphonesimulator-yorster-app-build.log`.
iPhoneOS remains unsigned. These are build results, not new app UI or
physical-device checks. The extended block and rematch checks are pending.

Yoshi's Island block checkpoint: `--yoshis-island-blocks` uses PAD input to
walk toward the lowest block row and issue downward attacks, then releases
input and observes restoration. The initial `yorster-blocks.log` fixture
ended before block objects had been created by the round-start callback;
the observer now waits for `yorster.xC4 == 0`. The next fixture
`yorster-blocks-ready.log` reached 6,500 frames without a sanitizer error but
failed its coverage condition: maximum-speed steering oscillated across the
target and did not reliably attack. The fixture now walks with stick 40 and
stops within twelve stage units, instead of dashing with stick 80 to within
three units. No gameplay parameters or block state are forced.

`yorster-blocks-approach.log` passes at 6,500 frames, with both fighters active
and no ASan/UBSan report. The observer requires one block to enter states 2
(triggered), 3 (removed), then 1 (restored). Block 3 completes the first cycle
at frame 3,603; further cycles occur across the central row. Block damage
is nonzero during the interactions. `yorster-blocks-approach.png` was inspected.
Build: `yorster-blocks-approach-build.log`. This verifies a real PAD-driven
block interaction and timer-based restoration, not every landing-speed,
attack or item interaction. Only fixture code changed after the app builds.

The initial Island rematch fixture (`yorster-rematch.log`) entered Sudden
Death at frame 2,318 and results at 4,015. Its first scheduled confirmation
was at 4,000, so it missed the results/menu sequence and failed at 6,000;
there was no sanitizer report. The Island fixture now uses the established
Fountain menu delay (+1,600 frames), an additional both-player Start pulse,
a fresh stage-guidance reset at 5,900, and an 8,000-frame final check. This
changes the test navigation only. The delayed rerun is pending at this point.

Yoshi's Island rematch checkpoint: `yorster-rematch-delayed.log` passes at
8,000 frames. Observed scenes are Sudden Death 2,318, results 3,796, character
select 5,814, stage select 6,003 and second match 6,201. Both fighters remain
active at the final check. No ASan/UBSan report appears. The final
`yorster-rematch-delayed.png` was inspected. Build:
`yorster-rematch-delayed-build.log`. This covers the tied-match transition
and one stage teardown/reload, in addition to the separate block restoration
check. `git diff --check` is clean. The previously built Island packages
remain current because subsequent changes are test-fixture code only.

All process handles from this checkpoint are terminal. No Simulator or Mac
VM was started. The next useful app-level check is a fresh iOS Simulator
touch session with the current package; the last such session predates the
recent stages. Remaining stages/modes, Mac app-window and physical-device
verification, broader input/hazard fidelity and opt-in MetalFX are unfinished.
The full port goal remains active.

Yoshi's Island iOS touch checkpoint: `ios-control-session-yorster.log` and
`ios-control-session-yorster.xcresult` report one passing XCTest control
session (533.068 seconds). The current Simulator package ran Fox against
CPU level 1 Yoshi in a normal, unshortened two-minute match. Touch controls
navigated character/stage selection, sent movement, A/B/X and R presses,
paused/resumed, returned from results to character select, and loaded a
second Island match. This uses UIKit touch events through XCTest, not
direct PAD-state injection. Individual attack/jump/shield outcomes were
not isolated from CPU hits in this session.

Inspected captures in `native/build/ios-control-session-yorster/` include
`frame-15.png` (Island selection), `frame-20.png` (combat), `frame-23.png`
(pause), `frame-26.png` (resumed combat), `frame-31.png` (results: Fox -2,
Yoshi +1), `frame-33.png` (character select), `frame-37.png` (Island selected
again), and `frame-39.png` (second match, timer 1:44). The scene renders
visible stage/fighter content; these are actual Simulator app captures.
The session ran with `MELEE_TEST_SILENT=1`, so it adds no audio evidence.
It also does not verify physical multitouch, Bluetooth, an actual iPhone,
iPad or a Mac app window. XCTest exited successfully; the Simulator was
already shut down when cleanup requested shutdown. No test process remains.

MetalFX remains requested as an opt-in, persisted setting, off by default
on supported devices after core gameplay is working; it is not implemented.
Remaining stages/modes, broader gameplay fidelity and hardware checks are
still unfinished. The full port goal remains active.


Green Greens native checkpoint: `/GrGr.dat` now uses the owned stage archive
API, with seven model entries, 31 scalar hazard words and two apple entries
(WhispyApple 225 and WhispyHealApple 226). Both own their article, three
animation/script records and pointer-bearing special attributes, including
five common parameter words. The native Whispy attributes keep the original
retail layout outside `MELEE_NATIVE`.

`greens-archive-tests.log` initially passed the existing 40 checks but failed
both Greens checks because the shared environment decoder rejected fog with
start == end (2000). The original `GXPixel.c` explicitly handles equal fog
bounds. The decoder now accepts equality and retains both values, verified
by the Greens archive test after source memory is poisoned/freed. All 42
stage archive/playback checks pass in `greens-fog-tests.log` (29.09 seconds).
Build: `greens-fog-build.log`.

The initial `--green-greens` command was rejected by the fixture argument
allowlist; `greens-cli-build.log` includes the argument fix. `greens-live.log`
then passes at 2,800 frames with Fox and Ness active and no sanitizer report.
`greens-live.png` was inspected: stage, fighters and block columns render.
That run predates a separate Whispy initialization correction: the native
initializer now writes the scalar `greens2` state directly, rather than
using the block-controller union view whose pointers widened on ARM64.
The latter shifts its timers and flags relative to `greens2`. The original
retail initialization is preserved. Build: `greens-state-build.log`.

All three packages build with the corrected state: `mac-greens-app-build.log`,
`iphoneos-greens-app-build.log`, `iphonesimulator-greens-app-build.log`.
iPhoneOS remains unsigned. These are builds, not additional app-window,
Simulator touch or physical-device evidence. A corrected-state live check
is running at this checkpoint. Natural wind/apple cycles, block damage and
rematch coverage are still pending; the basic match does not prove them.

`greens-state.log` now passes at 2,800 frames with the corrected initialization;
both fighters are active and no ASan/UBSan report appears. The final
`greens-state.png` was inspected and shows visible stage/fighter/block
content. All build/test process handles for this checkpoint are terminal.
`git diff --check` is clean. No Simulator or Mac VM was started this turn.
The next Green Greens work is extended natural hazard and rematch coverage;
two pointer-narrowing casts in its block callback/unused return also remain
to review. Remaining stages/modes, full gameplay fidelity, hardware checks
and opt-in MetalFX are unfinished. The full port goal remains active.


Green Greens extended hazard checkpoint: `--green-greens-hazards` observes
Whispy's scalar state, active wind direction and live apple item kinds.
It leaves natural timers/spawn logic unchanged and requires wind plus an
actual apple before the 7,800-frame endpoint. `greens-hazards-build.log`
builds successfully; `greens-hazards.log` passes at 7,800 with both fighters
active and no sanitizer report. Wind state starts at 3,485, active direction
1 appears at 3,617, and idle resumes at 4,035. Another wind phase occurs at
5,135–5,686. Apple state starts at 7,485; damaging apple 225 appears at 7,526
and healing apple 226 at 7,542, followed by idle at 7,582. Final damage is
Fox 22 / Ness 1. `greens-hazards.png` was inspected.

This proves natural wind activation and spawning of both owned apple kinds;
it does not isolate wind displacement, apple damage/healing, the opposite
wind direction, block destruction or rematch behavior. Only fixture code
changed, so the prior three Green Greens app packages remain current.
The process exited successfully and no Simulator/VM was started. The next
stage-specific work remains block interaction and rematch verification.
The full port, physical-device checks and opt-in MetalFX remain unfinished.


Green Greens rematch checkpoint: `--green-greens-rematch` uses the existing
shortened-match fixture and delayed both-player menu confirmation schedule.
`greens-rematch-build.log` builds successfully. `greens-rematch.log` passes
at 8,000 frames, observing results at 2,318, character select at 5,614,
stage select at 5,804 and the second match at 6,039. Scene IDs were checked
against `gmvsmode.h`: 4 is Results, 3 is Sudden Death. An intermediate
commentary incorrectly called state 4 Sudden Death; this run does not
provide Sudden Death coverage. Both fighters are active in the second match
at the final check, with no sanitizer report. `greens-rematch.png` was
inspected and shows Fox/Ness, block columns and timer 1:29.

This verifies one native Green Greens stage teardown/reload through results
and menus. It is not a full-length match test or block-damage verification.
Only fixture code changed; existing Green Greens packages remain current.
All process handles are terminal. The next stage-specific work is block
interaction coverage and review of the remaining pointer-narrowing casts.
The full port, hardware verification and opt-in MetalFX remain unfinished.


Green Greens block checkpoint: the block callback's `value` argument and
`grGreens_BlockVars.x1C` now use `grGreens_PointerWord` (intptr_t on native,
s32 on retail). The unused pointer return from `grGreens_80215D54` uses the
same type. This removes pointer narrowing while preserving retail layout.
`greens-blocks-build.log` builds the runtime and probe successfully.

`--green-greens-blocks` observes a nonzero hit-source pointer and subsequent
active-block count reduction, then continues controller-driven approaches
and attacks through frame 6,000 and releases input. In `greens-blocks.log`,
the existing opening combat inputs already hit row 1 / column 3 at frame
1,802, recording full pointer 0x11a421840. Active blocks fall from 18 to 16
at 1,803. The test passes at 6,500 with Fox and Ness active and no sanitizer
report. `greens-blocks.png` was inspected. This establishes a real fighter
hit followed by block removal and continued play, not every bomb-chain,
collision or restoration combination.

All three packages build with this fix: `mac-greens-blocks-app-build.log`,
`iphoneos-greens-blocks-app-build.log`,
`iphonesimulator-greens-blocks-app-build.log`. iPhoneOS remains unsigned.
All process handles are terminal; no Simulator or VM was started. Remaining
stages/modes, broader fidelity, app-window and physical-device checks, and
opt-in MetalFX remain unfinished. The full port goal remains active.


Temple native checkpoint: `/GrSh.dat` is admitted through the owned stage
archive API. It has three model entries, an unused zero scalar at
`yakumono_param`, an empty item table, and no `map_ptcl`/`map_texg` symbols.
The Shrine variant explicitly requires their absence while owning models,
collision, scripts, lighting and quake resources. `shrine-build.log` builds
successfully; all 44 stage archive/playback tests pass in `shrine-tests.log`
(29.65 seconds), including source disposal and Temple's absent particle bank.

The first live fixture (`shrine-live.log`) failed its stage-progress check
at frame 2,800. Its saved screenshot shows Home-Run Contest character select,
not Temple; no stage archive was reached. Package builds ran concurrently
with that attempt. After those builds exited, the isolated rerun
`shrine-live-isolated.log` passes at 2,800, loading `/GrSh.dat` at 1,601 and
ending with Fox and Ness active and no sanitizer report. This suggests a
possible timing-sensitive navigation fixture, not a proven stage failure;
no production timing change was made and the cause remains unconfirmed.

`shrine-live-isolated.png` was inspected. It renders the fighters and Temple
geometry but has an apparent reversed yellow menu-text fragment on the
upper scenery. This is an unresolved visual artifact requiring investigation;
the basic live check does not establish complete Temple rendering fidelity.
Full collision traversal, long matches and rematch coverage also remain.

All three Temple packages build: `mac-shrine-app-build.log`,
`iphoneos-shrine-app-build.log`, `iphonesimulator-shrine-app-build.log`.
iPhoneOS remains unsigned. All process handles are terminal and
`git diff --check` passes. No Simulator/VM was started. Remaining stages,
modes, rendering/gameplay fidelity, device verification and opt-in MetalFX
are unfinished; the full port goal remains active.


Temple rendering diagnostic checkpoint: the fixture-only
`MELEE_STARTUP_NO_HUD=1` option now also applies to Temple. The diagnostic
run `shrine-no-hud.log` passes at 2,800 with no sanitizer report. Its inspected
`shrine-no-hud.png` retains the yellow text-like scenery patch after hiding
the HUD, so this is not the same overlay confusion as the earlier Fountain
investigation. Temporary copy-binding logging observed two 256x256 I4
requests with matching 256x256 copy handles, each at revision 1. Logging was
once per source address; it does not exclude subsequent address reuse or
prove every binding is correct.

A CPU decode of 37 non-paletted public `_image` textures from `GrSh.dat`
produced `shrine-texture-atlas.png`, which was inspected. None visibly
contains the conspicuous yellow text patch. This narrows the investigation
but does not by itself identify the affected material, rule out every image
or prove a cache defect. Next inspect the static texture/draw binding for
the affected surface and its source descriptor, rather than altering stage
artwork or hiding geometry.

Temporary Aurora `texture.cpp` logging was removed by restoring the exact
pre-diagnostic source saved as `shrine-texture-before.cpp`.
`shrine-diagnostic-build.log` and `shrine-diagnostic-clean-build.log` both
build successfully. Only the opt-in probe HUD switch persists; production
renderer behavior is unchanged and the prior Temple app packages remain
current. All processes are terminal; no Simulator/VM was started. Temple's
visual issue, broader gameplay/device checks, remaining stages/modes and
opt-in MetalFX are unfinished. The full goal remains active.


Temple binding diagnostic checkpoint: `shrine-rehash.log` passes at 2,800
with temporary texture-object cache and binding fast paths bypassed. The
inspected `shrine-rehash.png` retains the same patch, so bypassing those
fast paths does not resolve it. This does not exclude other GPU caches.

A second temporary diagnostic captures non-paletted CPU texture sources
actually bound after renderer frame 2,000. `shrine-bound.log` passes at
2,800; the inspected `shrine-bound.png` still has the artifact.
`shrine-bound-textures/` contains 144 raw base-level images and decoded PNGs;
`shrine_bound_atlas.py` reproduces the atlas using the existing CPU decoder.
All four `shrine-bound-page-*.png` pages were inspected. They show stage,
fighter and effect artwork with no matching yellow text fragment. This
capture excludes palette textures and does not capture the GPU contents
of EFB copies: the two 256x256 I4 CPU buffers are not evidence of what those
copy handles contain. Next inspect those GPU copies directly and correlate
the offending draw's GPU texture bindings with the CPU descriptors.

All temporary Aurora diagnostic changes were restored from the exact
pre-diagnostic source (`shrine-rehash-before.cpp`). The clean probe builds
in `shrine-bound-clean-build.log`; all process handles are terminal.
No production renderer change or new package is claimed. Temple's visual
artifact remains unresolved, and the full port goal remains active.


Temple GPU-copy checkpoint: the windowless probe now supports opt-in
`MELEE_STARTUP_GPU_COPIES=1`, saving every currently cached framebuffer copy
alongside the final frame. `shrine-copy-build.log` builds and
`shrine-copies.log` passes at 2,800 without sanitizer errors. Both GPU copies
are 256x256, format 22; inspected `shrine-copies.png.copy-0.png` and
`shrine-copies.png.copy-1.png` contain clean Ness/Fox shadow silhouettes over
white. Neither contains the yellow text fragment. This directly checks GPU
copy contents, unlike the earlier CPU buffer dumps, and does not support
those copies as the source of the artifact.

Next compare GPU-resident static stage textures to the known CPU images and
trace the affected draw's bindings. Renderer resource-cache keys include
bind-group entries and texture views; source inspection alone is not proof
of correct bindings. Texture staging row alignment is applied during encode,
so the unaligned logical stride in queue_texture_upload_data is not by itself
a demonstrated bug. No speculative renderer change was made.

Only probe capture code changed. The existing Temple packages remain current,
all process handles are terminal, and no Simulator/VM was started. Temple's
artifact and the broader native port remain unfinished.


Temple rendering assessment correction: GPU readback resolves the alleged
yellow-text artifact as original stage artwork. `shrine-gpu.log` passes at
2,800; a temporary diagnostic retained bound GPU texture handles and read
back 149 base-level images. The first atlas page identifies yellow decorative
lettering in GPU textures 4 (256x256), 9 (128x128) and 10 (256x128).
These are indexed-color stage images, excluded from the prior non-paletted
source captures: `GrdShrineIseki2_C8_image`, `GrdShrineStep2_C8_image`, and
`GrdShrineStep1_C8_image`.

`shrine_palette_check.py` decodes each directly from `GrSh.dat` using its
RGB565 palette (251, 256 and 256 entries respectively) and compares it with
the corresponding GPU readback. All three are pixel-identical in RGBA;
every channel's difference extrema are (0,0). The three inspected
`shrine-retail-{Iseki2,Step2,Step1}.png` images visibly contain the yellow
lettering and triangular motif. The earlier description as reversed menu
text was mistaken. This evidence resolves that specific visual hypothesis;
no renderer or artwork change is warranted. It does not prove all Temple
rendering or collision fidelity.

Temporary GPU texture registry/readback instrumentation was restored from
`shrine-gpu-before.cpp` and `shrine-gpu-probe-before.cpp`.
`shrine-gpu-clean-build.log` builds the clean probe. The prior opt-in GPU-copy
capture helper remains; production code and existing Temple packages are
unchanged. All processes are terminal and `git diff --check` is clean.
Next resume Temple gameplay traversal and rematch checks rather than pursue
the disproven texture hypothesis. The full port and opt-in MetalFX remain
unfinished.

Temple rematch checkpoint: `--temple-rematch` now waits for the actual
Results scene before sending menu confirmations. The initial fixed-input
fixture (`shrine-rematch.log`) failed at 8,000 submissions while still in
Sudden Death; its menu inputs could land during gameplay. The corrected
fixture leaves that round alone and resets stage-selection guidance when
the second selection scene begins. No production gameplay rule changed.

`shrine-rematch-state-build.log` builds and `shrine-rematch-state.log`
passes at 11,000 submissions without sanitizer errors. Scene transitions
are first match at 1,603, Sudden Death at 2,317, Results at 8,553, character
selection at 8,986, stage selection at 9,049, and second match at 9,247.
Both fighters remain active on Temple at the final check. The inspected
`shrine-rematch-state.png` shows Fox and Ness at 0%, with 1:32 remaining
in the second match. This checks match teardown/reload, not exhaustive
Temple collision traversal or physical controller behavior.

Only the test fixture changed; the existing Temple app packages remain
current. All processes are terminal. Full stage/mode coverage, physical
device checks, and the requested opt-in MetalFX setting remain unfinished.

Venom asset checkpoint: extracted `GrVe.usd` from the supplied disc and
added an owned stage decoder. Its eight models, collision, ground parameters,
lighting, quake model and stage scripts use the existing native owners.
There is no particle bank. The hazard block contains fourteen scalar words
and a color-script pointer at +0x38; the native descriptor owns that script
and uses the typed color-script application path. `SIS_GrCorneriaData`
contains 53 relocated dialogue entries and is copied into an owned text bank.

The stage's item 234 is the Arwing laser, with six animation/script records.
Its special attributes own the five-word common prefix and the two float
multipliers (5 and 6); the common speed multiplier is 4. The shared typed
`ArwingLaserAttr` declaration preserves the original pointer-and-floats
layout while allowing native pointers. `hsd_venom_archive` poisons and frees
the source archive before checking the native parameters, item attributes,
text table, required symbols and clean destruction.

`venom-assets-build.log` builds the probe and runtime targets.
`venom-assets-tests.log` passes all 46 selected stage archive/playback checks
in 38.29 seconds, including both Venom checks, under the existing sanitizer
configuration. `git diff --check` is clean. No live Venom match or new app
package is claimed: the stage is not yet admitted by `grdatfiles.c`.

Next fix Venom's runtime addressing before admission: `grVenom_80203EAC`
derives callbacks from a fixed +0x44 offset beyond a separate global,
and the Arwing routines index pointers and neighboring tables through
32-bit words based at `grVe_803E5348`. Native typed references are needed
for callbacks, live Arwing objects, group/state tables and spawn data.
The previous Temple packages remain available; no GPU process, Simulator or
VM remains running. The full port and opt-in MetalFX remain active work.

Venom live-runtime checkpoint: native callback and Arwing object/state/group
access now use their typed owners. Animation, helper-object, joint and spawn
tables are addressed by their actual symbols instead of walking beyond
`grVe_803E5348`. The table values were checked against the supplied disc's
main DOL at 0x803E5530, 0x803E5644, 0x803E5680 and 0x803E56A0 and match.
Retail expressions remain behind the non-native branches. `/GrVe` is now
admitted through the owned decoder and `--venom` selects it through the
existing atomic stage-selection guidance.

The first live run (`venom-live.log`) exposed a pointer read crash in
`grVenom_8020454C`. Native environment traversal now iterates seven actual
joint pointers rather than shifting a Ground pointer; its previous-animation
frame lives in the native environment record instead of aliasing widened
pointers through another union view. Lighting reads the named environment
bits rather than the retail byte layout. The second run
(`venom-environment.log`) passed that point but exhausted Aurora's 32 MiB
storage buffer. The reproducible Aurora patch now reserves 64 MiB; its
checkout/patch verification passes. This increases renderer memory use and
still needs physical iPhone/iPad validation.

`venom-storage-build.log` builds and `venom-storage.log` passes at 2,800
submissions without sanitizer errors. Peak recorded frame storage is
50,051,592 bytes. The inspected `venom-storage.png` shows a live match on
the Great Fox above Venom, Fox at 10%, Ness at 20%, and 1:41 remaining.
This proves basic stage entry/combat/rendering, not every Arwing attack,
environment cycle, smash taunt, rematch or physical input path.

`venom-runtime-tests.log` passes all 46 selected stage archive/playback checks
in 38.93 seconds after the runtime changes. All three app packages rebuild
successfully in `{mac,iphoneos,iphonesimulator}-venom-app-build-approved.log`.
The initial unprivileged package attempts failed because Swift's macro
compiler could not start its sandbox; the escalated retries succeeded.
The iPhoneOS package remains unsigned and requires development provisioning.
All process handles are terminal, no Simulator or VM was started, and
`git diff --check` is clean. Next verify the natural Arwing/laser cycle and
Venom rematch, then continue remaining stages and physical-device checks.
The requested opt-in MetalFX setting remains unfinished.

Venom natural-hazard checkpoint: `--venom-hazards` observes an Arwing
arrival, a live laser article and an Arwing departure without forcing
hazard timers, object creation or fighter positions. `venom-hazards-build.log`
builds and `venom-hazards.log` passes at 7,800 submissions. Arwings arrive at
2,199 and 4,433, depart at 2,928 and 4,728, and another arrives at 7,690.
A laser article is observed in motion 3 at 4,540. Both fighters remain
active at the final check (Fox 20%, Ness 30%), with no sanitizer errors.
The inspected `venom-hazards.png` shows combat with 0:18 remaining and the
changed environment. This does not isolate laser damage, every laser motion,
all environmental transitions or the smash-taunt dialogue path.

The next fixture, `--venom-rematch`, reuses the scene-aware menu confirmation
helper from Temple (renamed `melee_startup_results_rematch`) and checks a
second live match after Results and fresh stage selection. These fixture
changes do not require rebuilding the production app packages.

The initial Venom rematch fixture (`venom-rematch.log`) reached Results
at 2,314, character selection at 2,747, stage selection at 2,810 and the
second regular match at 3,052. It subsequently reached that match's Sudden
Death at 10,482 without sanitizer errors, then failed its regular-match
assertion at 11,000. This was an observation-window error: the fixture
inherited Temple's longer limit, which extends past Venom's second regular
round. Venom's final check is now at 6,000; Temple retains 11,000. This does
not change game timers or rules.

`venom-rematch-window-build.log` builds and `venom-rematch-window.log`
passes at 6,000 submissions without sanitizer errors. Results occurs at
2,319, character selection at 2,752, stage selection at 2,816 and the second
match at 3,042. The inspected `venom-rematch-window.png` shows both fighters
at 0% with 1:12 remaining in the second match. All process handles are
terminal and `git diff --check` is clean. Production Venom app packages
remain current because only fixtures changed. The full native port, physical
device validation and opt-in MetalFX remain unfinished.

Corneria native checkpoint: `GrCn.usd` from the supplied disc now has an
owned decoder for eleven models, collision, parameters, particles, lighting,
quake and scripts. Its 35-word hazard block includes a color-script pointer
at +0x84; the native typed descriptor owns that script and preserves the
trailing +0x88 scalar. Dialogue uses the same 53-entry SIS bank format as
Venom. Itemdata contains Arwing laser 234 (six animation/script records) and
Great Fox laser 235 (two records). The latter has four float attributes,
50/20/10/20, covered by the scalar schema and source-disposal archive test.

`corneria-assets-build.log` builds, and `corneria-assets-tests.log` passes
all 48 selected stage archive/playback checks in 39.37 seconds. The live
loader admits `/GrCn`, and `--corneria` uses atomic stage-selection guidance.
`corneria-live-build.log` builds and `corneria-live.log` passes at 2,800
submissions without sanitizer errors. The inspected `corneria-live.png`
shows both fighters, the Great Fox, the environment and an Arwing with
1:41 remaining. Both fighters are at 0%, so this run proves stage entry
and active fighter/rendering state, not damage from laser hits or full
combat coverage. Next check natural Arwing/Great Fox laser behavior and
rematches; smash-taunt dialogue also remains unverified.

All three Corneria app packages build successfully in
`{mac,iphoneos,iphonesimulator}-corneria-app-build.log`. The device package
remains unsigned and needs development provisioning. No Simulator or VM
was started, all process handles are terminal, and `git diff --check` is
clean. Full port coverage, physical-device validation and opt-in MetalFX
remain unfinished.

Corneria natural-laser checkpoint: `--corneria-hazards` observes both
stage-specific laser kinds without forcing hazard timers, item creation,
or fighter positions. `corneria-hazards-build.log` builds and
`corneria-hazards.log` passes at 7,800 submissions without sanitizer errors.
Great Fox laser 235 appears in motion 0 at frame 2,040; Arwing laser 234
appears in motion 5 at 4,105. Both fighters remain active at the final
check. The inspected `corneria-hazards.png` shows Fox and Ness on the Great
Fox with 0:18 remaining and the city background. Both are still at 0%, so
this verifies natural laser creation and continued match execution, not
laser damage or every projectile motion.

Only the fixture changed; existing Corneria app packages remain current.
All process handles are terminal, no Simulator or VM was started, and
`git diff --check` is clean. Corneria rematches, smash-taunt dialogue,
physical-device checks, remaining stages/modes and opt-in MetalFX remain
unfinished.

Corneria rematch checkpoint: `--corneria-rematch` reuses the scene-aware
Results confirmation helper and resets stage-selection guidance for the
second selection. `corneria-rematch-build.log` builds and
`corneria-rematch.log` passes at 6,000 submissions without sanitizer errors.
The first regular match begins at 1,614, Sudden Death at 2,271, Results at
3,956, character selection at 4,390, stage selection at 4,453 and the second
regular match at 4,663. The inspected `corneria-rematch.png` shows Fox at
20%, Ness at 0%, and 1:39 remaining in the second match. The damage source
was not isolated, so this is not proof of a particular laser-hit behavior.

Only the fixture changed; production Corneria app packages remain current.
All process handles are terminal, no Simulator or VM was started, and
`git diff --check` is clean. Corneria smash-taunt dialogue, physical input,
remaining stage/mode coverage and opt-in MetalFX remain unfinished.

Mute City asset checkpoint: the owned decoder covers GrMc.dat's 39 models,
collision, ground parameters, lighting, quake and scripts, with no item
articles or particle bank. Its 20-word hazard block contains two color
script pointers and two pointers to nine-word collision-hit records (the
source labels these DynamicsDesc pointers). The native typed parameter
owner copies both hit records and the scalar tail. The stage color calls
use the typed native script API; the retail expressions remain intact.

Initial `mutecity-assets-tests.log` passed 48/50 but rejected joint
animations for models 36 and 37. Their AObj object references point to
spline joints at 0x2528 and 0x3DA8, outside each model's own joint tree.
The native scene decoder now owns these external spline trees and reuses
references within a scene. It checks the referenced root is a spline and
rolls back new owners when animation binding fails.

The first implementation decoded all 39 models but failed the HSD cleanup
assertions (`mutecity-spline-tests.log`): releasing the external tree before
AObj binding left sibling joints outside the AObj's root-only unref path.
Keeping the full external tree owned until final scene destruction fixes
that lifetime. `mutecity-spline-owner-build.log` builds, and
`mutecity-spline-owner-tests.log` passes all 50 selected stage archive and
playback checks in 43.31 seconds, including source-buffer disposal and zero
remaining HSD ID/AObj/FObj allocations. `git diff --check` is clean.

Mute City is not yet admitted by grdatfiles.c and no live match is claimed.
Next inspect car pointer storage (`grMc_8049F4B8[].x24`), the fixed dynamic
model offset in grMuteCity_801F25D0's vicinity, and moving-track state before
stage admission. App packages have not been rebuilt for this checkpoint;
the latest available bundles are the preceding Corneria builds. All process
handles are terminal; no Simulator or VM was started. Full port coverage,
physical-device validation and opt-in MetalFX remain unfinished.

Mute City live checkpoint: the existing native car-object pointer storage
was already widened. Two remaining runtime assumptions needed changes:
grMuteCity_801F106C now references the actual car array instead of assuming
it follows the separate sort-index global, and grMuteCity_801F28A8 constructs
a native DynamicModelDesc from model 38 instead of adding retail byte offset
0x7B8 to the widened model table. Retail branches retain their expressions.
The prior checkpoint's reference to the vicinity of 801F25D0 was imprecise;
801F28A8 is the actual fixed-offset accessor.

`/GrMc.dat` is now admitted through the owned archive decoder and
`--mute-city` selects it through atomic stage-menu guidance.
`mutecity-live-build.log` builds and `mutecity-live.log` passes at 2,800
submissions without sanitizer errors. The inspected `mutecity-live.png`
shows Fox at 0%, Ness at 6%, and 1:41 remaining on the moving platform.
This establishes basic stage entry, active fighters and rendering, not
all track phases, car collision/damage, crash effects or rematches.

All three Mute City app packages build successfully in
`{mac,iphoneos,iphonesimulator}-mutecity-app-build.log`; the physical-device
package remains unsigned and needs development provisioning. All processes
are terminal, no Simulator or VM was started, and `git diff --check` is
clean. Next verify the natural track/car cycle and rematch. The full port,
physical-device validation and opt-in MetalFX remain unfinished.

Mute City track checkpoint: `--mute-city-track` observes the actual track
controller, all three track modes, both states of its collision/movement
update enable bit, and translation of a car joint. It does not force stage
timers, car positions, damage or track commands. The initial observer
(`mutecity-track.log`) mistakenly read map object 0, the scenery, as the
track controller and crashed inside the fixture. The controller is map 30,
as established by its init/proc callbacks. No production fix was needed for
that observer error.

`mutecity-track-controller-build.log` builds and
`mutecity-track-controller.log` passes at 7,800 submissions without sanitizer
errors. Car movement is observed at 1,600. Track modes change from 2 to 1
at 2,693, to 0 at 3,467, and back to 2 at 4,097. The update-enable bit clears
at 5,853 and sets again at 6,869. Both fighters remain active at the final
check, Fox at 30% and Ness at 87%. The inspected
`mutecity-track-controller.png` shows the platform, passing cars and both
fighters with 0:18 remaining. Damage sources were not isolated, so this is
not proof of a specific car hit or collision-hit record.

Only fixture code changed; the preceding Mute City app packages remain
current. All process handles are terminal, no Simulator or VM was started,
and `git diff --check` is clean. Next verify rematches and targeted car
collision/crash behavior. Full stage/mode coverage, physical-device
validation and opt-in MetalFX remain unfinished.

Mute City rematch/lifetime correction: `--mute-city-rematch` exposed a
production teardown crash in `mutecity-rematch.log` while entering Sudden
Death. The prior external-spline owner retained live HSD joints across the
game's arena teardown. The archive destructor then attempted to release
those invalid runtime objects. Passing isolated archive cleanup tests was
insufficient to prove lifetime correctness across real scene transitions.

External spline owners now retain descriptors only. The referenced root's
`next` is detached from unrelated siblings because the AObj owns/unrefs
only its referenced root (and children), not a separate sibling tree.
The descriptor storage and spline data remain owned until archive disposal;
live joints are created/referenced through the game's AObj lifecycle.
This supersedes the earlier recommendation to keep the full external HSD
runtime tree alive inside the scene owner.

`mutecity-path-lifetime-build.log` builds, and
`mutecity-path-lifetime-tests.log` passes all 50 selected stage archive and
playback checks in 40.33 seconds. `mutecity-rematch-lifetime.log` passes at
6,000 submissions without sanitizer errors: first match at 1,587, Sudden
Death at 2,303, Results at 3,589, character selection at 4,022, stage selection
at 4,085, and second match at 4,270. The inspected
`mutecity-rematch-lifetime.png` shows both fighters at 20% with 1:33 remaining
in the second match. Both are in motion 184 at that sampled frame; damage
sources were not isolated. The stage countdown graphic is visible.

All three updated app packages build successfully in
`{mac,iphoneos,iphonesimulator}-mutecity-lifetime-app-build.log`. The device
package remains unsigned and requires provisioning. All process handles
are terminal, no Simulator or VM was started, and `git diff --check` is
clean. Targeted car collision/crash checks, remaining stages/modes,
physical-device validation and opt-in MetalFX remain unfinished.


### Big Blue assets and animated camera ownership (2026-09-11)

Big Blue now has an owned stage archive decoder for its 81 scalar hazard
words, 41 models, and empty item/particle banks. Model 31 uses camera eye
and interest animation, previously rejected by the stage loader. The new
MeleeCameraAnimation owner copies camera and world XYZ animation streams,
validates supported channels, and releases them with the stage model.
Spline paths and world constraints remain explicitly unsupported in this
camera decoder; general scene camera animation support is unchanged.

`bigblue-camera-build.log` passes. `bigblue-camera-tests.log` passes all
52 selected stage asset/playback checks (48.69 seconds). The additional
source-disposal camera test loads the actual Big Blue camera, plays eye
and target tracks across 7200 frames, requires finite coordinates and
movement for both, then checks HSD ID/AObj/FObj cleanup. Its build and
`bigblue-camera-motion-tests.log` pass (2/2, 3.74 seconds).

This is asset and animation proof, not a live Big Blue match. Runtime raw
Ground pointer offsets in grbigblue.c still need ARM64 conversion before
stage admission, live hazards, and rematch validation. Production app
packages still contain the last validated Mute City lifetime fix. MetalFX
remains deferred until core gameplay works, opt-in and off by default.


### Big Blue car runtime conversion (2026-09-11)

Converted native car-manager lane counting, closest-car selection, spawn
selection, direction counts, ordered pair separation, and collision impulse
access to grBigBlue_CarLane fields. The original PowerPC-only rlwimi state
writes now have native typed assignments for target state 10 and following
state 4. Retail code paths remain conditional and unchanged in behavior.
The car physics routine now references actual native collision jobjs and
lane arrays rather than a padded GameCube Ground overlay; it uses typed
state/direction/collision-slot fields and typed road current/previous Y.
The car acceleration/fade routine likewise uses native lanes instead of
its padded byte overlay. These changes do not yet admit Big Blue to play.

`bigblue-car-runtime-build.log` builds melee_game_startup and
melee_game_runtime successfully. `bigblue-car-runtime-tests.log` passes
native_bigblue_car_lanes with ASan enabled (leak detection disabled).
The deterministic test covers closest eligible selection, state-10 and
state-4 transitions, timer behavior, collision-slot/direction preservation,
a car within 60 units, an existing state-10 car, and no eligible cars.
It does not validate live physics, road motion, collisions, or rendering.
Remaining platform/manager raw offsets and pointer conversions must be
resolved before live stage admission and hazard/rematch validation.
Production app packages have not been rebuilt for these unfinished stage
changes. MetalFX remains queued after core gameplay.


### Big Blue platform and manager native state (2026-09-11)

The native flyer now owns typed state, timer, target rotation/height, and
speed fields. Manager event notifications use complete pointer slots and
preserve the optional joint pointer through grBigBlue_801E8978. Converted
all platform-manager data and joint accesses to the canonical manager
view, removing disagreement with the old anonymous overlay after pointer
widening. The manager item reference is now an Item_GObj pointer on native
builds. Platform slot selection and collision filtering use typed entries
instead of fixed 0x54 strides. The car collision-joint allocation is now
30 * sizeof(pointer), correcting the original 120-byte allocation on ARM64.

`bigblue-platform-final-build.log` successfully builds startup, runtime,
and the scene probe. `bigblue-platform-tests.log` passes 53/53 selected
stage asset/playback and native lane checks (50.45 seconds). This is not
live Big Blue platform, physics, collision, or rematch proof. Big Blue
stage admission and a live fixture remain next; production packages still
contain the last validated Mute City build. MetalFX remains deferred until
core gameplay support is working.


### Big Blue first live match (2026-09-11)

Added /GrBb.dat native archive admission, Big Blue stage-selection guidance,
and --big-blue startup fixture with live stage/fighter/mode assertions at
2800 submissions. The first run (`bigblue-live.log`) returned 6 because the
fixture had not unlocked Big Blue. Added an in-memory unlock using the
same hidden-stage bit mapping as existing fixtures; no save is written.
`bigblue-unlocked-build.log` builds successfully. `bigblue-unlocked.log`
passes 2800 submissions with two live fighters on Gr_Kind_BigBlue and no
reported sanitizer error. Final Fox damage 0, Ness 5; the damage cause was
not isolated. `bigblue-unlocked.png` was visually inspected: both fighters
are visible on the Falcon Flyer with ocean/track scenery, timer 1:41.07.
The framebuffer reports black=0. This is a real windowless Metal frame,
not a host desktop screenshot or a mockup.

Longer natural hazards, road/car/platform cycles, collision fidelity and
rematch teardown still require validation. No production app packages
were rebuilt for Big Blue yet. The full macOS/iOS objective and opt-in
MetalFX request remain unfinished.


### Big Blue natural track observation (2026-09-11)

Added --big-blue-track (7800 submissions, 360-second watchdog), using
read-only observers of map 33 cars, map 34 road, map 35 Falcon Flyer,
and map 32 platform manager. It requires road and car displacement over
10 units, Flyer states 3 and 0 (departure then absent), an active manager
platform (state 3), and car state 10. No timers, stage positions, damage,
or stage state are forced. The existing scripted player inputs remain.

`bigblue-track-build.log` builds successfully. `bigblue-track.log` passes
7800 submissions with no reported sanitizer errors. Flyer state 2 was
observed at 1575, state 3 at 2466, and state 0 at 3478; final accumulated
car states 0x7f2, flyer states 0xd, and platform states 0xf. Both road and
car movement assertions passed. Final live-stage/fighter assertions pass.
`bigblue-track.png` was inspected: tilted ocean/track backdrop, an active
platform with the stage sphere, both fighters on respawn platforms at 0%,
timer 0:17.59. This image is not evidence of damage-free play; the fighters
have respawned. This does not isolate hazard damage or prove every road,
car, or Flyer cycle. Flyer reentry, full collision fidelity, and rematch
teardown remain unverified. Production app packages remain unchanged.


### Big Blue rematch and app packages (2026-09-11)

Added --big-blue-rematch using the existing results-aware input helper
and stage-selection guidance reset, with a 6000-submission final live
Big Blue assertion. `bigblue-rematch-build.log` passes. The live run
`bigblue-rematch.log` passes with no reported sanitizer error: first VS at
1575, Results at 2293, CSS at 2726, SSS at 2790, and second VS at 3015.
Final 6000-frame capture `bigblue-rematch.png` was inspected: Fox and Ness
visible on the cars, timer 1:12.27, both at 0%. This verifies results,
cleanup/reload, and continued play in a second native match; it does not
prove all collision/hazard cases or full stage-cycle fidelity.

All three production packages rebuilt successfully after the test:
`mac-bigblue-app-build.log`, `iphoneos-bigblue-app-build.log`, and
`iphonesimulator-bigblue-app-build.log`. Outputs are the existing
`macosx-game/MeleeNative.app`, `iphoneos-game/MeleeNative.app` (unsigned),
and `iphonesimulator-game/MeleeNative.app` under native/build. These
builds do not constitute physical iPhone/iPad, Bluetooth-controller,
audio, or latest macOS GUI validation. MetalFX remains queued after core
gameplay; the overall requested port is still unfinished.


### Fourside archive conversion and collision flag access (2026-09-11)

Added the owned Fourside archive decoder: 19 parameter words, explicit
halfword conversion at 0x44/0x46/0x48 (and padding at 0x4a), seven models,
and absent item/particle banks. The archive test checks the first 68 bytes
and all four trailing halfwords after poisoning/freeing the original
archive, required symbols, and HSD ID/AObj/FObj cleanup. Stage playback
uses the existing animation traversal test.

`fourside-assets-build.log` passes; `fourside-assets-tests.log` passes
55/55 selected stage/Big Blue lane tests in 49.40 seconds. A subsequent
runtime review found grFourside_801F30A0 using M2C_FIELD at GameCube CollData
offset 0x34. Its native branch now reads x34_flags.b1234, preserving the
retail branch. `fourside-collision-build.log` builds startup and runtime.
The callback has not yet been exercised by a live Fourside fixture.
Fourside stage admission, live hazards, and rematch validation remain
next. Production app packages still contain the validated Big Blue build.


### Fourside first live native match (2026-09-11)

Added /GrFs.dat stage admission, Fourside selection guidance, and a
--fourside live startup fixture with an in-memory hidden-stage unlock.
The unlock does not write a save. `fourside-live-build.log` builds startup
and runtime successfully. `fourside-live.log` passes 2800 submissions:
Gr_Kind_Fourside, at least two fighters, active VS, and no match-over flag;
no reported sanitizer error. Final Fox and Ness are both at 0% (this does
not prove no earlier damage or deaths). `fourside-live.png` was inspected:
Fox and Ness visible on rooftops, illuminated city geometry and HUD,
timer 1:41.24, framebuffer black=0. This only establishes basic live
stage entry/rendering. Crane/UFO/helicopter behavior, collision callback
coverage, and results/rematch cleanup remain to test. Production packages
still contain the preceding validated Big Blue build.


### Fourside natural hazard observation (2026-09-11)

Added --fourside-hazards, a 7800-submission window with a 360-second
watchdog and read-only observers for crane (map 1), UFO (map 5), and
helicopter (map 3). It requires crane displacement above one unit and
natural activation of both UFO and helicopter. Timers, stage positions,
and damage are not forced. `fourside-hazards-build.log` builds;
`fourside-hazards.log` passes with no reported sanitizer errors.
UFO first appears at 1875, reaches state 2 at 3019, returns to state 0 at
4278; helicopter activates at 4279; UFO appears again at 5476. Final
state masks: crane 0xff, UFO 0x17, helicopter 0x7, crane movement true.
The live Fourside/fighter assertion also passes at 7800.
`fourside-hazards.png` was inspected: both fighters visible on rooftops,
helicopter visible on the right landing pad, part of UFO at upper right,
timer 0:18.07. Both fighters are at 0%; hazard damage and platform riding
are not isolated by this test. Full collision fidelity and rematch cleanup
remain unverified. Production packages still contain the Big Blue build.


### Fourside rematch and production packages (2026-09-11)

Added --fourside-rematch to the results-aware fixture and stage guidance
reset. `fourside-rematch-build.log` passes. `fourside-rematch.log` passes
6000 submissions with no reported sanitizer error: first VS 1575, Results
2292, CSS 2726, SSS 2789, second VS 2999. Final live Fourside assertion
passes. Inspected `fourside-rematch.png`: Fox and Ness on opposite rooftops,
UFO partially visible above, helicopter on landing pad, timer 1:12.00,
both fighters at 0%. This confirms stage reload and continued second-match
rendering, not full hazard/collision fidelity.

All packages rebuilt successfully: `mac-fourside-app-build.log`,
`iphoneos-fourside-app-build.log`, `iphonesimulator-fourside-app-build.log`.
Outputs remain native/build/{macosx-game,iphoneos-game,
iphonesimulator-game}/MeleeNative.app. The device build is unsigned and
needs development signing/provisioning. Physical device, latest Mac GUI,
Bluetooth/audio testing and other unsupported gameplay remain unfinished.
MetalFX remains deferred until core gameplay is working.


### Icicle Mountain initial audit (2026-09-11)

`icemt-model-playback.log` passes --stage-playback on GrIm.dat: all 9
models, 756 material descriptors, 20 light overrides, 7 point mappings,
source lifetime, and HSD cleanup. No live stage admission yet.
The parameter block at 0x87558 has mixed halfword/float fields and three
pointer slots at +0xac/+0xb0/+0xb4. They reference signed-halfword tables:
0x87508 (16 entries), 0x87528 (12), 0x87540 (12), each ending in -1.
The itemdata table references kind 0xd9 (Polar Bear/It_Kind_Whitebea),
article 0x9daa0; this needs stage article conversion. Spawn descriptors
begin at +0xbc and require byte/halfword-aware treatment, not blindly
converting the apparent trailing float fields in the decompiled type.

Changed fn_801F8E58 cooldown selection/decrement to index x18[i] with a
stable Ground pointer, removing artificial two-byte Ground pointer steps
and the Ground_GObj/Ground pointer alias. `icemt-cooldowns-build.log`
builds startup and runtime successfully. This loop is not yet exercised
in a live Icicle Mountain match. Other raw collision-table offsets and
pointer/scalar union layouts still need audit. Production packages remain
at the validated Fourside build. MetalFX and full platform validation are
still pending within the original goal.


### Polar Bear owned article (2026-09-11)

Added It_Kind_Whitebea to the shared item article decoder. Its native
attribute owner retains the referenced 20-byte common block, float fields,
and five signed halfword settings. The parent article owns all data after
source disposal; no pointer into disc bytes remains in the special attrs.
The GrIm article has eight animation records (distinct from the 12 runtime
motion callbacks, which reuse animation indices).

`whitebear-article-build.log` builds scene probe, startup, and runtime.
`whitebear-article-tests.log` passes 3/3: Polar Bear article plus Big Blue
and Fourside archive regressions (3.23 seconds). The Polar Bear test poisons
and frees source bytes, checks referenced/mixed-width attributes, loads
and animates all eight records, checks finite matrices and HSD ID/AObj/FObj
cleanup. `whitebear-common-article-tests.log` passes both common article
suites (ItCo.dat and ItCo.usd, 16.39 seconds).

This is article ownership and animation proof, not a live Polar Bear spawn
or complete Icicle Mountain support. Stage parameter tables, item admission,
runtime collision layouts, live scrolling, and rematch validation remain.
Production apps still contain the validated Fourside build.


### Icicle Mountain owned stage parameters (2026-09-11)

Moved grIceMt_YakumonoParam to its header for shared typed decoding.
Native builds use a 32-record spawn array beginning at the logical +0xbc;
the original struct's xBC and four following float fields underdescribe
the archive block. In GrIm.dat the block spans 0x87614..0x87693, before
the next referenced descriptor at 0x87694; Polar Bear records occur beyond
the old struct end. The native generator call now passes the owned array.

Added melee_icemt_stage_decode: mixed-width parameter prefix, three owned
signed-halfword tables (16/12/12 with terminal -1), xB8/padding halfwords,
32 byte/halfword spawn records, eight-animation Polar Bear article,
particle bank, and nine stage models. The existing BattleStage owner
releases the copied tables with all other stage resources.

`icemt-params-build.log` and `icemt-params-test-build.log` pass.
`icemt-params-tests.log` passes 58/58 selected stage and article checks in
56.21 seconds. The new archive test overwrites/frees original bytes, checks
all prefix fields, all three tables, all 32 spawn records (including
Polar Bear index 15), item/particle presence, and HSD cleanup.
This is archive proof only. Runtime collision table offsets/overlays,
stage admission, live scrolling, and rematch validation remain unfinished.
Production app packages still contain the validated Fourside build.


### Icicle Mountain collision storage and first live match (2026-09-11)

Native icemt1 now owns two separate seven-halfword collision records.
The retail layout stores these after a model-specific number of material
pointers; widening those pointers broke the old overlapping offsets.
Initialization, per-frame processing, and all three collision callbacks
now share icemt_collision_record. Retail offsets 0x100/0x108 select the
first record for different model kinds; 0x10e selects the second. Native
storage is separate from all 20 material pointers. Retail uses its old
byte offsets. `icemt-collision-build.log` passes.

Added GrIm.dat native admission, Icicle Mountain stage guidance, and
--icicle-mountain live fixture. `icemt-live-build.log` passes startup and
runtime builds. `icemt-live.log` passes 2800 submissions with two live
fighters on Gr_Kind_Icemt and no reported sanitizer error. The inspected
`icemt-live.png` shows Fox on a platform, Ness on a respawn platform,
snow/ice platforms and mountain backdrop, timer 1:42.07. Both are at 0%;
this does not establish damage-free play. No production packages rebuilt.
Longer scrolling, Polar Bear appearance, collision activation/fidelity,
and rematch validation still remain before claiming full stage support.


### Icicle Mountain scrolling and segment replacement (2026-09-11)

Added --icicle-mountain-scroll: 7800 submissions, 360-second watchdog,
read-only map-9 controller/segment observations and Polar Bear sightings.
It requires terrain displacement over 20 units and at least two segment
pair changes. It does not require a random Polar Bear appearance or force
stage timing/spawns. `icemt-scroll-build.log` builds successfully.
`icemt-scroll.log` passes 7800 submissions without reported sanitizer
errors. Terrain moved while the initial pair was still active (3000);
scroll speed changed sign. Segment pairs: (2,5) at 1608, (4,2) at 4511,
(1,4) at 4982, (5,1) at 5337. Final changes=3, moved=1, bear=0.
`icemt-scroll.png` was inspected: both fighters visible on a platform in a
different mountain segment, timer 0:18.73, both at 1%. Damage was not
isolated. No Polar Bear was observed; live Polar Bear behavior, targeted
collision activation and rematch cleanup remain unverified. Production
apps still contain the validated Fourside build.


### Icicle Mountain rematch and app packages (2026-09-11)

Added --icicle-mountain-rematch to the results-aware fixture, stage
guidance reset, and 6000-submission live assertion. Build log
`icemt-rematch-build.log` passes. `icemt-rematch.log` passes without
reported sanitizer error: first VS 1610, Results 2323, CSS 2757, SSS 2820,
second VS 3025. Final live stage assertion passes. The inspected
`icemt-rematch.png` shows Fox on a respawn platform and Ness near the
bottom edge, mountain geometry, timer 1:12.42, both at 0%. This verifies
stage teardown/reload and continued rendering; it does not prove every
collision case, post-replacement teardown, or live Polar Bear behavior.

All production packages rebuilt successfully in `mac-icemt-app-build.log`,
`iphoneos-icemt-app-build.log`, `iphonesimulator-icemt-app-build.log`.
Outputs remain native/build/{macosx-game,iphoneos-game,
iphonesimulator-game}/MeleeNative.app. Device app remains unsigned.
Physical iPhone/iPad, latest Mac GUI/input, Bluetooth/audio verification,
remaining unsupported gameplay, and opt-in MetalFX remain unfinished.


### Icicle Mountain controlled Polar Bear runtime (2026-09-11)

Replaced three truncated u32 pointer tests in itwhitebea.c with full-width
null comparisons. Added --icicle-mountain-bear, using the original
it_8027B5B0 spawn path after entering Icicle Mountain. This is an explicit
controlled spawn near a fighter, not proof of natural generator timing.
The fixture requires a successful spawn, at least 60 active observations,
over 10 units of displacement and multiple motion states, then continues
through 5000 submissions. Motion-mask indexing guards both bounds.

Builds pass in icemt-bear-build.log, icemt-bear-guard-build.log and
icemt-bear-capture-build.log. First live run icemt-bear.log passes with
1565 active frames, motions ef (0,1,2,3,5,6,7), displacement observed.
Updated capture run icemt-bear-capture.log also passes: spawn at 1809,
661 active frames, same motion mask, displacement observed. Neither log
reports sanitizer errors. The inspected frame-2000 image
icemt-bear-capture.png.bear.png shows the bear on the right-hand platform,
Ness nearby and Fox above it, timer 1:55.56. This is a direct framebuffer
capture independent of desktop lock state. Both final images were also
inspected. Capture-run final timer 1:05.49, Fox 4%, Ness 0%; damage was
not isolated to the bear. Natural spawning, targeted damage/defeat behavior,
all collision states and post-segment-replacement teardown remain unproven.

All three packages rebuild successfully in mac-icemt-bear-app-build.log,
iphoneos-icemt-bear-app-build.log and
iphonesimulator-icemt-bear-app-build.log. Device package remains unsigned.
No new physical-device, Mac GUI/input, Bluetooth or audio proof. Remaining
unsupported gameplay and opt-in MetalFX are still unfinished. MetalFX is
recorded above as off by default, capability-gated, with persistent settings
on macOS/iPhone/iPad after core gameplay is complete.


### Icicle Mountain teardown after terrain replacement (2026-09-11)

Strengthened --icicle-mountain-rematch to require the scrolling observer's
movement and two segment-pair changes before shortening the first match
at submission 6200. Its final second-match assertion and capture now occur
at 10000; watchdog is 440 seconds. Earlier 6000-frame results remain valid
only for the earlier fixture. This changes test behavior only.

icemt-scroll-rematch-build.log passes. icemt-scroll-rematch.log passes all
10000 submissions without reported sanitizer errors. First VS 1608;
segment pairs (5,6), (3,5) at 2069, (2,3) at 4489, (3,2) at 5762;
Results 6641, CSS 7074, SSS 7138, second VS 7343. Final assertion verifies
Icicle Mountain, two live fighters, no match-over flag. The inspected
icemt-scroll-rematch.png shows the stage and fighter markers, timer 1:17.76,
Fox 3%, Ness 2%. Damage sources and all collision cases remain unisolated.
This extends cleanup evidence across terrain replacement, results, reload
and another 2657 submissions. No production code changed this turn; app
packages remain the successful icemt-bear builds. Full objective remains
unfinished, including remaining stages/modes, physical-device validation,
latest Mac GUI/controller/audio checks and opt-in MetalFX.

Next-stage inspection: GrFz.dat (Flat Zone) extracted to native/build.
map_head 0x3e0 describes eight models. yakumono_param 0x47908 has the
16 scalar words declared by grFlatzone_YakumonoParam. itemdata 0x47900
points to entry 0x49298: kind 0xe6 (Tools), article 0x49280 (common 0x48ffc,
special 0x49080, hurtbones 0x491d8, states 0x491e0, model 0x491c8, no
dynamics). Tools has ten runtime animation entries and five selectable
shapes; itToolsAttributes declares a one-element trailing motion array
although runtime indexes shapes 0..4. Flat Zone native admission, owned
decoding, source-free playback and live validation are not implemented.


### Flat Zone owned stage and tool decoding (2026-09-11)

Native itToolsAttributes now exposes all five trailing motion records;
the retail declaration is preserved. Added the 156-byte Tools scalar
schema: three leading floats, one integer, five seven-float motion records.
All floats require finite values. --tools-article checks every decoded word
against GrFz.dat, then erases/frees the source and plays all ten animations
with finite joint matrices and clean HSD allocation counts. It additionally
rejects a truncated parameter region and a nonfinite parameter.
flatzone-tools-build.log passes; flatzone-tools-tests.log passes 4/4 tests
including ItCo.dat/usd articles and Polar Bear (20.36 seconds).

Added melee_flatzone_stage_decode with 16 scalar hazard words, Tools
article (ten animation records), eight models, collision, scripts, lighting
and quake ownership. Flat Zone has no particle bank. --flatzone-archive
checks all 16 parameters, model count, item presence, required public
symbols and cleanup after source erasure/free. --stage-playback covers
its model animations separately. Build passes in flatzone-archive-build.log.
flatzone-archive-tests.log passes all ten focused tests (20.47 seconds):
Flat Zone archive/playback, Tools, Icicle Mountain, Polar Bear, Fourside,
and Big Blue. These prove archive ownership and playback only.

Flat Zone is not yet admitted through grDatFiles or exercised in a live
match. Next work is native stage admission, stage selection fixture/unlock,
live gameplay and falling-tool behavior, then rematch cleanup. App packages
remain the icemt-bear builds; no app rebuild or device test this turn.
Full port, hardware/input/audio validation and opt-in MetalFX remain open.


### Flat Zone live stage loading and ARM64 hazard fields (2026-09-11)

Added GrFz.dat native admission, Flat Zone stage-selection guidance and
--flat-zone live fixture. The fixture unlocks Flat Zone only in memory,
without writing a save. Its final assertion requires the correct stage,
two fighters, VS gameplay and no match-over flag at submission 2800.
flatzone-live-build.log and flatzone-live.log pass. The initial inspected
image shows Fox and Ness on Flat Zone, timer 1:42.00, both 0%.

Source review found grFlatzone_802176BC mixing the pointer-free flatzone
and generic unk overlays with flatzone2, which contains a widened xCC
pointer. State reads/writes and timer accesses now use flatzone2.xD0 and
flatzone2.timer consistently. Retail fields retain equivalent offsets.
flatzone-state-build.log and flatzone-state-live.log pass. The corrected
run reaches 2800 submissions without reported sanitizer errors; inspected
flatzone-state-live.png shows Fox on an upper platform, Ness below,
Game & Watch background and platforms, timer 1:42.22, both at 0%.
These short runs do not prove falling-tool or oil-hazard cycles, isolated
collision/damage behavior, or rematch cleanup. Those remain next work.

All app packages rebuilt successfully: mac-flatzone-app-build.log,
iphoneos-flatzone-app-build.log, iphonesimulator-flatzone-app-build.log.
Outputs remain native/build/{macosx-game,iphoneos-game,
iphonesimulator-game}/MeleeNative.app. Device package remains unsigned.
No new physical-device, Mac GUI/input, Bluetooth or audio proof. Full
native gameplay coverage and opt-in MetalFX remain unfinished.


### Flat Zone natural hazard cycles (2026-09-11)

Added --flat-zone-hazards, a 7800-submission windowless fixture with a
360-second watchdog and read-only observations. It requires a live match,
a falling tool, an oil spill state and a non-null dynamic spill attribute.
Hazard timing, shape selection and spawn behavior are not overridden.
flatzone-hazards-build.log passes. flatzone-hazards.log passes all 7800
submissions without reported sanitizer errors. Oil states: idle at 1610,
0 at 4567, 1 at 4631, 2 at 4751, 3 at 4797, 4 at 5995, idle at 5996.
A non-null spill attribute was observed in state 3. Falling shapes first
seen: 4 at 6982, 0 at 7042, 2 at 7101, 1 at 7161, 3 at 7281. Final masks
shapes=1f, motions=3ff, oil=1f, spill=1: all five shapes and ten tool motion
states were observed. No controlled spawns were used.

Inspected flatzone-hazards.png shows both fighters on the bottom surface,
timer 0:18.81 and both at 10%; final image does not isolate a tool or oil
spill. Damage sources, collision fidelity and rematch cleanup remain
unverified. This turn changed only fixture code; app packages remain the
successful Flat Zone builds. Full objective, hardware validation and
opt-in MetalFX remain unfinished.


### Flat Zone results and rematch (2026-09-11)

Added --flat-zone-rematch with results-aware input, stage-guidance reset,
and final live-stage assertion at 6000 submissions. The fixture shortens
the first match at 1850; it does not verify teardown after late hazards.
flatzone-rematch-build.log passes. flatzone-rematch.log passes without
reported sanitizer errors: first VS 1609, Sudden Death 2267, Results 3579,
CSS 4013, SSS 4076, second VS 4282. Final second-match assertion passes.
Inspected flatzone-rematch.png shows Fox and Ness on the bottom surface,
Game & Watch background/platforms, timer 1:33.42, both at 0%. This covers
stage teardown/reload and continued rendering. Collision fidelity and
post-hazard teardown remain unverified. Production app packages are still
the validated Flat Zone builds; only test code changed this turn.

Next-stage inspection: Brinstar Depths is GrKr.dat, extracted to
native/build/GrKr.dat. map_head 0x208 has five models, itemdata 0x74fec is
empty. yakumono_param 0x74ff0 contains 13 scalar words matching
 grKraid_YakumonoParam (seven scalar fields, six floats for Kraid X).
map_ptcl 0x6d3c0, map_texg 0x6d6c0, map_plit 0x750e8,
ALDYakuAll 0x75040, quake_model_set 0x10d9e8. Native decoder/admission,
rotating collision geometry and Kraid behavior remain to be implemented
and validated. Full objective and opt-in MetalFX remain unfinished.


### Brinstar Depths owned archive and initial live match (2026-09-11)

Added melee_kraid_stage_decode using 13 scalar hazard words, five models,
empty item table, collision, scripts, particles, lighting and quake owner.
--kraid-archive verifies all parameters and required public symbols after
source erasure/free, with clean HSD allocation counts. Separate model
playback test included. kraid-archive-build.log passes;
kraid-archive-tests.log passes 11/11 focused tests (22.83 seconds), covering
Brinstar Depths, Flat Zone, Icicle Mountain, Polar Bear, Fourside and Big Blue.

Added GrKr.dat native admission, Kraid stage guidance and --brinstar-depths
live fixture with in-memory-only stage unlock. kraid-live-build.log and
kraid-live.log pass. At 2800 submissions there are two live fighters on
Brinstar Depths, not match-over; no reported sanitizer error. Inspected
kraid-live.png shows angled terrain and both fighters, timer 1:41.44,
Fox 0%, Ness 20%. The image alone does not prove correct rotating collision
or Kraid interaction; longer instrumented hazard tests and rematch cleanup
remain required. Damage sources were not isolated.

All packages rebuilt successfully in mac-kraid-app-build.log,
iphoneos-kraid-app-build.log and iphonesimulator-kraid-app-build.log.
Outputs remain native/build/{macosx-game,iphoneos-game,
iphonesimulator-game}/MeleeNative.app; device package remains unsigned.
No new physical-device, Mac GUI/input, Bluetooth or audio validation.
Remaining unsupported gameplay and opt-in MetalFX remain unfinished.


### Brinstar Depths Kraid cycles and terrain rotation (2026-09-11)

Added --brinstar-depths-hazards: 7800 submissions, 360-second watchdog,
read-only map-4 Kraid state/attack observations and map-3 terrain rotation.
Requires all five states, at least two returns from retreat to idle, and
a terrain joint rotation change exceeding 0.25 radians. No hazard timing,
attack selection or spawn behavior is overridden.

kraid-hazards-build.log passes. kraid-hazards.log passes all 7800 submissions
without reported sanitizer errors. Final states=1f, attacks=7, cycles=4,
terrain_rotated=1. All three attack animations were observed and four full
cycles completed. Inspected kraid-hazards.png shows both fighters on the
terrain, timer 0:18.14, Fox 0%, Ness 20%. The final image does not show
Kraid itself. This proves runtime cycles and joint rotation, not every
collision response or damage source. Rotating collision fidelity and
rematch teardown remain open. This turn changes test code only; app
packages remain the successful kraid builds. Full goal, physical hardware,
latest Mac input/audio/controller validation and MetalFX remain unfinished.


### Brinstar Depths rematch cleanup (2026-09-11)

Added --brinstar-depths-rematch with results-aware input, stage-guidance
reset and final live-stage assertion at 6000 submissions. The fixture
shortens the first match at 1850; it does not assert a full Kraid cycle
before teardown. kraid-rematch-build.log passes. kraid-rematch.log passes
without reported sanitizer errors: first VS 1574, Results 2291, CSS 2724,
SSS 2787, second VS 2982. Inspected kraid-rematch.png shows both fighters
on the angled terrain, timer 1:11.73, Fox 0%, Ness 15%. This verifies
teardown/reload and continued live rendering. Damage sources and rotating
collision fidelity remain unisolated. Only fixture code changed this turn;
production packages remain the successful kraid builds.

Next-stage inspection: Rainbow Cruise GrRc.dat extracted to native/build.
map_head 0x794 describes seven models, two cameras and five other records.
itemdata 0x74df8 is empty; yakumono_param 0x74dfc has 18 scalar words
matching grRCruise_YakumonoParam. dynamicsdata_shipflag at 0x1a4 needs
owned decoding; it is not one of the current fixed flag3/4/6 names.
map_plit 0x74ecc, quake_model_set 0xe2b48, coll_data 0x74a98.
Archive public symbols contain no map_ptcl/map_texg. Native stage decoder,
ship-flag dynamics, live traversal/collisions and rematch remain undone.
Full goal and opt-in MetalFX remain unfinished.


### Rainbow Cruise owned archive and ship flag (2026-09-11)

Added melee_rcruise_stage_decode with 18 scalar hazard words, seven models,
empty item table and no particle bank. Collision, cameras/model animations,
scripts, lighting and quake data use the existing owned decoders. The
archive bridge now has a fourth flag slot for dynamicsdata_shipflag,
including cleanup and public lookup. Its six DynamicsDesc parameter records
(15 finite floats each) and position are decoded into owned native storage.
Existing castle flag3/4/6 ownership remains in the original slots.

--rcruise-archive compares all 18 stage words, all 90 flag parameter words
and three position words after erasing/freeing the source archive. It also
checks model count, empty item/particle data, required public symbols and
HSD cleanup. Separate --stage-playback exercises model animations.
rcruise-archive-build.log passes all requested build targets.
rcruise-archive-tests.log passes 15/15 focused tests in 28.17 seconds,
including Rainbow Cruise, castle flag regressions, Brinstar Depths,
Flat Zone, Icicle Mountain, Polar Bear, Fourside and Big Blue.

This establishes owned decoding/playback only. Rainbow Cruise is not yet
admitted through grDatFiles, and no live traversal, ship-flag simulation,
collision or rematch proof exists. App packages remain the kraid builds.
Next work is stage admission and live runtime fixes, followed by longer
traversal and rematch tests. Full goal and opt-in MetalFX remain unfinished.


### Rainbow Cruise native setup fixes and first live match (2026-09-11)

Added GrRc.dat admission, Rainbow Cruise stage guidance and --rainbow-cruise
fixture. Default stage needs no unlock. rcruise-live-build.log passes, but
rcruise-live.log initially crashes in OSAllocFromHeap during setup.
Moving-platform records were allocated via Map_Chikuwa's fixed 0x198-byte
retail buffer and the unrelated map overlay; disappearing-platform records
were allocated using descriptor size instead of native entries with pointers.
Now allocate 17 typed grRCruise_Entry records and ARRAY_SIZE(lbl_803E5014)
typed Map_VanishEntry records; all accesses use the rcruise owner fields.
The vanish field's pointee is corrected, and the platform index table has
an explicit 17-element declaration available at the allocation site.

rcruise-layout-build.log passes; rcruise-layout-live.log passes the previous
failure but crashes at the scrolling-camera object reference. Stage setup
stored that reference through rcruise2.xEC, which no longer overlaps
scroll.anim_gobj after native pointer widening. It now writes the latter
field directly. rcruise-scroll-build.log and rcruise-scroll-live.log pass
2800 submissions without reported sanitizer errors. Both fighter records
are active at 0% on the correct stage. Inspected rcruise-scroll-live.png
shows the ship, landscape, Ness and HUD, timer 1:41.88; Fox is not clearly
visible in the image. Full scrolling traversal, platform/collision fidelity,
flag motion and rematch cleanup remain to be verified.

All app packages rebuilt successfully: mac-rcruise-app-build.log,
iphoneos-rcruise-app-build.log, iphonesimulator-rcruise-app-build.log.
Outputs remain native/build/{macosx-game,iphoneos-game,
iphonesimulator-game}/MeleeNative.app; device build remains unsigned.
No new physical-device, Mac GUI/input, Bluetooth or audio proof.
Full objective and opt-in MetalFX remain unfinished.


### Rainbow Cruise scrolling and disappearing platforms (2026-09-11)

Added --rainbow-cruise-traverse: 7800 submissions, 360-second watchdog,
read-only map-3 scrolling position and map-1 first-eight vanish-entry
observations. Requires camera displacement over 100 units and at least
eight state transitions. Route timing and platform behavior are unchanged.
rcruise-traverse-build.log passes. rcruise-traverse.log passes all 7800
submissions without reported sanitizer errors. Camera passes (212,-0.3),
(-431,57), (-221,262), (127,211). First platform transitions begin at 4453;
final states=f, changes=32, moved=1. All four disappearing-platform states
were observed. Both fighters remain active on the stage at final assertion.
This does not prove a complete return to the initial ship position, all
platform collision responses, or ship-flag simulation fidelity.

rcruise-traverse.png was inspected. Rematch cleanup remains next work.
This turn changes only test code; app packages remain rcruise builds.
Full native port, hardware/input/audio verification and MetalFX remain open.


### Rainbow Cruise rematch and menu-fixture limitation (2026-09-11)

Added --rainbow-cruise-rematch with results-aware input, stage-guidance
reset and second-live-match assertion at 6000 submissions. Build passes in
rcruise-rematch-build.log. First run rcruise-rematch.log exits 6 at 1850
because timed menu inputs entered other modes (20 then 32), not an active
match. That failed run is retained; it provides no stage teardown evidence
and exposes remaining timing sensitivity in the generic menu fixture.

A separate run rcruise-rematch-retry.log passes all 6000 submissions without
reported sanitizer errors: first VS 1602, Results 2319, CSS 2752, SSS 2815,
second VS 3013. Inspected rcruise-rematch-retry.png shows both fighters on
platforms against the rainbow background, timer 1:12.22, both 0%. This
supports stage teardown/reload and subsequent scrolling, not deterministic
menu navigation, every collision case or teardown after the full route.
Only fixture code changed; production packages remain the rcruise builds.

Next-stage inspection: Poke Floats uses GrPu.dat, extracted to native/build.
map_head 0x7cd90 has 28 models; itemdata 0xed2b0 is empty. yakumono_param
0xed2b4 starts with zero and is assigned but otherwise unused in grpura.c;
the following words appear to be unrelated script data, not more parameters.
map_plit 0xed340, coll_data 0xed018, grGroundParam 0xed1d4,
quake_model_set 0x4a328. No particle bank public symbols were listed.
Native Poke Floats decoding/admission and moving-platform gameplay are next.
Full goal and opt-in MetalFX remain unfinished.


### Poke Floats owned archive (2026-09-11)

Added melee_pura_stage_decode: 28 models, empty item table, no particle
bank, one placeholder hazard word. grpura.c assigns yakumono_param but
never reads it; following script data is not treated as extra parameters.
Existing owned collision, model/camera animation, scripts, lighting and
quake decoding is reused. --pura-archive verifies model count, placeholder,
empty item/particle data, required symbols and HSD cleanup after erasing
and freeing source bytes. Separate --stage-playback covers model animations.
pura-archive-build.log passes all requested targets. pura-archive-tests.log
passes 17/17 focused tests in 31.96 seconds, including Poke Floats and
Rainbow Cruise/castle flag regressions plus prior stages.

Poke Floats has not yet been admitted into grDatFiles or exercised in a
live match. Next is native stage selection/loading, followed by moving
platform and camera-subject behavior and rematch validation. Production
app packages remain rcruise builds. Full goal and opt-in MetalFX remain open.


### Poke Floats initial native match (2026-09-11)

Added GrPu.dat native admission, Pura stage-selection guidance and
--poke-floats fixture. Stage unlock is in-memory only and writes no save.
pura-live-build.log passes. pura-live.log passes 2800 submissions without
reported sanitizer errors, with two active fighters on Gr_Kind_Pura and
no match-over flag. Inspected pura-live.png shows Squirtle and Onix
platforms against the sunset background, Ness visible and Fox partly at
the lower edge; timer 1:41.64, both 0%. Onix has a speckled appearance;
visual fidelity of that material has not been compared with retail.
This short run does not prove all platform cycles, camera-subject behavior,
collision fidelity, or rematch cleanup. Those remain next work.

All app packages rebuilt successfully in mac-pura-app-build.log,
iphoneos-pura-app-build.log and iphonesimulator-pura-app-build.log.
Outputs remain native/build/{macosx-game,iphoneos-game,
iphonesimulator-game}/MeleeNative.app; device package remains unsigned.
No new physical-device, Mac GUI/input, Bluetooth or audio verification.
Full goal and opt-in MetalFX remain unfinished.


### Poke Floats platform/camera-subject sequence (2026-09-11)

Added --poke-floats-platforms, 7800 submissions with 360-second watchdog.
The read-only observer checks map-4 camera subjects for finite positions,
activation transitions and displacement over 20 units. It requires at
least eight subjects active during the run, eight moved, eight transitions.
pura-platforms-build.log passes. pura-platforms.log passes all 7800
submissions without reported sanitizer errors. Final active_seen=1c01d52
(10 subjects), moved=1c01d56 (11 subjects), transitions=17. Inspected
pura-platforms.png shows Sudowoodo/Wooper platforms and fighter markers,
timer 0:17.79, both 0%. This does not prove a full 25-subject cycle, all
moving collision responses, or rematch cleanup. Production packages remain
pura builds; this turn changes only test code.

Material investigation for next work: grpura.c grPu_803E6E20 is a compiled
u16[1024] RGB565 texture, referenced directly by grPu_803E7620 and passed
to HSD_MObjSetToonTextureImage in fn_802130D0. Native texture decoding reads
format-4 pixels as big-endian bytes, but compiled u16 storage is native
little-endian. This is a likely toon-texture byte-order defect requiring
an explicit byte-preserving native conversion and verification. It has not
been fixed yet, and is not established as the cause of Onix speckling.
Full goal, hardware verification and opt-in MetalFX remain unfinished.


### Poke Floats compiled toon-texture byte order (2026-09-13)

Added grPura_NativeToonImage, converting the compiled u16 RGB565 values
into aligned, persistent big-endian bytes before the stage supplies its
toon image. Retail path remains unchanged. --pura-toon/native_pura_toon
runs the real texture decoder and compares all 1024 RGBA pixels against
independent RGB565 channel expansion in GX tile order. It also checks
32-byte alignment, descriptor metadata and repeated initialization.
pura-toon-build.log and pura-toon-test.log pass. An initial compile failure
from a missing complete HSD_ImageDesc include was corrected before testing.

pura-toon-live.log passes 2800 submissions without reported sanitizer
errors. Inspected pura-toon-live.png shows Squirtle/Onix and Ness, timer
1:41.76. Onix remains speckled, so this validated byte-order correction
must not be claimed to solve that material appearance. Further visual
investigation and rematch/full-cycle checks remain.

All app packages rebuilt successfully in mac-pura-toon-app-build.log,
iphoneos-pura-toon-app-build.log and iphonesimulator-pura-toon-app-build.log.
Device app remains unsigned. No new physical-device, Mac GUI/input,
Bluetooth/audio proof. Full native gameplay and opt-in MetalFX remain open.


### 2026-09-13: user-reported Classic startup crash

Two real macOS MeleeNative crash reports in DiagnosticReports/Retired
(2026-09-11-210412 and -210506) identify an ASan read in
`gm_Mode_Classic_OnLoad`, formerly gmclassic.c:701. The native build now
references the actual matchup global and actual order buffer, instead of
assuming adjacency to the scene table and intro buffer. Native shuffling
swaps within its explicit order slice. Retail expressions remain unchanged.

The windowless menu-to-Classic test exposed two subsequent memory errors:
- A fixed 0x78-byte shared single-player buffer overflowed when storing native
  callback pointers. Native storage now uses the complete UnkAllstarData type.
- Trophy initialization copied beyond the small first global of a retail
  contiguous state block. Native Toy26B8 owns the complete session, with the
  named trophy flag array and animation aliases sharing that same object.

`classic-entry-checked.log` passed at 1800 submissions, Classic mode 3/state
112, with all four matchup permutations valid and no sanitizer error.
`classic-entry-checked.png` was visually inspected: genuine rendered Classic
character selection, Very Easy/three stocks. The earlier trophy-entry run
reached 1687 submissions but hit its original 30-second watchdog; the checked
fixture uses a 120-second allowance (later extended to 180 for match testing).
Reproduce with `--classic-entry` on melee_game_startup using the same disc,
cache, and output arguments as other windowless probes.

**Classic gameplay is not yet working.** `--classic-match` selects Fox and
starts through the real menus. `classic-first-match.log` reached submission
2203 then asserted in lbArchive_InitializeDAT via fn_80186634: GmIntEz.dat is
still being passed to the retail, big-endian parser. GmIntEz's layout table,
IrAls/IrEzTarg/IrEzTuki/IrRdMap scene data, and the intro's demo fighter data
need native decoding and validation. Do not describe the startup fixes as a
completed single-player port, and do not skip the intro merely to hide this
failure. Adventure, All-Star, and a full Classic run remain unverified.

Build logs for packages containing the startup fixes are
`mac-classic-startup-app-build.log`, `iphoneos-classic-startup-app-build.log`,
and `iphonesimulator-classic-startup-app-build.log` (check terminal results).


### 2026-09-13: Classic intro layout and trophy name tables

Added `gmIntro_NativeDecodeLayout` in gm_1832.c: owned 0x9B8-byte typed
ClassicIntroLayout, relocation/external rejection, bounded scalar reads,
finite-value validation, and commit only after full decode succeeds. The native
intro uses it instead of the retail parser for GmIntEz.dat. Retail loading is
unchanged. `classic-layout-test.log` passed all 622 word comparisons,
source-erasure ownership, truncation rejection, and nonfinite rejection while
preserving previously valid output. Command: melee_game_startup
`--classic-layout native/build/GmIntEz.dat` (fixture extracted with inspect-disc).

`classic-layout-live.log` passed the previous parser failure and found another
ASan global read overflow in tyDisplay_8031C454: trophy archive names were read
via a synthetic three-table global block. Native code now copies the actual
archive-name table; the analogous later material-name lookup also references
its actual table. `classic-trophy-names-live.log` reached the next failure at
submission 2203: lbArchive_LoadSymbols in tyDisplay_8031C454 still sends trophy
graphics through the retail archive parser (reported file size 0x215AC).
This test terminated with assertion/exit 134; it is NOT a passing intro or match.
The final source compiled in `classic-intro-progress-build.log`.

Next: implement owned native trophy graphics archive decoding used by
Ground_801C5878 even during the dummy intro stage, then the IrAls/IrRdMap intro
scenes and embedded demo motions. IrAls.dat was extracted/inspected: one
ScItrAllstar_scene_data plus 27 ftDemoIntroMotionFile* nested DAT symbols,
no externals. Existing melee_scene_desc_decode can be investigated for scene
models/cameras/lights/fog; it currently supports restricted animation schemas.
Current distributable app packages still contain the previous startup fixes;
these newer intro changes are in the built native runtime/probe only.


### 2026-09-13: native trophy graphics archives

Added melee_trophy_decode alongside the menu model adapter. Trophy archives
own serialized bytes, lazily decode joint/optional animation bundles, and
release temporary runtime HSD objects while retaining descriptors. The native
lbArchive_LoadSymbols path routes trophy model archive families (TyMyc, TyMap,
TySeri, TyEtc, TyPoke, TyItem, TyStandD, TyQuesD) through this adapter. Other
Ty archives retain their existing loaders. tyDisplay_8031C8B8 now destroys
native trophy archives when clearing its cache during scene preload.

The new hsd_scene_probe --trophy-archive fixture frees source bytes, loads each
public Top_joint, samples animation, removes objects, destroys the adapter,
and checks HSD allocation counts. trophy-archives-tests.log passed all 42
unique trophy archive filenames from the original table. Main menu regression
(trophy-menu-regression.log) passed its existing 57 model checks. Final cleanup
code builds in trophy-cleanup-build.log. Per-trophy fixture currently permits
absent animation descriptors; further visual fidelity and full scene teardown
coverage still required.

classic-trophies-live.log progressed past trophy initialization and now fails
in lbArchive_80016DBC directly from fn_80186634 (symbol offset +444), loading
the intro scene archive. No passing first-match claim: this run terminated
134 at submission 2203. Next implement the IrAls/IrRdMap scene archive adapter
and embedded demo animation handling. Current app packages predate these
changes; updated native runtime/probe are built. No live test remains running.


### 2026-09-13: native intro scene archives

SceneDesc conversion now preserves empty fog-animation lists (two null pointer
fields per record), alongside its existing empty camera/light animation lists.
Nonempty fog tracks still reject. Both IrAls and IrRdMap scene tests pass 120
animation frames after source disposal (intro-scene-test.log and
intro-map-test.log). Existing HUD/prize/pause/coget regressions pass 7/7 in
intro-scene-regressions.log.

Added melee_intro_decode, an owned HSD archive bridge in scene_desc.c, exposing
ScItrAllstar_scene_data and validated serialized nested ftDemoIntroMotionFile*
DAT payloads. The lbArchive_80016DBC native route caches the five named intro
archives for runtime lifetime; descriptors contain no live HSD objects. Archive
checks --intro-archive passed IrAls (27 nested motions), IrRdMap, IrEzTarg,
IrEzTuki and IrEzFigG with source erasure, repeated lookups, model instantiation
and cleanup. Logs: intro-archive-test.log, intro-map-archive-test.log,
intro-variants-tests.log. Builds: intro-archive-build.log and
intro-archive-check-build.log. Initial missing HSD_Archive declaration was fixed
by including the archive header.

classic-intro-archive-live.log now loads the intro scene and progresses to
creating demo fighters. It terminates 134 at ftData_80085B98:2236,
fp->ft_data->x14 is NULL (Native demo motion data is not installed), from
ftDemo_CreateFighter -> Player_80036F34 -> fn_80185E34. Next install intro motion
data from the embedded archives into native fighter demo motion descriptors,
following existing native results-motion installation. No rendered intro or
live Classic match has passed yet. These changes are built in the runtime and
probe; app packages still predate intro work. No live process remains.


### 2026-09-13: Classic reaches its first native match

Added melee_intro_motion_bytes with a span bounded by the next public symbol,
and melee_fighter_load_intro, installing original demo table entries 10/11.
Native ftDemo_SetArchiveData for intro archives installs these before creating
fighters. Motion owners cache by character/source (bounded five source slots),
remain separate from results-motion owners, and reassign the active x14 table
when requested. --intro-motions passed all 27 character sets, preserving index
slots and scripts after source disposal and rejecting invalid ranges. Logs:
intro-motions-tests.log; results regressions intro-results-regressions.log
27/27 pass. Build logs: intro-motion-build.log/intro-motion-probe-build.log.

The initial live test (classic-intro-motions-live.log) reached the intro frame
loop but found an ASan global overflow in character-name sizing,
fn_80160DE8. Native references to +0x21/+0x42/+0x63 beyond lbl_803B75F8 now use
the actual three corresponding global arrays. Retail expressions unchanged.

classic-intro-capture.log PASSED at 3800 submissions, mode 3/state 1, two live
fighters, no sanitizer error. The real flow navigated menus, selected Fox,
started Classic, ran the intro and entered its first round. Final image
classic-intro-capture.png was inspected: Fox versus Donkey Kong at Kongo
Jungle, timer 4:37.66, both 0%, Fox three stocks and DK one. This image is the
final gameplay capture (it overwrote the earlier frame-2400 checkpoint).
This proves the first-match entry, not a complete Classic run or later round
transitions. Next test round completion/results/next intro, then other 1P modes.

Mac, iPhone device and Simulator packages are being rebuilt in
mac-classic-first-match-app-build.log, iphoneos-classic-first-match-app-build.log
and iphonesimulator-classic-first-match-app-build.log; check terminal outcomes.


### 2026-09-13: Classic first-round completion probe

Added --classic-round: uses the real menu/intro path, confirms first-round play
at frame 3800, then places opponents below the blast zone at 3900 to exercise
the normal KO/round-ending code. It does not prove combat input or natural
victory. Logs mode/state changes and expects second-round mode 3/state 9 by
6500 submissions, with a 300-second watchdog.

classic-round.log exposed a SEGV in ifMagnify_802FC750: off-screen fighter
indicator cleanup cast its base pointer to u32. Native cleanup now uses the
typed player[i].gobj directly. Also audited the imminent 1P results callback
chain: fn_8016C46C, its wrapper and Classic/Adventure/All-Star handlers carried
a MatchEnd pointer in int. Parameters and the two call-site casts now use
intptr_t, preserving pointer width throughout. Build passed:
classic-round-pointers-build.log.

classic-round-pointers.log progressed through the KO and cleanup/results
callbacks, then asserted in lbArchive_80016DBC via fn_80180630 (gmregclear.c)
at submission 4013: single-player score display assets still use the retail
archive parser (reported archive size 0x2D555). This is not a passing round-two
transition. First stage in this run was Rainbow Cruise; earlier baseline used
Great Bay. No full Classic claim. Next convert the score display loaded by
fn_80180630, then continue the same test. Runtime/probe built; app packages
still contain the previous verified first-match changes. All processes terminal.


### 2026-09-13: score display conversion and round-two gameplay

Added melee_single_scene_decode, generalizing the intro scene archive bridge
by its public scene symbol. GmRegClr now uses a runtime-lifetime native cache
for ScGamRegClear_scene_data. Scene models now delegate to the existing
MeleeDynamicModel decoder, preserving shape animation and variant tables.
Score archive/120-frame animation checks pass after source disposal;
classic-score-animation-test.log. Related scene regressions pass 7/7
(classic-score-scene-regressions.log), and all five intro archive regressions
pass (score-intro-regressions.log).

Live tests exposed and corrected several retail layout assumptions:
- fn_80168A6C copied scene pointers as s32 words. Native code copies the typed
  model, lights, camera/animation, and fog fields.
- gmregclear.c had incompatible overlay structs for one score state. Native
  views now alias a shared complete type, including the ten joint pointers,
  text pointers and seven cached score values. Value accesses account for the
  eight preceding native text pointers.
- lb_80012994 addressed GXColor values before a stack GXTexObj. Native blur
  rendering now uses 21 explicit color slots; retail expressions remain.
Logs classic-score-live.log, classic-score-pointers.log and
classic-score-state.log preserve the three intermediate failures.

classic-score-blur.log reached frame 6500 with a rendered score screen and no
sanitizer error, but exited 6 because the test pressed A instead of Start.
classic-score-blur.png was inspected (score 55000, special bonuses and Press
Start). The fixture now sends Start for the score overlay, A for intros.
classic-round-start.log advanced to state 8 at4144, state9 at4285, then found
an actual grab-release crash in the second match at5701:
ftCo_CaptureWaitHi_Anim used a fixed-offset FighterOverlay and passed a null
attacker to ftCo_800DA698. Native code now uses canonical Fighter grab_timer,
victim_gobj, capturewait motion fields and typed common attributes. Build
classic-capture-wait-build.log passes (initial x3B4 field-name error corrected
to shouldered_anim_rate).

classic-capture-wait.log exited6 at1800 because a timed menu input was missed;
it did not exercise the gameplay fix. Added scene-aware menu confirmation
retries between frames1100 and1700 while still in GM_MENU. Build
classic-navigation-build.log passes; latest full regression is
classic-round-checked.log (inspect its terminal outcome). No claim of a full
Classic run or other single-player modes. Current package apps predate these
score/grab changes until rebuilt.


Latest result: classic-round-checked.log PASSED at6500 submissions,
Classic mode3/state9 with five fighters and no sanitizer error. The image
classic-round-checked.png was inspected: Icicle Mountain team round, Fox two
stocks/0%, ally Mewtwo16%, Kirby59%, Ice Climbers55%, timer4:24.79. The test
proves the controlled first-round KO -> score -> Start -> second intro ->
second match path, including sustained second-round gameplay. It does not prove
completion of round two or full Classic, nor independently instrument whether
the exact formerly crashing grab-release branch ran in this randomized retry.

App build logs for this revision: mac-classic-round-two-app-build.log,
iphoneos-classic-round-two-app-build.log,
iphonesimulator-classic-round-two-app-build.log. Next continue later rounds,
including target-test/bonus stages and bosses, and other 1P modes; preserve
original goal scope and opt-in MetalFX request after core stability.


### Classic first bonus-stage transition (2026-09-13)

The windowless native Metal `--classic-bonus` regression now passes 9,000
submissions with ASAN enabled. It follows the ordinary Classic menus as Fox,
uses fixture-controlled blast-zone KOs at frames 3,900 and 6,600 to finish the
first two fights, advances the ordinary score/intro scenes, and reaches Fox's
Target Test (mode 3, scene 17, ground 46, one live fighter). This verifies the
transition and sustained bonus-stage execution, not a naturally won fight,
completed Target Test, complete Classic run, or other single-player modes.

The first attempt aborted in the retail archive loader while loading
`/GrTFx.dat`. Its native adapter now owns four surface-hit records and the Mato
target article, including its common-data pointer and reserved special fields.
The surface-hit consumer now reads the scalar damage descriptor on native
builds rather than treating it as a pointer-expanded DynamicsDesc. Retail
behavior is preserved. The adapter admits only Fox's target stage.

`--target-fox-archive` validates every surface-hit word and the target's common
record after erasing and freeing the original DAT. Target ownership tests and
Icicle Mountain, Poke Floats, and Rainbow Cruise archive regressions passed.
Build/test evidence: target-fox-build.log, target-fox-ownership-build.log,
target-fox-ownership-test.log, target-stage-regressions.log. Full transition:
classic-bonus-native.log (exit 0). Its screenshot classic-bonus-native.png was
visually inspected: Fox, targets, timer, damage HUD, and stage are visible.
The earlier classic-bonus.log records the original loader failure.

App package build logs: mac-classic-bonus-app-build.log,
iphoneos-classic-bonus-app-build.log, iphonesimulator-classic-bonus-app-build.log.
Physical iOS and interactive Mac verification remain outstanding. Next verify
bonus completion and later Classic rounds, then other single-player modes.
Opt-in MetalFX remains pending after core stability; the original goal is active.


### Classic bonus completion and following fight (2026-09-13)

The new `--classic-after-bonus` fixture passes 12,000 submissions with ASAN
(classic-after-bonus.log, exit 0). After the existing controlled first/second
fight KOs and Fox Target Test entry, frame 9,100 invokes the normal damage
callback and removal for all ten Mato target items. The remaining count reaches
zero; the ordinary completion/score flow advances to intro scene 24 at frame
9,364, then the following match scene 25. At the final check there are two live
fighters on Yoshi's Story (ground 10). This tests target callback/removal and
bonus completion/record/scene paths, not natural attack hit detection or a full
Classic completion. The captured classic-after-bonus.png is a direct native
framebuffer image; inspect it before sharing. This turn changes only probe
fixtures/help and documentation; production app packages remain the verified
classic-bonus builds. Initial invocation failed usage validation (exit 2);
the flag admission was corrected before the successful run. Build evidence:
classic-after-bonus-build.log. Continue later fights, bonus stages, bosses,
and other single-player modes; MetalFX and full device verification remain open.

Visual verification: classic-after-bonus.png inspected; Fox and Yoshi, stage,
timer, and HUD are visible. Final help-text rebuild also exited 0.


### Native damage HUD texture lookup correction (2026-09-14)

Inspection of the prior Classic screenshot revealed an HP marker on a normal
opponent. ifStatus_802F4EDC still used a retail HSD_AnimJoint/HSD_AObjDesc alias
when restarting the marker animation; initial HUD construction already used
the proper native HSD_MatAnimJoint chain. Applied that native chain to restart
as well. All six digit texture lookups (initialization and per-frame refresh)
also incorrectly accessed HSD_MatAnim.texanim via HSD_AObjDesc.fobjdesc: those
fields share retail offset 8 but have native offsets 16 and 8. They now use a
typed native helper. Retail lookups remain unchanged.

Evidence: the intermediate marker-only Classic run (classic-hud.log, exit 0,
12,000 frames) completed the bonus and entered a Brinstar fight. Both player
stamina flags were zero, damages 39 and 49. Visually inspected classic-hud.png
showed the CPU as 99%, confirming a digit-selection problem despite no ASAN
error. That binary did not yet contain the digit helper correction.

The final `--hud-restart` regression raises both players' damage to 123 at
frame 2,200, lowers it to 42 at 2,400, and checks canonical texture-table
identity for six digits plus both marker textures at frames 2,500 and 2,800.
It passed with ASAN (hud-restart.log, exit 0). Visually inspected
hud-restart.png shows Ness correctly at 42% and Fox at 0% after respawning,
matching the runtime report. This verifies normal damage and restart/respawn
HUD paths; dedicated stamina-mode coverage remains outstanding.

Build logs: classic-hud-build.log (marker only), hud-textures-build.log (final).
App rebuild logs: mac-hud-textures-app-build.log,
iphoneos-hud-textures-app-build.log, iphonesimulator-hud-textures-app-build.log.
Full Classic completion, other single-player modes, physical device checks,
and opt-in MetalFX remain pending. Continue the original goal.


### Stamina HUD regression (2026-09-14)

`--hud-stamina` extends the native HUD restart fixture through 3,200 frames.
At 2,600 it enables player 1's stamina HUD, sets maximum HP to 100 and damage
to 25, and rebuilds that player's existing HUD through ifStatus_802F5EC0.
At 2,700 it lowers damage to 10 (healing), exercising the animation restart.
At 2,750 the displayed value equals runtime remaining HP (90), and the selected
marker image equals the HP image in the canonical material-animation table.
At 2,900 it restores ordinary damage mode and sets damage to 42. The 3,000 and
3,200 checks verify 42 and the percent marker. Existing eight texture-table
identity checks also pass at 2,500 and 2,800.

Build hud-stamina-build.log and ASAN windowless Metal run hud-stamina.log both
exit 0. This validates HP/percent HUD setup, healing/restart and restoration
through game routines; it does not prove stamina combat rules, boss loading,
or a complete boss fight. No production code changed this turn, so packages
remain the hud-textures builds. Next continue Classic beyond scene 25 toward
the remaining bonus stages and bosses, keeping the full native Mac/iOS and
opt-in MetalFX goals open.


### Classic giant-round transition (2026-09-14)

`--classic-fifth` extends the existing continuous Classic regression to 15,000
submissions. Frame 12,100 puts the post-Target-Test opponent below the blast
zone, then the ordinary score/intro transition enters scene 32 at 12,335 and
scene 33 at 12,477. The latter is Classic's giant-opponent round; it loaded
Fountain of Dreams. The final assertion reports mode 3, scene 33, four retained
fighter objects, ground 12. ASAN run classic-fifth.log exits 0; build
classic-fifth-build.log exits 0. This run includes the final native HUD fixes.

Visually inspected classic-fifth.png shows the giant round's score screen,
with score 263,600 and Press Start. There is no extra scripted KO in scene 33:
the round reached its end screen during ordinary runtime combat. Retained
fighter count at the final assertion must NOT be described as four fighters
still actively fighting. This fixture does not yet assert the exact match
outcome or advance that score screen. Next advance scene 33 with Start and
verify the following bonus stage (intro 40 / match 41).

Only fixtures and documentation changed this turn. Production packages remain
the hud-textures builds. Full Classic, other modes, actual device validation,
and opt-in MetalFX remain pending under the original goal.


### Trophy bonus stage and coin article work in progress (2026-09-14)

Added `--classic-trophy`, extending the continuous fixture to 18,000 frames /
scene 41 after the giant round. The giant round is finished by a controlled
opponent KO only if still active at 15,100; Start advances its score screen.
The fourth-round fixture now also accepts natural completion at 12,100 (the
first retry exited 6 there because the old fixture demanded an active match).

Added native `/GrNFg.dat` stage admission and owned decoder: two models, six
scalar yakumono words, empty item table, no particle bank. `--figureget-archive`
passes source erasure / ownership checks; target-fox and Poke Floats archive
regressions pass. Logs: figureget-build.log, figureget-archive-test.log,
figureget-regressions.log. An obsolete test was intentionally terminated with
SIGTERM (exit 143, classic-trophy.log) when the stage adapter was ready.
classic-trophy-native.log stopped on the fourth-round fixture assertion.

The corrected continuous run (classic-trophy-checked.log) loaded GrNFg.dat and
then aborted at the first trophy spawn: native item article 159 (Coin) was not
registered, from it_802F2094 / grFigureGet_80219898. This is a real production
failure, not a fixture failure. Added Coin's nineteen float attributes and
owned built-in character-table registration. Its six CODE states all have -1
animation IDs; the serialized article has no animation table or joint model.
Trophy model joints are supplied dynamically by tyDisplay_8031C5E4. Initial
six-animation decode failed the items-data assertion (coin-items-test.log).
Corrected serialized count to zero; `--coin-article` now passes exact parameter
comparison after source erasure, null model/state schema, and root hurtbone.
Logs: coin-dynamic-build.log, coin-probe-build.log, coin-article-test.log.

LIVE at this checkpoint: full windowless Metal retry session 10317 writes
classic-trophy-coin.log / classic-trophy-coin.png; comprehensive item-data
regression session 40320 writes coin-items-checked.log. Poll these exact handles
to terminal before drawing conclusions. No current GPU/app verification of the
coin fix yet. Do not rebuild app packages while the GPU probe runs. Production
packages remain hud-textures builds. Finish this live test and address subsequent
failures; then rebuild Mac/iphoneos/iphonesimulator packages if verified.
The original full native-playability goal and opt-in MetalFX remain active.


### Trophy spawn failure resolved; following fight reached (2026-09-14)

Both previously live processes are terminal. Comprehensive item-data regression
session 40320 / coin-items-checked.log exited 0, including Coin's source-free
parameter verification and the existing ordinary/Pokemon item tests.

The continuous native Metal run session 10317 / classic-trophy-coin.log ran
through 18,000 submissions with no ASAN error or production panic. It entered
the trophy bonus (scene 41) at frame 15,356, passed the prior first-spawn panic,
and automatically advanced to intro scene 48 at 16,276, then fight scene 49
at 16,418. The endpoint reports mode 3, scene 49, two fighters, ground 18.
Visually inspected classic-trophy-coin.png shows Fox versus Captain Falcon on
Mute City, both with normal percent HUDs. Trophy collection success is NOT
asserted; the ordinary bonus can end after missed trophies too.

The run exited 6 solely because the old endpoint required scene 41. Updated
`--classic-trophy` to require observed scene-41 entry and allow either the
bonus still running or the following scene-49 fight. That fixture-only change
builds successfully (classic-trophy-exit-build.log) but has not been rerun yet.
Do not label the prior exit-6 run as an exit-0 test. Evidence proves the live
spawn path and subsequent transition, not the amended final assertion or full
Classic completion. No GPU processes remain active.

App package build logs for these production fixes:
mac-trophy-bonus-app-build.log, iphoneos-trophy-bonus-app-build.log,
iphonesimulator-trophy-bonus-app-build.log. Next continue scene 49 and the later
Classic rounds/bonus/bosses, with actual collection/combat coverage still needed.
The full native Mac/iOS playability goal and opt-in MetalFX remain unfinished.


### Classic multi-opponent intro and shared spawn state (2026-09-14)

Added `--classic-team`: after the established trophy bonus / following fight,
frame 18,100 KOs the scene-49 opponent only if the match is still active; Start
advances the score screen. Target endpoint is scene 57 at 21,000 submissions.
Initial build classic-team-build.log succeeded. The original run session 47999
(classic-team.log) is terminal exit 134: ASAN caught a 16-byte stack overflow in
MTXOrtho, called by fn_80185408 in gm_1832.c during the multi-opponent intro.
The local Mtx was only 48 bytes; native now uses Mtx44 (64 bytes). Retail local
layout is retained. Build classic-team-projection-build.log succeeds.

Independent source audit found native opponent state problems in gm_16A2:
event callback setter/caller truncated pointers to s32; its overlay callback
at aligned 0x1C0 overlapped the expanded state's following roster array; the
fixed-prefix reset did not clear that callback; and two roster shuffle/read
paths hardcoded retail 0x1C0. Native state now has a typed callback field,
setter/caller use intptr_t, callback reads use the typed accessor, and reset /
roster access use offsetof(x1C0). The native callback invocation uses that
accessor; retail invocation remains unchanged.

Focused `melee_game_startup --spawn-state` test passes with ASAN:
full-width setter round-trip, actual callback invocation, route callback
preservation, no roster overwrite, callback reset, and roster preservation
across gm_8016A22C. opponent-state-test.log exits 0. Initially placed this test
in hsd_scene_probe, whose limited linkage lacked gm_16A2 symbols; moved it into
the full startup probe and restored hsd_scene_probe linkage. Final build:
opponent-state-probe-build.log, exit 0. The original Classic run did NOT include
these independently developed spawn-state changes.

LIVE now: retry session 7856 writes classic-team-checked.log and captures
classic-team-checked.png using BOTH production fixes. Poll this exact handle
until terminal. Do not claim the intro fixed at runtime until it passes; do
not rebuild app packages while this GPU test runs. Packages still trophy-bonus
builds (Mac, unsigned iphoneos, Simulator all built successfully last turn).
Continue later Classic rounds and other modes; full playability, physical-device
verification, and opt-in MetalFX remain pending under the original goal.


### Native intro draw FIFO correction (2026-09-14)

Retry session 7856 / classic-team-checked.log is terminal exit 134. It passed
the previous MTXOrtho stack overwrite, then faulted on a WRITE to 0xCC008000
at gm_1832.c:682 in fn_80185408. This function still directly wrote its twelve
vertex floats to the GameCube FIFO. Native now calls GXPosition3f32 for the
four identical vertices and GXEnd; retail FIFO writes remain unchanged.
classic-team-fifo-build.log succeeds. The shared lbvector projection path was
also audited and already has the native Mtx44 correction.

Added short `--intro-mask` GPU regression: submit_frame calls the exact
fn_80185408 sixty times (frames 60..119) while a native GPU frame is active.
The run goes through 600 submissions due to default confirm navigation;
intro-mask.log records all sixty calls and exits 0, with no sanitizer error.
This validates the formerly crashing projection and FIFO path directly, not
the complete multi-opponent intro/round. Build intro-mask-probe-build.log passed.
The prior full retry included the opponent-state corrections and passed all
earlier Classic transitions, but stopped in this drawing function.

App package logs: mac-intro-mask-app-build.log, iphoneos-intro-mask-app-build.log,
iphonesimulator-intro-mask-app-build.log. Full scene-57 regression still needs
a retry with the FIFO correction. Full goal, later single-player modes,
physical-device validation and opt-in MetalFX remain unfinished.

All three intro-mask app package builds exited 0. Full retry started with
classic-team-fifo.log / classic-team-fifo.png; inspect its live session from
this turn before restarting or building packages.
LIVE retry handle: session 6721. Poll to terminal; no other tests are active.


### Classic Kirby item pickup: external animation references (2026-09-14)

The continuous Classic retry in `classic-team-fifo.log` reached frame 6426
and crashed in `ftAnim_80070904` during Kirby item pickup. The animation
pointer was `-1`. PlKb.dat declares the first left/right hand poses as external
symbols; that word terminates their external relocation chains. It is not a
runtime sentinel. `lbArchive_InitializeDAT` resolves all such references to
null, and no later Melee code binds those symbols. The native part-animation
decoder now uses the same external-clearing behavior before decoding owned
trees. It preserves null entries and continues to validate internal pointers.

`kirby-extern-build.log` passed. The ASan `--kirby-data` test in
`kirby-extern-test.log` passed after disposing of both source archives, checking
both middle-hand entries are null and the remaining four hand poses retain
animation tracks. The Fox decoder regression also passed in
`kirby-extern-fox-regression.log`. A continuous Classic retry is recorded in
`classic-team-kirby.log`; its result is pending at this checkpoint. This change
is not yet in the packaged macOS/iOS apps.

The retry ended with fixture exit 6 at frame 6600, after passing the old Kirby
crash frame and reaching scene 9 with four fighters. There was no ASan error;
the test rejected an already finished match. The team-round fixture now accepts
natural completion, as later-round fixtures already do. The macOS, iPhoneOS
(unsigned), and Simulator packages were rebuilt successfully with the external
reference fix (`*-kirby-extern-app-build.log`). Full Classic completion remains
unverified.


### Race to the Finish native archive groundwork (2026-09-14)

GrNPo.dat now has an owned stage adapter and admission in grDatFiles. Its
parameter block contains six pointers to nine-word hit records, 30 entries
mixing 32-bit words and signed 16-bit fields, and 33 key/value entries. The
adapter decodes these explicitly, preserves models/collision/lights/animations,
and owns the hit records independently of the serialized archive. The hazard
callback's result now writes a full native pointer, preserving the retail
32-bit assignment in the original build. The shared parameter declarations
moved to grpushon.h so the native decoder and game use one layout.

`pushon-adapter-build.log`, `pushon-probe-build.log`, and the ASan source-disposal
test `pushon-archive-test.log` passed. `--pushon-archive` checks every hit-record
word, mixed-width entry and lookup after destroying the source bytes. This
establishes archive ownership and schema correctness, not playable completion.
The new `--classic-race` fixture continues from the ten-opponent round to scene
65 by putting enemy waves below the blast zone, testing normal replenishment
and transitions. It is built (`classic-race-probe-build.log`) but not run yet.
The current continuous team regression is `classic-team-natural.log`.
The packaged apps still contain the preceding Kirby fix; Race to the Finish
runtime testing and packaging are pending.

Race-stage light audit also found two point-light descriptors referencing
single-float globals, which would read past those globals on native builds.
Both begin with ref_br=16; GXInitLightDistAttn consequently disables distance
attenuation regardless of the adjacent bytes. Native descriptors now supply
a complete `{16, 0, GX_DA_OFF}` point record, preserving that initial behavior
until the stage assigns its configured lights. Retail initializers are retained.
`pushon-lights-build.log` passed; in-game race lighting remains to be checked.


The continuous `--classic-team` run in `classic-team-natural.log` finished
successfully (exit 0) at frame 21000: Classic scene 57, four fighters, ground 3.
No ASan error was reported. `classic-team-natural.png` was visually inspected:
Fox versus the Mario team on Rainbow Cruise, visible stage/fighters/HUD and
remaining opponent icons. This checks the previously crashing multi-opponent
intro rendering in a continuous run through the preceding Classic rounds.
The fixture uses controlled knockouts/target callbacks; it does not establish
natural full-game victory. Race to the Finish and final rounds are still pending.

All three app packages rebuilt successfully (`mac-pushon-app-build.log`,
`iphoneos-pushon-app-build.log`, `iphonesimulator-pushon-app-build.log`). They
include the Race to the Finish archive and initial-light changes, whose runtime
verification remains pending. The physical-device package remains unsigned.
Next continuous probe: `--classic-race` with the current game_startup binary.


### Continued Classic / boss attribute groundwork (2026-09-14)

The first `--classic-race` launch revealed that the new flag was missing from
the probe argument allowlist (exit 2 before startup). The allowlist is fixed;
`classic-race-admission-build.log` passed and the actual continuous run is now
in `classic-race.log` (session 55328, still running at this checkpoint).

Master Hand and Crazy Hand now have checked special-attribute decoders in
character_attributes.c: 0x17c and 0x144 bytes respectively, explicit integer
field offsets, finite float validation and atomic output on failure. The
`--hand-attributes PlMh.dat PlCh.dat` ASan test compares every decoded word,
rejects nonfinite position data without partial output, and verifies data after
source disposal. Build and test passed (`hand-attributes-build.log`,
`hand-attributes-test.log`). These decoders are groundwork only: boss ftData
owners, items, motion loading and gameplay still require integration. The
retail boss ftData has null x58/x5c fields, so existing regular-fighter
owners cannot be reused without handling those absent fields explicitly. Its
x8 word is zero but is relocated: it correctly references data offset zero.


`classic-race.log` terminated with ASan exit 134 at frame 21562 while entering
GrNPo.dat, after the ten-opponent round completed and scene 64 was reached.
The crash is in HSD_LObjGetColor called by grPushOn_80218888 during race-stage
initialization. The running binary predates a lighting-list fix found during
its run: grPushOn_802187A8 cast the GX-link-head pointer array to HSD_GObj and
read next_gx. On GameCube that selects slot 4; on ARM64 it selects slot 3.
Native code now directly selects HSD_GObjGXLinkHead[4], matching other stages'
lighting setup. `pushon-light-list-build.log` passed. The corrected binary has
not yet repeated the race runtime check. No process remains from that run.

All three packages rebuilt with the race light-list correction; the logs
`mac-race-light-list-app-build.log`, `iphoneos-race-light-list-app-build.log`,
and `iphonesimulator-race-light-list-app-build.log` exited 0. Device signing
and runtime verification of Race to the Finish remain pending. Next action:
rerun `--classic-race` with a new log/capture path, retaining the failed log.


### Boss fighter decoding and game-over path (2026-09-14)

The `classic-race-light-list.log` retry ended at frame 10429 with archive parse
panic (exit 134) while entering game over after losing the fourth round. It
never reached the race stage, so it does not verify the race light-list fix.
All prior run handles are terminal. A focused `--classic-gameover` probe now
loses the first Classic fight through normal stock/blast-zone processing and
checks scene 104 at frame 6500. Current log `classic-gameover.log`, session 43345,
is live at this checkpoint.

The four game-over archives GmGover, GmGoCoin, GmGoAnim and GmRgStnd now use
owned native scene adapters in both archive-loading entry points. Destroying a
cached archive clears its cache entry for later retries. The ASan
`--gameover-archives` test loaded all four and reloaded their models after source
disposal (`gameover-archives-test.log`, build also passed). Actual screen
rendering, Continue/No behavior and repeat-loss cleanup remain unverified.

Master/Crazy Hand fighter owners now assemble common/special attributes,
collision/dynamics, visibility, sounds, wait data, and 50/49 AJ animation trees.
The generic auxiliary/model decoders preserve absent x58/x5c fields as null.
The owners also decode three projectile articles per archive: laser, bullet,
and bomb (1/2/2 animation states), with all native scalar special schemas.
ftData now routes both boss kinds through these owners. The source-free model
and data tests passed (`hand-items-model-test.log`, `hand-data-test.log`);
the Fox regression passed after the shared nullable-field changes, and
`hand-runtime-build.log` passed. Full boss gameplay and attack behavior still
require runtime validation. Packages have not yet been rebuilt with these
boss/game-over changes.


The first focused game-over run (`classic-gameover.log`) terminated at frame
4015 in Toy_803102D0 while parsing TyDataf.dat with the retail loader. Native
TyDataf decoding now owns its 293 main and five US file/model records, swapping
only the 32-bit IDs and preserving inline strings. A filename membership check
routes actual trophy files listed by the archive to the native model adapter;
it rejects unrelated TyDataf itself. Cache pointers clear on archive destruction.
Tests `trophy-files-test.log` and `trophy-file-admission-test.log` passed.

Game-over coin icon pointers now use the canonical x28.typed.jobjs array instead
of 32-bit overlay offsets for creation, initial visibility and animation updates.
`gameover-coin-pointers-build.log` passed. `classic-gameover-native.log` (session
75575) is a retry that includes the table decoder and coin fix, but predates the
filename-membership routing fix. Full screen validation is still pending.


The retry `classic-gameover-native.log` exited 6 at frame 3900 because its setup
fight had already ended naturally. The fixture now assigns ten setup stocks
and explicitly loses the last human stock. `classic-gameover-checked.log` then
reached game over but exited 134 when the trophy adapter rejected Fox's
`ToyFoxModel_TopN_joint` name. Native trophy decoding previously recognized
only `_Top_joint`; it now also recognizes `_TopN_joint` for trophy archives.
The Fox trophy model reload/animation-sampling test passed after source disposal
(`gameover-fox-trophy-test.log`, `trophy-topn-build.log`).

The focused loss fixture is now shorter: final-stock loss at frame 2800,
scene-104 check at 4500; the unrelated frame-3800 live-match assertion is omitted
only for this mode. `gameover-focused-build.log` passed. The current retry is
`classic-gameover-topn.log` (session 47571), running at this checkpoint. All
other probe handles from the preceding retries are terminal.


`classic-gameover-topn.log` completed 4500 rendered frames with no ASan error
and reached mode 3, scene 105. Its exit 6 was solely an incorrect fixture
expectation: gmclassic's scene table identifies 104 as Coming Soon and 105 as
GS_GAMEOVER. The fixture now checks 105. The framebuffer
`classic-gameover-topn.png` was visually inspected: CONTINUE?, YES/NO, Fox's
fallen trophy, the coin counter and score are visible. This verifies entry and
rendering after losing a Classic fight; selecting Continue/No and a second loss
still need runtime tests. Race to the Finish lighting and final-boss gameplay
remain unverified. All GPU processes from this checkpoint are terminal.

macOS, iPhoneOS (unsigned), and Simulator app packages rebuilt successfully with
the game-over fixes and boss fighter/projectile decoders. Logs:
`mac-gameover-hand-app-build.log`, `iphoneos-gameover-hand-app-build.log`,
`iphonesimulator-gameover-hand-app-build.log`, all exit 0. Next priorities:
verify Continue/No and repeat loss, rerun corrected Race to the Finish entry,
and reach the final bosses through Classic. MetalFX is still deferred until
core gameplay works, as requested.


### Continue and repeated-loss verification (2026-09-14)

`--classic-continue` completed 9000 frames, exit 0, without an ASan error
(`classic-continue.log`). It entered scene 105, selected Yes, verified a resumed
Classic fight in scene 1, lost a second time, verified another scene 105, then
selected No and returned to GM_MENU scene 0. `classic-continue.png` was viewed:
the single-player Regular Match menu is visible with Classic highlighted. This
checks both game-over choices and archive reload after a repeated loss.

Focused round probes `--classic-race-entry` and `--classic-boss-entry` now use
the existing Classic configuration's starting-round byte (8 or 10), which CSS
exit consumes to construct the normal round rules and assets. They check scene
65 or 81 at frame 5000; the boss variant also requires a live boss fighter
object. Build passed (`classic-focused-rounds-build.log`). These focused tests
supplement rather than replace the continuous Classic regression. The race
entry test is now live in `classic-race-entry.log`, session 51379.


The focused race run `classic-race-entry.log` loaded GrNPo.dat and reported
Classic scene 65 with one fighter at frame 3800, without a sanitizer error.
It reached the following metal-opponent intro (scene 72) by frame 5000, so its
old stay-in-scene-65 assertion exited 6. The final capture was viewed and shows
Fox versus metal Captain Falcon. The fixture now records entering the requested
round and accepts a naturally advanced race result in scene 72/73; it also
captures an intermediate `.round.png` at frame 3500. This verifies the prior
race lighting crash no longer occurs, but does not verify complete course
traversal or all hazards. `classic-focused-checkpoints-build.log` passed.

The focused final-boss probe is currently running as `classic-boss-entry.log`,
session 87352. All other GPU probes are terminal.


### Master Hand laser pointer lifetime (2026-09-14)

The first focused boss probe (`classic-boss-entry.log`, exit 134) loaded Final
Destination and reached Classic scene 81 with two fighters. It crashed in
it_802F046C from ftMh_FingerBeamLoop_Anim when finishing a finger-laser attack.
The native motion union overlaid 64-bit projectile pointers with 32-bit sound
IDs: assigning Master Hand's x30 sound ID corrupted its first laser pointer.
Crazy Hand uses an analogous overlapping pointer/sound layout.

Both hands now keep their four native laser pointers in the persistent fighter
variables, accessed through FT_MH_LASER/FT_CH_LASER; the retail macros retain
the original motion fields. Initialization, spawning, attack-end cleanup and
Crazy Hand's grab transition use these accessors. `hand-laser-storage-build.log`
and `hand-laser-state-probe-build.log` passed. The ASan `--hand-laser-state`
regression in `hand-laser-state-test.log` preserves all four pointers across
sound-ID assignment and clearing for both hands. The gameplay retry is live
in `classic-boss-lasers.log`, session 49475, at this checkpoint. App packages
have not yet been rebuilt with the laser-pointer correction.


`classic-boss-lasers.log` completed 5000 frames, exit 0, with Classic scene 81,
two fighters and one boss on Final Destination (ground 37). No ASan error was
reported. The final framebuffer was viewed: Fox and Master Hand are visible,
with a 150 HP boss display. This establishes initial boss combat and rendering,
not victory or complete attack coverage.

`--classic-boss-lasers` now explicitly starts Master Hand's finger-beam attack
at frame 2800 and requires observing all four laser pointers followed by their
clearing in FingerBeamEnd. It supplements the pointer-layout test and the
ordinary boss-entry test. Build passed (`boss-laser-attack-probe-build.log`).
The exact-attack run is live in `classic-boss-laser-attack.log`, session 57894,
at this checkpoint. All other GPU handles are terminal.


The exact finger-laser run `classic-boss-laser-attack.log` passed (exit 0,
5000 frames): four laser objects were observed and all pointers cleared in
FingerBeamEnd, while Classic remained in scene 81 with the boss alive. No
sanitizer error was reported. This directly verifies the attack that crashed
in the earlier boss-entry run. Boss victory, later scenes/credits, Crazy Hand
attack gameplay, full race traversal and the full continuous Classic route
remain to be tested. All GPU handles are now terminal.

All three Apple packages rebuilt successfully with the boss laser correction:
`mac-boss-lasers-app-build.log`, `iphoneos-boss-lasers-app-build.log`, and
`iphonesimulator-boss-lasers-app-build.log` exited 0. The device build remains
unsigned. The intermediate boss capture `classic-boss-laser-attack.png.round.png`
was viewed and shows Fox and Master Hand on Final Destination with normal
percent/HP displays. No processes remain live at this checkpoint.


### Classic boss defeat and ending archive conversion

Master Hand's defeat handler also accessed the old `dmg0` pointer overlay.
It now uses the same persistent native laser slots as spawning and ordinary
attack cleanup. Its sound IDs use the callback-expanded canonical motion
layout on ARM64, with retail expressions retained for GameCube.

`--classic-boss-defeat` enters the normal final round through the Classic CSS
starting-round setting, starts Finger Beam, waits until all four lasers exist,
and applies damage through `Fighter_TakeDamage_8006CC7C`. The first run confirmed
0 HP and motion 344, then reached the trophy-ending loader and exposed an
unconverted archive. `classic-boss-defeat.log` ended with the expected archive
parse failure; this is not a full Classic pass.

`melee_ending_decode` now owns all four GmRegEnd scenes and embedded demo motion
bytes. Scene conversion supports owned camera animations, light animation via
the existing environment owner, and fog tracks. The ending exports one camera
record per scene without a trailing null record; the ending decoder preserves
that layout explicitly. The native archive loader admits GmRegEnd plus the
three trophy display model archives TyMcCmDs/TyMcR1Ds/TyMcR2Ds.

The USD and DAT ending archive tests passed after source disposal, including
four models, four cameras, four camera animation sets, and Fox's embedded
motion file. The extended USD test also sampled light and fog animation.
TyMcCmDs passed six model bundles with animation sampling. Existing game-over
archive and NtMemAc scene descriptor lifetime regressions passed. Evidence:
`ending-archive-test.log`, `ending-japanese-test.log`,
`ending-environment-test.log`, `ending-trophy-display-test.log`,
`ending-gameover-regression.log`, `ending-scene-regression.log`.

The subsequent ending run passed both archive loads and identified the next
failure: the demo fighter had no installed motion table for archive index 2.
The native demo loader now selects the original ending record 12 (intro still
uses records 10–11). This uses the same owned motion decoder as the working
intro path. `ending-fighter-motion-build.log` passed. The shared scene changes
also passed IrAls (27 embedded motion archives) and GmRegClr regression tests.
An initial intro test mistakenly supplied GmIntEz, a scalar matchup table;
rerunning with the actual IrAls scene archive passed.

The next ending run reached frame 4347 before the demo Sleep transition hit an
assertion on an intentionally empty motion slot. Native primary/secondary
motion loading now matches retail null-animation behavior when x14 and x8 are
both zero, while nonempty records still require an owner. The focused
`--empty-demo-motion` test verified stale primary/secondary state is cleared
(`ending-empty-motion-test.log`, exit 0).

`classic-boss-ending-cleanup.log` then completed the trophy-ending sequence
and progressed to Staff Roll at frame 4756. It terminated with exit 134 when
`gm_Scene_StaffRoll_OnEnter` loaded the still-unconverted GmStRoll.dat archive.
This is a remaining known Classic completion crash, not a completed clear.
The requested symbols are ScGamRegStaffroll_scene_data and
ScGamRegStaffrollNames_scene_modelset. SdStRoll.dat SIS loading ran first.
The intermediate `.ending.png` was viewed: Fox is rendered during the close-up
fall, against black, but it is not a useful wide gameplay screenshot.

All three Apple app packages rebuilt successfully with the fixes above:
`mac-classic-ending-app-build.log`, `iphoneos-classic-ending-app-build.log`,
`iphonesimulator-classic-ending-app-build.log`. The iPhone device app remains
unsigned. All GPU and package process handles are terminal. Credits conversion,
full Classic completion, other single-player modes and opt-in MetalFX remain
unfinished.


### Staff Roll native archive and pointer-sized storage

`melee_staffroll_decode` owns ScGamRegStaffroll_scene_data and the ten
ScGamRegStaffrollNames_scene_modelset entries. Its camera record has no trailing
null entry. GmStRoll.dat now routes through that decoder in lbArchive_80016DBC.
`--staffroll-archive GmStRoll.dat` passed after source disposal: twelve models,
one camera, animated lights/fog, and all ten name groups were instantiated,
animated and released (`staffroll-archive-test.log`).

The first GPU retry loaded the archive and reached fn_801AB200, exposing the
old retail allocation sizes for 198 staff text records and 198 sorting records.
Native allocations/initialization now use 198*sizeof(actual record). Material
highlight writes now navigate typed DObj/MObj/Material fields and update diffuse
color, rather than interpreting every object as the same four-word node.
`staffroll-buffers-build.log` passed. The initial retry evidence is
`classic-credits-entry.log` (ASan failure at staffInfo[197], before the buffer fix).

The new `--classic-credits-complete` probe retains normal Classic final-round
entry and boss defeat, then runs through 10,500 submissions, with a 540-second
watchdog. It requires observed credits scene 1 followed by ending movie scene 2
or congratulations scene 3 in GM_CLASSIC_GOVER. No credits fast-forward or scene
skip is used. It captures a credits checkpoint at frame 5500.


The normal-speed full credits test passed: `classic-credits-complete.log`
exited 0 after 10,500 submissions and reported credits entered, mode 21,
scene 3. It advanced through the credits and ending movie into Congratulations.
Both PNGs were inspected: `.credits.png` shows the live staff roll and
`classic-credits-complete.png` shows Fox's rendered Congratulations image.
This proves the final-round-to-ending path, not a continuous start-to-finish
Classic campaign or every fighter/mode.

`--classic-ending-exit` additionally uses the original Start fast-forward
control at frame 5000, dismisses Congratulations with A at frame 7000, and
requires GM_MENU at frame 8000. It uses a 420-second watchdog. Its result is
recorded below when the run finishes.

The first ending-exit fixture finished normally with exit 6 at frame 8000,
still in credits. Its Start key was held from the final match-result transition,
so the frame-5000 press generated no trigger. The fixture now releases Start
at frame 4900 before pressing it in credits. This was a fixture input issue.
Explicit assertions in game_startup_input.c are now enabled despite Release
NDEBUG; empty-demo-motion, hand-laser-state and spawn-state were rerun with
assertions active and all passed (their `*-assertions-test.log` files).


The corrected ending-exit run (`classic-ending-exit-released.log`) passed the
credits fast-forward and reached Congratulations at frame 7000 with assertions
active. Pressing A dismissed it. At frame 7061 the game selected the legitimate
new-challenger flow and crashed loading NtAppro in gm_Scene_Approach_OnEnter.
The requested archive symbol is ScNtcApproach_scene_data; the loader is
lbArchive_80016DBC. This remains a known post-clear unlock-screen crash.
The exit fixture's expected menu should account for the actual unlock flow
once that screen and ensuing challenge are supported; do not bypass unlocks.
No GPU processes remain live.

All three packages rebuilt with the credits fixes: mac-staffroll-app-build.log,
iphoneos-staffroll-app-build.log and iphonesimulator-staffroll-app-build.log
exited 0. The iPhone device build remains unsigned. All process handles are
terminal at this checkpoint. Full Classic from the opening round, post-clear
unlocking, other single-player coverage, physical device/controller tests and
opt-in MetalFX remain unfinished.


### Challenger notice and post-Classic unlock match

NtAppro now uses an owned native scene archive via melee_approach_decode.
Both DAT and USD versions passed model and animation sampling after source
buffer disposal (`approach-dat-animation-test.log`,
`approach-usd-animation-test.log`), including the notice camera and lights.

The ending-exit fixture now acknowledges the legitimate branch: after
Congratulations, it either reaches the menu or confirms the challenger notice
and requires a live two-fighter challenger match. It does not suppress unlocks.
`classic-unlock-match.log` passed 8000 submissions with mode 20 scene 1.
The PNG was inspected: Fox vs Jigglypuff on Pokémon Stadium, with live percent
HUDs. This verifies the ending-to-unlock-match path, not full campaign coverage.

`--classic-unlock-win` extends that path to 11500 submissions. It keeps the
human fighter alive with extra stocks, then moves the opponent below the stage
at frame 8100 to exercise original KO/win/unlock callbacks. It advances result
and prize screens using virtual PAD input and requires both the native unlocked
character flag and GM_MENU at the end. The targeted test changes setup/stocks;
it is not evidence of a fully played unmodified Classic run.


The unlock-win test passed: `classic-unlock-win.log` exited 0 at frame 11500,
reporting kind 15 (Jigglypuff), unlocked=1, mode 1 scene 0. It traversed the
challenger fight, reward screens and returned to the menu. The final PNG was
viewed and shows the Regular Match submenu with Classic selected. The unlock
flag was checked in game state; persistence across a separate app restart was
not tested. No GPU processes remain active.

Mac, iPhoneOS and iPhone Simulator packages rebuilt successfully with the
challenger fix (`mac-challenger-app-build.log`,
`iphoneos-challenger-app-build.log`, `iphonesimulator-challenger-app-build.log`).
The device app is unsigned. All build and GPU process handles are terminal.
Continuous Classic from its opening round, other single-player modes,
restart persistence, physical-device/controller checks and opt-in MetalFX
remain to be verified or implemented; the full native-port goal remains open.


### Continuous Classic campaign transition probe

`--classic-campaign` uses normal menu/CSS entry from the opening round, without
changing the starting-round setting or forcing scene IDs. A scene-driven fixture
records all eleven Classic battle/bonus scenes, advances intros/results with
virtual PAD input, applies controlled blast-zone KOs to opponents, breaks target
items via their normal callbacks, and damages the final boss via the damage API.
Human stocks are increased in combat rounds to keep the transition test alive.
Trophy and race bonuses run to their normal end; full-course traversal is not
verified. Credits run at normal speed. The fixture then follows any legitimate
unlock match/prizes and requires return to GM_MENU plus the unlocked flag.

The probe has a 24,000-submission ceiling, a 1200-second watchdog, and exits
successfully as soon as its full set of conditions is met. Intermediate coverage
in `classic-campaign.log` reached all eleven rounds, trophy ending, credits,
Congratulations, and the Jigglypuff unlock match in the same process. The final
result follows below. This test covers transitions and lifetime across an entire
campaign, not manual combat, full bonus-course traversal, or other fighters and
difficulty settings.


`classic-campaign.log` passed at submission 23229 (exit 0): rounds=0x7ff,
10 targets broken, credits and Congratulations observed, Jigglypuff unlocked,
GM_MENU scene 0 reached. This is the first continuous opening-round-to-menu
Classic transition pass. The immediate return-to-menu PNG was inspected but
caught a black transition frame; it is not evidence of settled menu rendering.
The earlier focused unlock-win test has a settled menu capture. Production
runtime sources were unchanged by this campaign test; the existing challenger
app builds remain current.

An `--adventure-entry` probe now selects Adventure through the menu, requires
Adventure CSS (4/112), selects Fox through virtual PAD input, and attempts the
opening stage. Its final condition at frame 5000 requires Adventure scene 1
with a live fighter. It does not force the scene or starting stage.


Adventure reproduction: `adventure-entry.log` reached Adventure CSS 4/112,
then terminated (exit 134) loading /GrNKr.dat in grDatFiles_801C6038, called by
gm_Scene_IntroNormal_OnEnter. The fallback lbArchive_800171CC attempted to parse
retail big-endian pointer data directly. The opening Mushroom Kingdom route
archive requires a native stage adapter; gameplay was not reached. GrNKr.dat
has been extracted into native/build for the next conversion step. Its stage
implementation is src/melee/gr/grkinokoroute.c/.h (Gr_Kind_KinokoRoute).

All GPU/build handles are terminal. No production runtime changes were made
in this checkpoint, so no additional app packaging was necessary. The existing
challenger app builds remain current. Continuous Classic transition coverage
now passes, but Adventure conversion, other single-player coverage, manual
bonus-course/combat verification, physical devices/controllers, restart save
persistence and opt-in MetalFX remain unfinished.


Adventure opening conversion checkpoint:
- GrNKr now uses an owned native stage adapter, including its 80 mixed-width
  spawn records and four Nokonoko/Patapata/green-shell/red-shell articles.
  Nested enemy attribute pointers and shell scalar attributes are converted.
- `kinoko-route-archive-test.log` passes after poisoning/freeing the serialized
  source, checking all spawn fields, nested attributes and item model loading.
- The first retry passed GrNKr loading and exposed a second retail archive
  fallback in `gm_Scene_IntroNormal_OnEnter` for IrNml.
- IrNml now owns ScItrNormal_scene_data plus all twelve mc01–mc12 camera records
  and their animations. `adventure-intro-archive-test.log` passes source disposal,
  loading and animation sampling for all twelve cameras.
- The next GPU retry rendered the Mushroom Kingdom intro through frame 5000
  without a runtime crash, but the probe exited 6 because the intro was waiting
  for Start. The frame-3500 PNG was inspected and shows the stage and title.
  The fixture now presses Start at 3600/release 3606; gameplay verification is
  in progress. Packaged apps do not yet contain these Adventure changes.


Adventure opening verified after follow-up fixes:
- Start at 3600 exposed an ASan global overread in fn_801695BC: its adjacent
  costume shuffle starts at index one even when index zero is the empty-list
  sentinel. Native builds now return immediately for that empty list.
  `adventure-empty-roster-test.log` passes the actual zero-opponent Adventure
  setup together with callback and roster-preservation assertions.
- The next run reached the course spawner and stopped on unconverted common
  item 43 (Kuriboh/Goomba). ItCo now owns its five animation records, model,
  three scalar special attributes and nested integer/float common attributes.
  `adventure-goomba-archive-test.log` passes the complete ItCo.usd owner suite,
  including source disposal, Goomba fields/model and all previous item checks.
- Final `adventure-entry.log` exits 0 after 5000 submissions: mode 4, scene 1,
  one live fighter, ground 31. `adventure-entry.png` was inspected and shows Fox
  on the opening pipe, HUD/timer and Mushroom Kingdom course. This is actual
  native windowless Metal output. It proves entry and roughly 22 seconds of
  course runtime; it does not prove course traversal or later Adventure stages.
- All GPU and CPU test handles are terminal. Mac/device/Simulator packages are
  now rebuilding in mac-adventure-app-build.log, iphoneos-adventure-app-build.log
  and iphonesimulator-adventure-app-build.log. Do not launch another GPU probe
  until those builds finish. MetalFX remains deferred until core gameplay works.

All three Adventure app builds completed successfully (exit 0):
`native/build/macosx-game/MeleeNative.app`,
`native/build/iphoneos-game/MeleeNative.app` (unsigned), and
`native/build/iphonesimulator-game/MeleeNative.app`.
No build/test handles remain live. These packages include the current Adventure
opening fixes. Full Adventure traversal, other single-player modes, physical
hardware/controllers, save persistence and MetalFX remain open goal work.


Adventure traversal / breakable-block follow-up:
- Added --adventure-traverse: enters via menus, presses Start, then uses virtual
  gamepad run/jump/attack input through frame 9000. Logs position and damage.
  It requires a later Adventure scene for success; remaining in scene 1 exits 6.
- First traversal reached x124 and crashed in grKinokoRoute_802084B4 while
  processing block damage: its retail padded view read a JObj at byte 0xDD8.
- Native callback now reads Item.xDD4_itemVar.yaku.x4 and updates the typed
  Ground.u.kinokoroute2.flags_0 instead of writing at fixed Ground byte 0xC4.
- Replay reached x380 and ran through 9000 submissions with enemy damage and a
  death/respawn, without a runtime crash. Exit 6 reflects incomplete traversal,
  not a crash. The final image was inspected: Fox and a winged Koopa at the river.
- Added --adventure-blocks to exercise all 51 registered type-8 block hazards via
  their normal damage callback, asserting hidden joints and stage flags, then
  continuing until frame 5000. This focused verification is running now.

Focused block verification completed successfully: `adventure-blocks.log`
exits 0, asserting all 51 hidden models and native stage-flag updates, then
continues to frame 5000 with mode 4 / scene 1 / one fighter. The runtime crash
fix is verified independently of the incomplete traversal script.

Mac, unsigned iPhone device, and iOS Simulator app packages all rebuilt
successfully (exit 0) with this block fix; logs are respectively
mac-adventure-blocks-app-build.log, iphoneos-adventure-blocks-app-build.log,
and iphonesimulator-adventure-blocks-app-build.log. No test/build handles
remain live. Next work includes reliable course traversal/Yoshi encounter,
later Adventure scenes and remaining original goal requirements. MetalFX is
still pending after gameplay stability; the goal remains incomplete.


Adventure Yoshi encounter verification:
- --adventure-yoshis (focus 12) enters the opening course normally, then places
  Fox/camera at stage marker 0xBD and uses controlled Yoshi KOs. Placement sets
  Fall/previous position to avoid old-floor collision and moves camera so it
  does not create an artificial offscreen death. No encounter/scene IDs forced.
- Final adventure-yoshis.log exits 0 at 7500 submissions: Yoshi seen=1,
  cleared=1, mode 4 / scene 1. Encounter phase 2 at 4103, phase 3 at 5042.
  The 4250 checkpoint PNG was inspected and shows the Yoshi encounter with
  ten opponent icons and three CPU Yoshis. This checks spawning/cleanup, not
  manual combat or full course traversal. No runtime fix was needed here.
- --adventure-course-exit (focus 13, 8500 limit) extends the fixture by placing
  Fox/camera at finish marker 0x99 after encounter completion, then using Start
  on match-over and A on the intervening cutscene. It requires Adventure scene
  3 with at least three fighters, checking the Mario/Peach battle transition.
  That GPU regression is currently running; no app packaging is in progress.

Adventure course-exit regression completed: adventure-course-exit.log exits 0
at 8500 submissions. Yoshi encounter reached phase 3 at 5041; finish marker
(1091.2,148.2) was applied at 5500; by 6300 the original transition reached
Adventure scene 3 with three fighters. The battle remained active through 8500.
The final PNG was inspected. This verifies controlled course finish/next-fight
loading and sustained runtime, not full manual course traversal. All GPU/build
handles are terminal. Only fixtures/docs changed in this checkpoint, so existing
Mac/iOS app packages still contain all current production runtime changes.


Adventure jungle transition regression / archive cache lifetime:
- Added --adventure-jungle (focus 14, 12500 submissions) extending controlled
  Yoshi/course finish with Mario/Peach KOs, next intro Start, and small DK wave
  KOs. It requires the Giant DK battle (Adventure scene 10) with live fighters.
- Initial run crashed after Mario/Peach at frame 6843, loading GrKg and the
  second IrNml introduction. ASan reported an invalid archive read from the
  cached HSD_Archive in lbArchive_80016DBC. The first introduction's OnLeave
  destroyed that owner without invalidating the function-local cache.
- Promoted the six intro archive slots to native_intro_archives and invalidate
  matching slots in lbArchive_80016EFC before destroying the owner. The score
  archive used the same local-cache pattern and now also invalidates on release.
- The full jungle regression is rerunning to verify unload/reload in context.
- Extracted GrNSr.dat for the next stage audit. Underground Maze remains
  unconverted. Its yakumono first five words are relocated pointers despite
  the game struct declaring them ints; stage-specific conversion must account
  for that. No maze implementation changes made yet.

Adventure jungle replay completed successfully at 12500 submissions:
`adventure-jungle.log` reports mario=1 team=1 giant=1, mode 4, scene 10,
two fighters. Second introduction scene 8 began at 6845, small DK battle scene 9
at 7084, Giant DK scene 10 at 7924. This directly exercises the formerly stale
IrNml cache across release/reload and sustained giant-battle runtime. Final PNG
was inspected and shows Giant DK at Kongo Jungle with Fox respawning.
A prior replay did not reach the cache check because its short finish-marker
placement failed. The fixture now explicitly sets airborne physics and keeps
Fox/camera at the finish marker until detection, bounded to 300 frames. Its
logs confirmed detector 153 and flags 0x50, then normal scene progression.
No scene IDs or victory states are forced; opponent KOs/placements remain
controlled and do not establish manual full-course traversal.
All GPU handles are terminal. App rebuilds with this runtime fix are in progress.

Mac, unsigned iPhone device and iOS Simulator app builds all completed (exit 0)
with the archive cache fix. Logs: mac-adventure-jungle-app-build.log,
iphoneos-adventure-jungle-app-build.log, iphonesimulator-adventure-jungle-app-build.log.
No build/test handles remain live. Next Adventure compatibility work is the
Underground Maze (GrNSr); full manual course traversal, later modes, hardware
validation and MetalFX remain unfinished. The overall goal remains active.


Underground Maze preparation and reproduction:
- --adventure-maze (focus 15, 14500 limit) extends the jungle fixture with a
  controlled Giant DK KO and intro-16 Start, requiring scene 17 with a fighter.
- `adventure-maze.log` exits 134 loading /GrNSr.dat during the next intro,
  falling into lbArchive_800171CC retail DAT parsing. Opening, Mario/Peach,
  small DK and Giant DK transitions were passed first. All GPU handles terminal.
- Added owned It_Kind_Likelike article decoding. Its common record is five
  mixed scalar words; special record expands the leading pointer, preserves
  fourteen scalar words and four packed bytes, and converts two 9-word hit
  descriptors. It has ten animation records. `maze-likelike-test.log` passes
  source poisoning/freeing, exact fields, all ten model animations and cleanup.
- CPU hsd_scene_probe build passed. Startup/runtime relink is now running.
  The new article is not yet wired into a maze stage adapter; gameplay remains
  blocked at GrNSr loading. Do not claim maze playable or package this as a fix.
- GrNSr data offsets: map_head 0x3f0, coll_data 0x6e9c0, map_ptcl 0x6ea00,
  map_texg 0x6f1a0, grGroundParam 0x78b04, itemdata 0x78be0,
  yakumono_param 0x78c7c, ALDYakuAll 0x78dc0, map_plit 0x82974,
  quake_model_set 0x18e108. Item table: one kind 0xd5 (Like Like), entry
  0x824c0, article 0x824a8, ten state records at 0x82408, special 0x822b8,
  common 0x822a4; then NULL.
- Maze parameters: first four words point to seven-word color scripts
  0x78be8/0x78c04/0x78c20/0x78c3c; fifth points to nine-word record 0x78c58
  (0x78c7c immediately follows); then four floats, one int, and spawn records
  beginning 0x78ca4. There are 64 spawn records before script 0x78da4, referenced
  by ALDYakuAll. Do not assume 80 inline spawn records like Kinoko Route:
  copying 80 would include commands and other data. The generator has 80 slots
  but only reads descriptors for present map spawn points. Verify map count.
- grshrineroute.c's private YakumonoParam wrongly types the first five pointers
  as ints. First four are passed to grMaterial_801C9604; native typed alternative
  grMaterial_ApplyColorScript already exists. Fifth is returned through
  grShrineRoute_8020AE08's s32* output into ftDevice callback path; its native
  pointer output must be handled before maze environmental effects are safe.
  Stage color script owner API: melee_stage_colors_decode(table,count).

Startup and runtime relink completed (maze-likelike-runtime-build.log, exit 0).
All build/test handles are terminal. App packages remain at the last verified
jungle-cache fix; no maze app packaging performed because stage integration is
unfinished. Next concrete task: native GrNSr stage parameter/color/spawn owner,
Like Like registration, then source-disposal and GPU intro/gameplay checks.


Underground Maze native stage adapter checkpoint:
- Added melee_maze_stage_decode / STAGE_MAZE and GrNSr loader routing.
  Owns Like Like (10 records), four stage color scripts, a 9-word damage
  descriptor, scalar parameters and 64 packed spawn records. Allocates 80
  native spawn slots with unused tail zeroed, avoiding retail adjacent data.
- Moved grShrineRoute_YakumonoParam into grshrineroute.h with native pointer
  fields. Native color callers use grMaterial_ApplyColorScript. Environmental
  damage callback now returns its descriptor through intptr_t* (retail remains
  32-bit by platform), matching the caller's pointer-sized destination.
- maze-stage-archive-test.log passes source disposal, scripts, exact hit/scalar/
  packed spawn fields, Like Like model loading and HSD pool cleanup.
- First full GPU retry reached maze gameplay initialization, then ASan found
  an overread in LObjLoad from grShrineRoute_8020AB58. Its point-light descriptor
  pointed at a lone float. Both maze temporary lights now use complete native
  HSD_LightPointDesc {16,0,GX_DA_OFF}, retaining retail initializers under #else.
- Startup/runtime builds passed. Full adventure-maze GPU retry with light fix
  is running; no app packaging yet. Common maze enemy articles may need further
  conversion once the spawner runs. Do not claim maze gameplay verified yet.

Maze light-fix retry reached scene gameplay and the enemy spawner, then stopped
on missing native item 44 (ReDead). GPU handle is terminal. Added common ItCo
ReDead (10 animation records), Octorok (5), and Octorok stone (1, no special
attributes) owners. Nested 5-word common attributes and mixed scalar/halfword/
byte special fields are converted. Full startup/runtime builds passed.
The expanded --items-data regression now verifies exact source fields after
source disposal and all 16 added animation records, alongside previous items.
That CPU regression is running in maze-enemies-archive-test.log. No GPU or app
packaging is currently live; wait for this CPU handle before the next replay.

Maze combined verification passed:
- Expanded ItCo.usd suite (maze-enemies-archive-test.log) exits 0, including all
  ReDead/Octorok/stone field and animation checks after source disposal.
- adventure-maze.log exits 0 at 14500 submissions: mode 4, scene 17, one live
  fighter. Maze intro scene 16 at 8764, gameplay scene 17 at 9005; continued
  more than 5000 submissions afterward without crash. Final PNG was inspected
  and shows Fox, maze geometry, sword marker, lighting and HUD. This proves
  entry and stationary runtime, not room traversal, environmental damage,
  grabbing enemies, sword encounters, or exit completion.
- Native maze parameters and temporary light fixes are integrated; common
  ReDead/Octorok/projectile assets are registered. All GPU/CPU handles terminal.
- Mac/device/Simulator app rebuilds now running in *-adventure-maze-app-build.log.
  Do not run another GPU test until packaging is complete.

All three app builds finished (exit 0): mac-adventure-maze-app-build.log,
iphoneos-adventure-maze-app-build.log, iphonesimulator-adventure-maze-app-build.log.
Mac and Simulator packages and unsigned physical-iOS package now contain the
verified maze-entry changes. No build/test handles remain live. Next work:
maze traversal/environment damage/sword encounters/exit, later Adventure
coverage and the original goal's remaining hardware/UI/MetalFX requirements.


Maze sword encounter follow-up:
- Found grShrineRoute_80208F70 storing the temporary arena GObj in u32 xD4;
  changed grShrineroute_GroundVars.xD4 and its assignment to uintptr_t.
- Added native symbols[6] to grShrineroute_GroundVars and switched all maze
  symbol operations through SHRINE_SYMBOLS to the native fields; retail keeps
  its original map union view. This removes dependence on unrelated map layouts.
- Added --adventure-maze-link (focus 16, 16500 submissions) extending the maze
  route. Selects a real sword symbol (map_id 1), places Fox at its marker using
  airborne physics/camera alignment, observes Link, uses controlled KOs, and
  requires state 0 with that sword's completion bit set. It does not force
  encounter state or completion flags. The regression is currently running.
- Startup/runtime builds passed. No app packaging performed yet; verify the
  encounter before rebuilding distributable packages.

Initial maze-Link test ran to 16500 without a runtime crash but exited 6:
Link never spawned (phase remained 0). Final PNG showed Fox in the water room
being grabbed by ReDead. The test placed Fox at marker height, while encounter
activation requires real contact on collision joints 8–13 (xCA/xCC flags).
The fixture now places Fox 60 units above the marker briefly and lets him
land normally, with position/ground-line/detector diagnostics. This retry is
running; encounter pointer fix is not yet verified in its active path.

Maze Link placement diagnostics: the above-marker retry also ran to 16500
without a runtime crash but never spawned Link (exit 6). Fox was clamped to
x341.3 despite the marker being x600.8: the fixture updated fighter positions
but retained collision sweep history. The fixture now uses mpColl_80043680 to
reset current/previous/last collision positions before releasing the fighter.
Startup/runtime build passed; another windowless replay is running. Production
encounter-pointer changes remain pending active-path verification.

The raw GrNSr map_head point mapping was independently inspected: no markers
0x60 through 0x6f exist. grZakoGenerator assigns slots 64–79 those marker IDs
and reads spawn data only after marker lookup succeeds. This confirms the
zero-filled tail of the 64-record native maze spawn conversion is unused.

Maze Link encounter verification now passes (adventure-maze-link.log, exit 0,
16500 submissions). Correct collision-history reset allows normal landing on
the sword trigger. Arena phases: 1 at 9234, 2 at 9270, 3 at 9296, 4 at 10438,
5 at 10468, and exploration phase 0 at 10494 with completedMask=1. Final result
seen=1 cleared=1 mode=4 scene=17; continued about 6000 submissions after cleanup.
This actively verifies the uintptr_t temporary-arena handle and native symbol
array paths. Uses controlled placement, stocks, and opponent KOs; it does not
establish manual traversal or all sword rooms. Final framebuffer inspected:
maze geometry, HUD, items, and P1 visible; Fox partly occluded by a crate.
Mac/device/Simulator package builds running in *-adventure-maze-link-app-build.log.

All three maze-Link app builds passed (exit 0) after rerunning with the Swift
compiler plugin sandbox access required by Xcode. macosx-game/MeleeNative.app,
iphoneos-game/MeleeNative.app (unsigned), and iphonesimulator-game/MeleeNative.app
now include the verified sword-arena pointer and symbol fixes. All process
handles terminal. Remaining Adventure work: other maze rooms, hazards/enemies,
exit and subsequent stages; full goal and MetalFX remain open.

User requested completion of all feasible Mac/Simulator work before connecting
an iPhone. Current follow-up:
- Isolated Mac VM responds over guest.sh but remains at loginwindow; a guest
  screenshot was black. Asked user to unlock that VM; host desktop untouched.
- Current Simulator testTouchAdvancesStartup passed (ios-ui-current.xcresult).
  Reviewed final attachment: Regular Match menu, not VS as its filename says.
  Touch labels are black-on-black in light mode. Changed game-overlay labels
  to white and neutral borders to translucent white; pending new visual check.
- Apple input tests passed (apple-input-current.log).
- Added focus 17 --adventure-maze-exit to place at the real exit symbol after
  a completed Link encounter, advance Zelda and require Samus scene25.
  Current windowless GPU session running, adventure-maze-exit.log.
- Found app runtime never inserts a memory card. Added run_with_save API;
  GameView supplies Application Support/MeleeNative/MemoryCardA.card. Runtime
  fails with an explicit message if existing card cannot be opened, preserving
  it. Existing two-argument run remains cardless for diagnostic compatibility.
  Runtime and probe build passed. Card store/backend/request tests passed.
  Actual game save/reload and these changes' app packaging still pending.

Further verified progress:
- adventure-maze-exit.log passes 18000 submissions: Link cleared, maze exit,
  Zelda fight then Samus scene25 with two fighters. Final frame inspected.
- Added optional MetalFX 2x/3x spatial presentation (Off by default) and graphics
  sheet; Mac typecheck passed. test_metalfx.swift passed actual GPU dimension,
  quadrant/channel/orientation and input-reuse tests at both scales; generated
  gameplay PNG inspected. Simulator SDK has no MetalFX module: explicit
  canImport fallback keeps original image and disables the options there.
- Save-enabled runtime first revealed filename memcpy reading beyond a short
  string; native uses bounded strncpy. Then found raw reserved embedded
  CardState layout in lbcardnew.static.h: native initialization overwrote tasks.
  Replaced with owned typed CardState and x28/x4C field aliases. Game creates
  98260-byte test card, and fresh process reads its data successfully.
  runtime-save-first.log and runtime-save-reload.log both pass sequence600,
  pause/resume and audio producer tests. Actual unlock/options persistence
  remains to be checked beyond basic original save creation/reload.
- Added opt-in MELEE_CARD_TRACE operation logging for diagnostics.
- Mac and unsigned iPhone app builds passed. Simulator rebuild with unsupported
  MetalFX fallback running. Do not run GPU tests while packaging is live.

Save setting roundtrip passed: runtime-save-setting-write.log creates a fresh
card with P1 rumble disabled at a paused checkpoint before normal confirmation;
runtime-save-setting-read.log starts a new process and asserts loaded rumble=0.
Both pass 600 rendered frames, pause and audio checks. Native-only diagnostic
wrapper lives behind MELEE_RUNTIME_PROBES; C helper avoids Aurora/GameCube type
header collisions. Latest packaged Mac runtime predates this diagnostic helper.

Simulator dark verification corrected: launch -AppleInterfaceStyle did not
change UIKit traits. New test uses externally selected Simulator appearance;
ios-ui-dark.xcresult passed and both screenshots inspected. Restored light.
Graphics sheet and touch overlays are legible. No iOS MetalFX SDK, so fallback
is explicit under canImport(MetalFX).

Brinstar transition focus18 reproduced unconverted Vi0401.dat parsing at scene26.
Added two-scene native cutscene conversion, bounded cached LoadSymbols owners,
IfAll HUD bridge and Pl*DViWaitA* nested-motion owners. ftDemo arr_idx3 loads
native motion13. CPU tests: two scenes/three models animate after source free;
nested wait-motion buffer survives source free (its extent includes alignment
padding, unlike its inner DAT header length). Both tests pass.
Full windowless focus18 retry now running in adventure-brinstar-escape.log.
Focus18 now uses Start for the cutscene and stops once escape scene27 has run
600 submissions, avoiding a stationary-player timeout. No packaging is live.

The first cutscene-conversion full retry exited 6 at 19000 while still in maze:
Link cleared, but randomly chosen exit marker1 at y238.1 was approached from
60 units above; Fox landed on an upper platform at y293.6. No runtime crash.
Both sword/exit fixtures now drop 15 units above the actual marker with collision
history reset, avoiding overhead floors. Latest startup build passed.

Added GrNZr.dat escape stage adapter: two scalar words camera_timer=1200,
zako_spawn_chance=1, native models/collision/lights/quake/scripts, no stage items
or particle banks (confirmed absent map_ptcl/map_texg). Initial generic adapter
expected particle banks; dedicated no-particle variant fixes that. CPU source-
disposal/model tests pass (zebes-route-archive.log / zebes-route-models.log).
No full escape-stage runtime proof yet. All CPU build handles terminal.

iPad Simulator UI tests currently running (ipad-ui-current.xcresult/log),
graphics test passed and touch startup test in progress. No native GPU probe
or app packaging live; wait for UI handle before retrying focus18.

iPad graphics and touch tests both report success (2 tests, 0 failures), but
xcodebuild remains in collectSimulatorDiagnostics semaphore wait. Sample saved
in ipad-xcode-result-sample.txt. ps confirms neither MeleeUITestGame nor its
UI runner remains. Therefore there is no live Simulator game/GPU probe; started
next windowless Adventure retry while the independent diagnostic collector
finishes. Do not package apps while this native GPU retry is live.
Latest retry includes Vi0401/IfAll/demo-wait owners, GrNZr no-particle adapter,
15-unit sword/exit placement with reset collision history, and early success
once escape scene27 has run 600 submissions. No successful escape entry yet.
Added reproducible test_runtime_save.py and test_metalfx.py wrappers, not yet
run as wrappers (their underlying checks already passed).


### Physical-device fixes (September 14, 2026)

Item scripts retain only reachable command words; explicit branches are remapped
into the compact native allocation. Serialized scene data uses explicit shared
immutable archive ownership within an article and nested scenes. Borrowed inputs
are copied once, and owners remain valid after their source is disposed. The
294 asset/animation regressions and sparse command-graph tests pass.

The AX decoder accepts an unused loop target when looping is disabled. Synth
must also ignore stopped stream channels while its staggered node cleanup runs;
stopped voices can point at the fallback address outside the stream ring. Tests
cover mono/stereo completion and the retail opening track. The physical iPhone
passed the complete opening into attract-mode gameplay with its audio engine and
mixer active; this does not establish audible speaker output.

MetalFX presentation waits until its MTKView is attached and laid out and submits
inside the normal draw callback. Actual A15 GPU captures verify 2× and 3× output.
Persistent drawable allocation failure falls back to the original framebuffer.

For tethered runtime checks, explicitly launch with MELEE_DEVICE_CONTROL=1 and
MELEE_DEVICE_DIAGNOSTICS=1, then use `native/tools/device_control.py --help`.
The fixture accepts bounded pad input through Melee's own Documents/Diagnostics
folder, ignores stale commands after relaunch, and waits for fresh frames and,
when requested, completed scaled output. These synthetic commands do not verify
physical touchscreen delivery. Normal home-screen launches have neither fixture
enabled and automatically open the user's Documents/Melee.ciso if present.


### Physical iPhone audio gaps (September 14, 2026)

The user reported static/choppy sound despite the engine remaining active.
Lock-free audio-ring counters established that the phone's 48 kHz, 1024-frame
I/O quantum requests up to 683 frames from the 32 kHz source. The old producer
ceiling was only 640 frames, guaranteeing silence padding on every callback.
The baseline measured 39,979 missing of 639,659 requested frames, with 937
underrun callbacks (`build/iphone13-audio-baseline-state.json`).

The producer now buffers 1,600 frames (50 ms) and adapts upward to larger
observed device requests, maintaining one quantum plus 640 frames of
scheduling headroom, rounded to 160-frame AX blocks and bounded by the ring.
The real-time consumer remains free of locks, allocation, logging, and game
work. Lifetime atomic counters expose requested/missing frames, underrun
callbacks, and maximum request size through opt-in device diagnostics.

Regression tests cover the real 683-frame quantum and a change to 1,366
frames, preserving ordered nonzero stereo samples and producer pacing.
ASan/UBSan and ThreadSanitizer transport/mixer tests pass; offline Apple audio
conversion at 32/48 kHz and interruption/recovery tests pass. See
`build/audio-buffer-regression.log`, `build/audio-buffer-tsan-test.log`, and
`build/audio-buffer-apple-test.log`. These verify buffering and conversion;
subjective quality still requires listening on the phone.

The fixed iPhone build delivered 10,990,934 source frames (343 seconds) with
zero missing frames and zero underruns on the same 48 kHz / 21.33 ms route.
The run includes startup, menus and Classic Mario versus Ice Climbers on
Icicle Mountain with movement/special input. Evidence is in
`build/iphone13-audio-fixed/gameplay/received/state.json` and `frame.png`.
The fixed signed app was reopened without diagnostics or synthetic input.


### iPhone frame pacing (September 14, 2026)

The user confirmed the audio fix, then reported ongoing uneven motion. The
renderer already uses Aurora's Metal backend. Timing instrumentation separates
CPU submission, GPU-completion waits, readback, game-frame intervals, UI
sampling and skipped source frames; it also records thermal state and audio
underruns. `MELEE_DEVICE_DIAGNOSTICS=1 MELEE_PERF_ONLY=1` writes bounded
`Documents/Diagnostics/performance.jsonl` without PNG encoding. Always start a
fresh process with these flags: a previously running app retains its original
environment, and old diagnostic files are not evidence of current progress.

The baseline's last 20 two-second windows generated 2,410 frames but delivered
only 2,082 to the UI, skipping 328 (13.6%). The phone reported nominal thermal
state. Median per-window maximum GPU wait was 4.04 ms and maximum readback
was 2.10 ms. Evidence: `build/iphone13-performance-baseline.jsonl`.

The UI now owns a persistent clock instead of reconstructing a Combine timer
with its SwiftUI view. iOS uses a 60 Hz CADisplayLink. After sampling input and
consuming a completed frame, it signals the VI worker to begin the next game
retrace. Game callbacks remain on that worker under the interrupt gate; the UI
never performs game or GPU work inline. Pulses coalesce, rapid bursts are
ignored, and a 1000/1001 phase accumulator retains the original NTSC cadence
(one repeated display refresh per 1001 pulses). Pausing/stopping the display
link restores the autonomous monotonic retrace clock. macOS retains an owned
timer and the independent VI clock.

Native VI tests cover pulse coalescing, callback thread ownership, switching
clocks, shutdown with no pending pulse, callback ordering and interrupt-gate
semantics under sanitizers. GPU rendering still includes CPU readback for
SwiftUI presentation; this change does not claim a GPU-only display path.

The final phone Classic session's last 20 windows delivered all 2,398 generated frames
to the UI over about 40 seconds (59.85 FPS), with zero skipped source frames,
zero audio underruns and nominal thermal state. No game-frame interval
exceeded 33.37 ms; 27 exceeded 20 ms, so this does not claim perfectly uniform
simulation timings or measured panel scanout. The sample was saved before
debugger attachment in `build/iphone13-performance-final.jsonl`. This is a
different run from the baseline, not a deterministic same-match benchmark.
Simulator background/foreground and settings-resume frame-progress tests pass,
as does graphics/touch appearance (`build/iphone-display-paced.xcresult`).

A later own-app screenshot shows Classic first-round results at 55,200 points
(`build/iphone13-paced-results.png`). The timing sample is not an isolated
combat benchmark. The final signed build was reopened normally with
`--terminate-existing` and no diagnostic or synthetic-input environment.
