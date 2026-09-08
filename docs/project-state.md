# Project state

## Comparison baseline

The baseline is the unmodified USA PlayStation release of *C-12: Final Resistance* on original
hardware or a PS1 emulator. The intended product authenticates the user's image, executes any
title-owned native overrides directly, and translates every remaining MIPS instruction at runtime
through psxport's pinned Lightrec revision.

## Current focus

**S003** — Compose the authenticated title over psxport's per-`Core` Lightrec executor and recover
the field lifecycle needed by the player product. Interpreter-only execution remains diagnostic;
backend fallback must be bounded and explicitly counted. The deleted static product is not a bridge
or oracle.

## Capability inventory

| ID | Capability / observable outcome | State | Dependencies | Goals |
| --- | --- | --- | --- | --- |
| S001 | The USA disc resolves through `SYSTEM.CNF` to authenticated `SCUS_946.66` | verified | — | G001 |
| S002 | The pre-migration execution boundary is recorded through first VSync and guest PC `0x800A7F90` | verified | S001 | G001 |
| S003 | The gameplay product executes non-native guest code dynarec-first with bounded, reason-coded fallback accounting | partial | S001, shared psxport executor | G001 |
| S004 | The authenticated program returns from first VSync and executes beyond `0x800A7F90` through Lightrec | missing | S003 | G001 |
| S005 | Representative interactive gameplay passes with correct input, timing, interrupts, devices, audio, rendering, and per-host frame time | missing | S004 | G001 |
| S006 | The offline translator, generated corpus, static dispatcher, seed-only metadata, and static-only checks are deleted without a compatibility mode | verified | — | G001 |
| S007 | Hosted CI truthfully distinguishes repository policy from native product support on Linux, Windows, macOS, and Android | partial | S003 | G001 |
| S008 | Widescreen renders additional source geometry with correct projection and culling | missing | S009 | G002 |
| S009 | Native scene construction produces the C-12 picture from recovered title state | missing | S005 | G002 |
| S010 | 60 fps source-geometry interpolation preserves authored simulation timing | missing | S009 | G002 |
| S011 | Asynchronous loading removes waiting-only screens; logos/sequences support complete cancellation | missing | S005 | G002 |
| S012 | No-terminal file picker authenticates the complete user installation, including bounded nested ZIP input | missing | S001 | G003 |
| S013 | Saves and settings persist in OS application data with reset/reselection support | missing | S012 | G003 |
| S014 | Physical controller and keyboard controls drive the intended title actions | missing | S005 | G002 |
| S015 | Authored SVG touch controls support multitouch, cancellation, safe areas, and controller handover | missing | S014 | G003 |
| S016 | Asset-free Linux AppImage installs and runs qualified gameplay | missing | S005, S009, S012, S013 | G003 |
| S017 | Asset-free Windows package installs and runs qualified gameplay | missing | S005, S009, S012, S013 | G003 |
| S018 | Asset-free macOS application runs qualified gameplay on its declared architectures | missing | S005, S009, S012, S013 | G003 |
| S019 | Android APK provides arm64 dynarec gameplay, SVG touch input, and measured device performance | missing | S005, S009, S012, S013, S015 | G003 |
| S020 | WebAssembly/GitHub Pages release executes the shipping runtime and renders native C-12 gameplay | missing | S005, S009, S012 | G003 |
| S021 | Independent oracle comparison diagnoses execution, gameplay, input, rendering, and audio differences | missing | S003 | G002, G003 |

## Evidence and exact gaps

### S001 — Authenticated target

Evidence: The supplied USA CHD resolves through `SYSTEM.CNF` to `SCUS_946.66`. This proves title
selection and runtime-image input, not gameplay execution.

