# Codemap — C-12: Final Resistance

Placement and ownership only. Intent lives in `docs/project-goals.md`, factual capability state in
`docs/project-state.md`, and ordered reverse-engineering evidence in `docs/re-frontier.md`.

The framework side of every seam named here lives in `external/psxport` and is mapped in that
repository's `docs/codemap.md`; this page covers what C-12 owns and where a defect in it belongs.

## Directories

| Directory | Namespace | Class / owner | Responsibility |
| --- | --- | --- | --- |
| `game/entry/` | `c12` | `createCatalog` | The product's one-title `psx::host::SingleTitleCatalog`: the `TitleIdentity` (serial, size, SHA-256, header words, disc path) from `title.json` and a `C12Runtime`. `c12_port` is built by `psxport_add_product`; the framework's `main` and `ProductHost` own the command line, provisioning, authentication and the boot (`TitleSession`). |
| `game/frame/` | `c12` | `C12FrameDriver` | The display-field owner. `stepFrame` begins the logic frame through `psx::frame::LogicFrame::begin` (clock, producer census, pad), executes one bounded guest turn, resumes the VSync continuation, commits presentation exactly once and steps audio. A turn that ends any other way requests the end of the run after its fence. |
| `game/frame/` | `c12` | `resumeVsyncContinuation` | The one rule for a guest VSync continuation in `r31`: aligned and owned by a loaded image, or refused and reported. |
| `game/cd/` | `c12` | `installTitleOverrides`, `completeCdReadySyncCommand` | This title's CD completion owner. It runs the guest's own `CdReadySync` body at its measured `0x800ABD98` — so the command is still issued by the guest through the framework's declared `cdCommandAddress` seam — and then delivers the completion that body's own libcd poll would have delivered, to the guest's CURRENT registered command-status callback at `0x800EEEB8`. It is a native override, not a hardware service, because only the native-override table has the one-call suppression scope that makes running the original legal. Installed from `C12Runtime::registerOverrides`, after the executable is mapped. |
| `game/render/` | `c12` | `rebuildViewFrustumCorners`, `drawWorldMeshPass`, `collectVisibleCells`, `CellCollector`, `CellOutput`, `WorldVisibility`, `FogTables`, `DrawDistance`, `widenedFrustumHalfWidth`, `widenedWindowMargin`, `polygonOutsideWindow` | The native overrides that widen and extend world culling: the view frustum corner builder (`FUN_800660D8`), the visible-cell collector (`FUN_800684C0`, `cell_collector`, with its guest and host outputs in `cell_output`) and the terrain polygon pass with its screen reject (`FUN_8006769C`). `draw_distance` is the one scale for the draw distance knob, `fog_table` stretches the fade tables with it. At a 4:3 plan and increase 0 all reproduce the guest. Installed from `C12Runtime::registerOverrides`; `WorldVisibility` is the per-Core context (`createContext`). |
| `game/widescreen/` | `c12` | `WidescreenPolicy` | The title's `GuestWidescreenProjection`: asks for 16:9 when the knob is on. The canvas widens, the guest projection words do not change. |
| `game/facts/` | `c12` | none (measured constants) | The guest addresses, HLE plan, module-arena window, CD ready-callback layout and packet-pool window C-12 publishes to psxport, and `executable_identity.h` generated from `title.json`. Values only; evidence is in `docs/re-frontier.md`. |
| `game/title/` | `c12` | `widescreenCvar`, `drawDistanceCvar`, `titleIntSettings` | The title's pc_enh knobs and the integer settings its menu shows (knob doc: `docs/config.md`). |
| `game/runtime/` | `c12` | `C12Runtime` | The title's `GameRuntime` policy: the program image facts (`bootEntry=DriverOwned`, resident text from the executable header), the declared HLE/streaming/pool facts, the record-path render capability (no native producers, no interpolation), the statement that this title's guest VRAM is its picture, the `C12FrameDriver` it creates and the field count it adds to the run-end report (`reportRun`). Admission is the framework default `admitExecutable`. |
| `tools/` | — | `config`, `launcher`, `title_identity`, `source_policy`, `psxport_fetch`, `verify` | Python side: disc-path resolution and the zero-argument launch, the same identity check outside C++, the retired-static-path gate, framework resolution, and the asset-free verification command. |
| `tests/` | `c12::test` | fixtures and test binaries | The title's runtime-service declarations (including the declared record path, the boot entry and that `Game` installs the render path), `C12FrameDriver` through `FrameLoopShell` over synthetic guests, draw distance and the terrain pass. |
| `replays/` | — | recorded pad files | `first-mission/menu-to-mission-run-right-left.pad`: unkeyed (absolute from boot) pad recording, boot → menu → NEW GAME → story page → briefing skip → first mission → run right then left. Feed with `PSXPORT_PAD_REPLAY`. |

