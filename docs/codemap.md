# Codemap — C-12: Final Resistance

Placement and ownership only. Intent lives in `docs/project-goals.md`, factual capability state in
`docs/project-state.md`, and ordered reverse-engineering evidence in `docs/re-frontier.md`.

The framework side of every seam named here lives in `external/psxport` and is mapped in that
repository's `docs/codemap.md`; this page covers what C-12 owns and where a defect in it belongs.

## Directories

| Directory | Namespace | Class / owner | Responsibility |
| --- | --- | --- | --- |
| `game/entry/` | `c12::app`, `c12::probe` | `runPlayer`, `observe` (plus each translation unit's `main`) | The two executable entry points. Each parses its arguments, composes a `Machine`, and hands the run to the owner of the next step. No boot policy or field logic lives here. |
| `game/boot/` | `c12` | `Machine` | The whole machine for one run: the installed title runtime policy, the authenticated image, and the `Game` carrying 2 MiB of guest RAM. Construction installs the policy, binds the per-Core devices and resolves the render path (`render_path_install`, the title's declared `RenderCapabilities`); `mapExecutable` maps the guest executable and reports the framework's refusal. |
| `game/frame/` | `c12` | `GuestFieldLoop` | The display-field owner. `run` turns fields until a frame limit or an unresumable turn; `stepField` latches input, executes one guest turn, resumes the VSync continuation, and crosses the presentation fence exactly once per field. |
| `game/frame/` | `c12` | `resumeVsyncContinuation` | The one rule for a guest VSync continuation in `r31`: aligned and owned by a loaded image, or refused and reported. Shared by the player and the probe. |
| `game/cd/` | `c12` | `installTitleOverrides`, `completeCdReadySyncCommand` | This title's CD completion owner. It runs the guest's own `CdReadySync` body at its measured `0x800ABD98` — so the command is still issued by the guest through the framework's declared `cdCommandAddress` seam — and then delivers the completion that body's own libcd poll would have delivered, to the guest's CURRENT registered command-status callback at `0x800EEEB8`. It is a native override, not a hardware service, because only the native-override table has the one-call suppression scope that makes running the original legal. Installed from `C12Runtime::registerOverrides`, after the executable is mapped. |
| `game/render/` | `c12` | `rebuildViewFrustumCorners`, `drawWorldMeshPass`, `widenedFrustumHalfWidth`, `widenedWindowMargin`, `polygonOutsideWindow` | The two native overrides that widen world culling with the canvas: the view frustum corner builder (`FUN_800660D8`) and the terrain polygon pass with its screen reject (`FUN_8006769C`). At a 4:3 plan both reproduce the guest body. Installed from `C12Runtime::registerOverrides`. |
| `game/title/` | `c12` | `widescreenCvar` | The title's `pc_enh` knob (`PSXPORT_C12_WIDESCREEN`); read through `psx::config::enh` so comparison runs suppress it. |
| `game/widescreen/` | `c12` | `WidescreenPolicy` | The title's `GuestWidescreenProjection`: asks for 16:9 when the knob is on. The canvas widens, the guest projection words do not change. |
| `game/facts/` | `c12` | none (measured constants) | The guest addresses, HLE plan, module-arena window, CD ready-callback layout and packet-pool window C-12 publishes to psxport. Values only; evidence is in `docs/re-frontier.md`. |
| `game/runtime/` | `c12` | `C12Runtime` | The title's `GameRuntime` policy: the authenticated program's image facts, the declared HLE/streaming/pool facts, the record-path render capability (no native producers, no interpolation) and the statement that this title's guest VRAM is its picture. No behaviour of its own — psxport owns the behavior these facts select. |
| `game/image/` | `c12` | `authenticateImage`, `readAuthenticatedImage`, `AuthenticatedImage`, `ExecutableIdentity`, `kUsaIdentity` | Exact-revision admission of the USA executable: size, SHA-256, then PS-X EXE parse. Nothing about the executable is trusted before this returns. `kUsaIdentity` is materialized by CMake from `title.json`. |
| `tools/` | — | `config`, `launcher`, `title_identity`, `source_policy`, `psxport_fetch`, `verify` | Python side: disc-path resolution and the zero-argument launch, the same identity check outside C++, the retired-static-path gate, framework resolution, and the asset-free verification command. |
| `tests/` | `c12::test` | fixtures and two test binaries | Admission contract for the image and the title's runtime-service declarations, including the declared record path and that `Machine` installs it. |
| `replays/` | — | recorded pad files | `first-mission/menu-to-mission-run-right-left.pad`: unkeyed (absolute from boot) pad recording, boot → menu → NEW GAME → story page → briefing skip → first mission → run right then left. Feed with `PSXPORT_PAD_REPLAY`. |

## Who owns it

### The frame turn

`c12::app::runPlayer` → `c12::GuestFieldLoop::run` → `GuestFieldLoop::stepField` →
`psx::cpu::LightrecExecutor::executeUntilExit` → `c12::resumeVsyncContinuation` →
`psx::frame::FramePresenter::commit` → `SpuAudio::frame` → `DbgServer::service` /
`DbgServer::honourPause`.

- One turn, one fence: `commit` is the only presentation fence and it runs once per field.
- Nothing in C-12 blocks the turn: loading, disc streaming and any movie playback are guest work
  inside `executeUntilExit`, so the field owner and the control channel resume as soon as the guest
  yields at its VSync boundary. C-12 has no title-side blocking call of its own.
- A guest that never takes its boundary is bounded by `GuestFieldLoop::kCyclesPerTurn` and reported
  by `stepField`, not hung.

### Host input → guest pad

`psx::input::HostInput::poll(windowAvailable)` (the ONE owner of the SDL event queue, the key state,
the open controllers, and the keyboard/game overlay decision; `drainEvents` is its queue drain) →
`psxport Pad::pollHostInput` (consumes that mask) → `GuestFieldLoop::stepField` calls
`Pad::serviceFrame` before the guest runs → the guest's own SIO0 read chain into its pad words.

- C-12 owns no key-to-bit mapping and no pad override. The framework's `Pad` is the only
  active-low mask owner, and the guest's own scan (`FUN_80043dec`) turns the delivered bits into its
  menu state.
- Whether a live window exists is answered in one place, `gpu_vk_windowed()` (`gpu_vk.h`), which
  gates host input, the audio device, and a title-owned headless frame cap.
- Forced input for a driven run and the debug pad drive come from `Pad` through the control channel's
  `tap` command.

### Guest draw → presentation

Guest GP0 packets → the GPU device (which holds guest VRAM) and `Gp0RecordTap` → `psx::frame::FramePresenter::commit` seals one record per field and presents it through `RecordRasterizer` (`RenderPath::Record`, declared by `c12::C12Runtime::renderCapabilities`, resolved by `c12::Machine` through `render_path_install`), with `guestVramIsPicture` declaring that this title has no native producer, so
guest VRAM is the picture. `PSXPORT_DEBUG=recordcheck` compares each present with the device. `gpu_vk_windowed()` decides windowed versus headless presentation for the
whole run. 60 fps interpolation and widescreen are framework-owned enhancements that this title does
not yet enable (S008/S010 in `docs/project-state.md`).

### CD and streaming

Guest `CdRead`/`CdSync`/command calls → `c12::kPlatformHlePlan` (the admitted measured entries) and
`c12::kCdStreamCallbackLayout` (guest-interrupt delivery) → the framework's CD owner, whose
completion reaches the guest's own registered ready callback slot at `c12::kCdReadyCallbackSlotAddress`.
C-12 owns the addresses and the layout, not the drive. The one thing C-12 owns over the drive is the
guest's own command-completion path: `c12::completeCdReadySyncCommand` (`game/cd/`), because the
libcd body the framework's declared `cdCommandAddress` replaces is where this title's command queue
learns that a command finished.

### Audio

The guest's SPU writes → `psxport SpuAudio`, driven once per field by `GuestFieldLoop::stepField`
and opened by `runPlayer`. The probe deliberately runs silent.

### Debug options and the control channel

`psxport DbgServer::attach` from `runPlayer` (always open on loopback; `PSXPORT_DEBUG_SERVER` names
the port) → `DbgServer::service` between fields in `GuestFieldLoop::run` → `honourPause`. The channel
is framework-owned; C-12 declares no debug option of its own today, so a new one belongs in a
title-owned module under `game/` reachable from the frame owner, never inside the probe.

### Boot

`run.sh` → `bootstrap.py` → `tools/launcher.py` (resolves and validates the user's disc, names the
executable through `PSXPORT_C12_EXECUTABLE`) → `c12::app::runPlayer` → `c12::readAuthenticatedImage`
→ `c12::Machine` → `Machine::mapExecutable` → `c12::GuestFieldLoop::run`.