The 2026-09-08 fresh extraction resolves the executable at LBA 24 and produces 921,600 bytes with
SHA-256 `0b93d073ecc211a51431ee8e3eaf8e72f3aa33e6bc0b985a408c0f37a4cc9c87`.
`tools/title_identity.py` accepts that extraction and rejects missing, wrong-name, ambiguous,
truncated, oversized, non-ASCII, and changed-byte inputs through the production authentication
function. Each file is read once with an explicit byte limit; a stale-size metadata discriminator
proves that later growth cannot bypass those bounds. The standalone
validator is available through `uv run --frozen python -m tools.title_identity DIRECTORY`; connection
to the player launcher's executable loading boundary remains part of S003.

### S002 — Recorded migration boundary

Evidence: The recorded pre-migration path starts `SCUS_946.66`, reaches the previously identified
libetc VSync boundary once, and then refuses guest PC `0x800A7F90`. That address is retained as a
discriminator because it identifies the first path the former static dispatcher did not own. It is
not evidence for a title frame-loop defect or a VSync configuration defect.

### S003 — Native/dynarec product

Partial capability: `c12_boot_probe` authenticates one bounded input buffer from `title.json`, maps
those same bytes through the shared PS-X EXE owner, composes C-12's resident image, and executes through
the per-`Core` Lightrec executor. The 2026-09-08 Clang 22.1.8/Ninja build compiled both the probe and
image contract; all four C++ translation units passed Clang-Tidy, six authored source/header files
passed formatting, and focused source-policy/image CTests passed 2/2.

The exact USA executable completed three 100,000-cycle diagnostic turns with budget-exhausted exits
at `0x800A8464` (100,008 cycles), `0x800A8464` (100,000), and `0x800B84FC` (100,030), reaching `InitHeap`
and `ResetGraph` output. Counters reported 149 translated blocks, 28,858 executed blocks, 139,283
instructions, 8 host dispatches, 28,709 cache hits, 152 misses, 28,714 invalidations, and zero faults.
Across three executor calls, fallback and refused fallback were zero blocks/instructions for every
reported reason. The probe was silent and nonpresenting with scratch persistence overrides.

Gap: this is bounded startup evidence, not the gameplay product, a recovered VSync return, or
native presentation. The title still needs its field lifecycle, device/service continuation, native
picture ownership, and interactive qualification. Lightrec warned that the memory map is suboptimal;
performance remains unqualified. Future native overrides must use complete image identity plus address.
The final pinned configure/build, source/image/style CTests, and executable-boundary positive/negative
checks passed. The style scanner initially misclassified the rejection tuple in `tools/source_policy.py`;
the shared scanner now recognizes its literal `STATIC_PRODUCT_MARKERS` declaration without requiring
a different module location. Focused checks passed after that tooling correction.

### S004 — First dynamic discriminator

Missing capability: execute the authenticated whole program with nonzero Lightrec blocks, cross the
first VSync as an explicit executor exit/resume, and continue beyond `0x800A7F90`. A pass reports both
the reached VSync boundary and subsequent guest execution; silence or merely starting the executable
cannot pass.

### S005 — Representative gameplay conformance

Missing capability: establish and drive a bounded interactive gameplay scenario. Boot, first VSync,
logos, menus, attract mode, and FMV remain checkpoints only. The scenario must prove meaningful input
response and cover relevant guest state, memory, interrupt/timing, devices, audio, rendering, and
frame-time behavior on each released host architecture, using an independent emulator or separately
built test oracle for divergence diagnosis.

### S006 — Static-path removal

Evidence: the tracked emitter/bootstrap path and generated dispatcher were deleted, the ignored
`generated/` corpus and prior static build tree were removed, and `tests/test_source_policy.py`
rejects their paths and source markers. CMake now exposes one explicit failing `c12_port` target that
names the title's unimplemented field lifecycle/native presentation rather than selecting a
compatibility mode.

### S007 — Platform CI coverage