## Who owns it

### The frame turn

`psx::host::TitleSession::step` → `psx::frame::FieldTurn::beginField` → `FrameLoopShell::step` →
`c12::C12FrameDriver::stepFrame` → `psx::frame::LogicFrame::begin` → `Pad::serviceFrame` → `psx::cpu::LightrecExecutor::executeUntilExit` →
`c12::resumeVsyncContinuation` → `psx::frame::FramePresenter::commit` → `SpuAudio::frame` →
`FieldTurn::endField`.

- One turn, one fence: `commit` is the only presentation fence and it runs once per field.
- Nothing in C-12 blocks the turn: loading, disc streaming and any movie playback are guest work
  inside `executeUntilExit`, so the field owner and the control channel resume as soon as the guest
  yields at its VSync boundary. C-12 has no title-side blocking call of its own.
- A guest that never takes its boundary is bounded by `C12FrameDriver::kCyclesPerTurn`, not hung; a
  turn that ends any other way is reported and ends the run through `Game::run`.

### Host input → guest pad

`psx::input::HostInput::poll(windowAvailable)` (the ONE owner of the SDL event queue, the key state,
the open controllers, and the keyboard/game overlay decision; `drainEvents` is its queue drain) →
`psxport Pad::pollHostInput` (consumes that mask) → `C12FrameDriver::stepFrame` calls
`Pad::serviceFrame` before the guest runs → the guest's own SIO0 read chain into its pad words.

- C-12 owns no key-to-bit mapping and no pad override. The framework's `Pad` is the only
  active-low mask owner, and the guest's own scan (`FUN_80043dec`) turns the delivered bits into its
  menu state.
- Whether a live window exists is answered in one place, `gpu_vk_windowed()` (`gpu_vk.h`), which
  gates host input and the audio device; `psx::host::RunBudget` reads it for the headless cap.
- Forced input for a driven run and the debug pad drive come from `Pad` through the control channel's
  `tap` command.

### Guest draw → presentation

Guest GP0 packets → the GPU device (which holds guest VRAM) and `Gp0RecordTap` → `psx::frame::FramePresenter::commit` seals one record per field and presents it through `RecordRasterizer` (`RenderPath::Record`, declared by `c12::C12Runtime::renderCapabilities`, installed by `Game`'s constructor), with `guestVramIsPicture` declaring that this title has no native producer, so
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

The guest's SPU writes → `psxport SpuAudio`, driven once per field by `C12FrameDriver::stepFrame`
and opened by `TitleSession::boot`.

### Debug options and the control channel

`psxport DbgServer::start` from `TitleSession::boot` (`PSXPORT_DEBUG_SERVER` names the port) →
`psx::frame::FieldTurn` services it around each field. The step cap is the process's
`psx::host::RunBudget`, which also lifts it for a live client; a client `quit` or a window close
ends the session and reaches the run-end report. The channel is framework-owned; C-12 declares no
debug option of its own today, so a new one belongs in a title-owned module under `game/` reachable
from the frame owner.

### Boot

`run.sh` → `bootstrap.py` → `tools/launcher.py` (resolves and validates the user's disc, passes the
extracted executable as the product's argument) → `psx::host::ProductHost::run` (`c12::createCatalog`;
with no argument it provisions `scratch/bin/c12/SCUS_946.66` from the disc) → `selectExecutableFile`
(size, header words, SHA-256 against the catalog's `TitleIdentity`) → `psx::host::TitleSession::boot`
→ `GameRuntime::admitExecutable` (framework default) → `C12Runtime::registerOverrides` → no crt0
(`DriverOwned`) → `TitleSession::step` per field.
