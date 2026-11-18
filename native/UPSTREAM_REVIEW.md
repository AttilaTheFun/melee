# Aurora contribution review — October 17, 2026

Compared the full 27-file local Aurora patch against upstream
`51797212b9283e91cc03c273854d63b2421d8490` (October 12). The port still pins
`749d6ee7a22bdfab78c8ece9047bca5d79aa72ca` (September 4).
The upstream worktree is `../aurora-melee-upstream`, branch
`melee-upstream-contributions`. No change to the port's dependency pin or
working vendor checkout is implied by this review.

Draft PR: https://github.com/encounter/aurora/pull/293

Contribution commit: `8c72063`, pushed to `AttilaTheFun/aurora` on
`melee-upstream-contributions`. All 232 GX FIFO executable tests pass with
float-cast-overflow UBSan after both new tests fail on unchanged upstream.

## Extracted for the draft PR

- `lib/gfx/texture.hpp`: hardware minification encodings 2 and 5 decode to the
  wrong SDK filters. Retain the table correction.
- `lib/dolphin/gx/GXTexture.cpp`: both LOD-bias setters convert negative floats
  directly to unsigned bytes, which is undefined. Retain signed intermediate
  conversion so the final integer-to-byte conversion preserves the signed
  fixed-point bit pattern.
- New FIFO round-trip tests cover all six min filters through both setters,
  and both LOD-bias setters with negative values, quantization and clamping.
  On unchanged latest upstream, the filter test fails and float-cast-overflow
  UBSan aborts on -128 in GXInitTexObjLOD. These are independently reproduced
  bugs, not claims about the unresolved black roof textures.

## Omitted from the upstream candidate

These categories cover every other file in the local patch. Omitted does not
mean safe to delete from the pinned, working port.

| Local changes | Disposition against latest upstream |
|---|---|
| `GraphicsConfig::synchronousPipelines`, blocking-policy changes in pipeline cache | Upstream now exposes `AuroraSetDrawsBlocking` (5179721). Drop our private flag from the proposed upstream patch; adopt the public API during any future full port migration. Upstream blocks at lookup; our deferred encode-time wait is not equivalent. |
| Framebuffer copy/XFB replacements, `GXManage.cpp`, FIFO draw-done completion, recording task bypass | Upstream readback service (6f07a59) overlaps the readback work. Our callbacks additionally enforce GPU completion and cooperate with the port's interrupt gate and frame owner. Do not transplant those hooks or claim readback replaces all their semantics. A migration needs explicit completion/lifecycle tests. |
| `melee_xfb_shutdown` in frame.cpp, VI init/flush/configure hooks | Application-specific callbacks; keep with the archived port, not Aurora. |
| VI render-mode mutex | Not upstream. Our separate retrace thread motivates it; a standalone contribution needs a documented thread-safety contract and regression test. Preserve as a follow-up candidate. |
| Pipeline observer, timing counters, startup preparation barrier and two Metal compiler workers | Not upstream. Preserve as follow-up design work; the latest cache separates persisted shader config from render-target-layout-specific runtime keys and CompiledPipeline, so the old patch cannot be applied as an API-neutral change. Observer callbacks need explicit synchronization/reentrancy guarantees. No claim that latest upstream eliminates shader hitches. |
| Pipeline-cache fault flag and writer DB-pointer access | Potential independent race fix remains; preserve for a fault-injection/TSan reproduction rather than fold an untested concurrency change into the texture fix. |
| Browser CMake provider/core/ImGui changes, frame staging/uniforms, GX pipeline/shader/command-processor changes | Incomplete upstream backend: relies on `melee_browser_*` entry points implemented outside Aurora. Preserve in archive; not a self-contained Emscripten support PR. |
| texture_replacement.cpp narrowing and SIZE_MAX guard | 32-bit/browser portability candidate, not made obsolete upstream. Preserve for a 32-bit build and allocation-overflow test (the original patch guards only one of the two narrowing sites). |
| StorageBufferSize increase from 8 to 64 MiB | Melee-stage workload workaround, not a general default change. Prefer configurable capacity or arena growth with memory-budget tests; retain working workaround in archive. |
| internal.hpp overflow fprintf | Local diagnostic; preserve in archive. An upstream proposal should use Aurora's logging/assertion conventions. |
| shader_info.cpp/.hpp widescreen state and command-processor hook | Hardcoded 4:3→16:9 projection policy specific to this application's presentation. Aurora already has aspect/resolution facilities; do not introduce another global policy without a generic API design. |
| gpu.cpp/.hpp headless initialization overload | Useful but not superseded. It bypasses normal surface initialization and configures our fixed offscreen framebuffer; needs a supported initialization API plus device-loss/resize/shutdown coverage before proposing it upstream. |

The extraction intentionally contains only independently verified, portable
correctness fixes. All other work remains in `native/aurora/aurora.patch` and
its supporting port files; none was erased. Updating the old port to current
Aurora is a separate task from creating this contribution.

## Port provenance and possible archive successor

Our source port forks **doldecomp/melee**, compiles its reconstructed C source
for ARM64, and uses **encounter/aurora** through Dawn/Metal. It was not forked
from Dashdance, melee-pc, or Melee Unlocked.

Dashdance's current README and technical guide describe a static-recompilation
port tracking Melee Unlocked, with its own Metal frontend, Slippi rollback,
replays and Apple UI. Its technical guide reports two local instances staying
in sync for a full online game. It also explicitly says the iPhone/iPad version
has only been tested in Simulator, not on physical devices. Its performance
report measures an M5 Pro Mac, not iPhone performance, and skips draws while
new shader pipelines compile. Older offline-acceptance notes contain unresolved
scripted-navigation/save-notice issues; these are not proof of current manual
play failure, but prevent an unqualified completeness claim.

Recommendation: preserve this port as a reference archive and link Dashdance
as an actively developed alternative, not a verified drop-in replacement.
Our physical iPhone/controller tests, completed single-player runs, shader
coverage and diagnostics remain useful evidence. Do not mark the GitHub
repository archived until the source, documentation, provenance and known
limitations are published and the user chooses that final step.

Sources:
- https://github.com/encounter/aurora/commit/51797212b9283e91cc03c273854d63b2421d8490
- https://github.com/encounter/aurora/commit/6f07a59
- https://github.com/TheAndersMadsen/dashdance/blob/main/docs/TECHNICAL.md
- https://github.com/TheAndersMadsen/dashdance/blob/main/docs/PERFORMANCE.md
- https://github.com/TheAndersMadsen/dashdance/blob/main/docs/OFFLINE_GAMEPLAY_ACCEPTANCE.md