Partial capability: `.github/workflows/ci.yml` now connects the Linux native startup boundary to
`tools.verify` with Clang and frozen Python, including synthetic file/span admission, style, source,
and execution-boundary checks. Its minimal bootstrap fetches the exact `psxport.pin`; the shared
framework setup action owns native packages, dependency revisions, and the Lightning prefix under
`build/deps/`. Bootstrap/setup refusal and positive mocks, the local native build, focused image/source tests, and
C++ analysis pass. All local consumer verification components passed, including executable-boundary
checks. Hosted Linux run [34220898626](https://github.com/SomeoneIsWorking/c12/actions/runs/34220898626)
passed at `58f33fc`, including cold framework/dependency setup and the complete native verifier.
This job does not claim a player package or gameplay support.

| Platform | Applicability | Current CI evidence and exact gap |
| --- | --- | --- |
| Linux x86-64 | applicable product target | Hosted native startup verification passed at `58f33fc`; player packaging and gameplay qualification remain open. |
| Windows x86-64 | applicable product target | Missing: no native/dynarec executable, Windows build, runtime test, or package boundary exists. |
| macOS arm64 | applicable product target | Missing: no native/dynarec executable, Apple-Silicon build, runtime test, or application package exists. |
| Android arm64 | applicable product target | Missing: no title Android package, shared `android-port` integration, native runtime, APK build, or install test exists. |
| WebAssembly browser | applicable product target | Missing: browser translation/backend qualification, native title rendering, browser checks, and a GitHub Pages release. |

Gap: add each native platform job only when it can exercise the corresponding redistributable
runtime/package boundary with synthetic inputs. A duplicated source-policy matrix is not platform
support.

### S008 — Widescreen

Missing capability: recover the source projection and horizontal culling owners, render additional
geometry at wide aspect ratios, and qualify framing without final-image stretching.

### S009 — Native scene construction

Missing capability: recover C-12's scene, geometry, materials, camera, and draw-order owners and
implement native picture construction. Guest-rendered output is not the native product.

### S010 — 60 fps source interpolation

Missing capability: measure the title's simulation cadence, preserve that cadence, and interpolate
explicitly matched source geometry to 60 fps with camera, topology, and scene-cut discriminators.

### S011 — Loading and cancellation

Missing capability: recover lifecycle-complete cancellation paths, remove waiting-only presentation,
and run asynchronous loading without removing authored transitions or fast-forwarding simulation.

### S012 — Player setup

Missing capability: native file selection, direct/ZIP complete-install validation, bounded archive
handling through Lucent, and transactional publication of the player's selected game files.

### S013 — Persistent user data

Missing capability: OS application-data ownership for saves/settings and verified reset/reselection
behavior. Checkout-relative or temporary files are not player persistence.

### S014 — Physical controls

Missing capability: qualify meaningful keyboard/controller actions in interactive C-12 gameplay,
including pause and cancellation. Shared input code presence is not title control evidence.

### S015 — SVG touch controls

Missing capability: authored SVG control art and a reachable, scale-aware action layout using the
same title input policy, with multitouch, cancellation, insets, and connected-controller behavior.

### S016 — Linux AppImage

Missing capability: asset-free AppImage setup, install/launch, native-picture gameplay, and host
performance qualification. The current policy CI is not an AppImage release.

### S017 — Windows package

Missing capability: Windows native build/runtime/package checks and no-terminal player setup,
followed by local real-title gameplay and performance qualification.

### S018 — macOS application

Missing capability: `.app` packaging, player setup, executable-memory/ABI/invalidation qualification,
and real-title native-picture gameplay on each claimed macOS architecture, including Apple Silicon.

### S019 — Android APK

Missing capability: shared `android-port` build inputs, Lucent platform runtime integration,
arm64-v8a dynarec execution, authored SVG touch controls, asset-free setup, and a named-device
rendering/audio/frame-time/thermal qualification matrix.

### S020 — WebAssembly release

Missing capability: browser-capable runtime translation with the same dispatch semantics as desktop,
native C-12 rendering, player input/file setup, and an asset-free GitHub Pages deployment. A browser
interpreter or an unavailable backend cannot substitute for the required dynarec path.

### S021 — Independent oracle comparison

Missing capability: an authenticated C-12 oracle run with matching initial state and input, reached
boundary denominators, first-divergence diagnostics, and representative gameplay/render/audio checks.
