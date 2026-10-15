# Features — C-12: Final Resistance

Title hops only. Each row is where C-12 joins a psxport feature hop (`psxport F-X.n`, psxport
`docs/features.md`); a feature with no row runs on the framework alone.

## F-BOOT

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-BOOT.1 | `game/entry/catalog.cpp:createCatalog` — the `TitleIdentity` from `title.json` and a `c12::Runtime` |
| 2 | psxport F-BOOT.6 | `game/runtime/runtime.cpp:c12::Runtime::registerOverrides` — binds `StockVSync` and `GuestInterruptLibcd`, installs the four natives below |
| 3 | psxport F-BOOT.6 | `game/runtime/session.cpp:c12::Session` — per-Core `WorldVisibility` |

## F-TURN

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-TURN.2 | `game/frame/frame_driver.cpp:c12::FrameDriver::stepFrame` — logic frame, one turn of `kCyclesPerTurn`, one commit, audio |
| 2 | psxport F-TURN.2 | `game/frame/vsync_continuation.cpp:resumeVsyncContinuation` — resumes at `r31` after VSync, or ends the run |
| 3 | psxport F-TURN.4 | `game/runtime/runtime.cpp:c12::Runtime::reportRun` — fields and VSync boundaries |

## F-CD

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-CD.1 | `game/runtime/runtime.cpp:c12::Runtime` — stock `CD_cw` at `0x800B4CA8`, work area `0x800EEED0`/`D4`, ready slot `0x800EEEBC` delivered by the guest's INT1 |
| 2 | psxport F-CD.1 | `game/cd/cd_command_completion.cpp:completeCdReadySyncCommand` — runs `CdReadySync` (`0x800ABD98`), then the command-status callback at `0x800EEEB8` |
| 3 | psxport F-CD.3 | `game/runtime/runtime.cpp:c12::Runtime::guestCodeModuleWindow` — the module arena `0x80105E40..0x801FFFF0` (`RELOCS/GT.LVB`) |

## F-RENDER

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-RENDER.1 | `game/runtime/runtime.cpp:c12::Runtime::renderCapabilities` — Record, no producers, guest VRAM is the picture |
| 2 | psxport F-RENDER.2 | `game/runtime/runtime.cpp:c12::Runtime::packetPool` — one window `0x800E3DE4..0x801B0F9C` |

## F-WIDE

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-WIDE.1 | `game/widescreen/widescreen_policy.cpp:WidescreenPolicy` — 16:9 when `PSXPORT_C12_WIDESCREEN` is on |
| 2 | psxport F-WIDE.2 | `game/render/view_frustum.cpp:rebuildViewFrustumCorners` — `FUN_800660D8` widened with the canvas |
| 3 | psxport F-WIDE.2 | `game/render/world_mesh_pass.cpp:drawWorldMeshPass` — `FUN_8006769C` screen reject widened by the margin |

## C12-DRAW (title only)

| hop | owner |
|---|---|
| 1 | `game/title/enhancements.cpp:titleIntSettings` — the stepped `PSXPORT_C12_DRAW_DISTANCE` menu row |
| 2 | `game/render/draw_distance.h:DrawDistance` — the one percent-to-scale rule |
| 3 | `game/render/cell_collector.cpp:collectVisibleCells` — `FUN_800684C0`, retail buffer plus the host list |
| 4 | `game/render/world_mesh_pass.cpp:drawWorldMeshPass` — draws the host list with the scaled far clip |
| 5 | `game/render/fog_table.cpp:FogTables` — fade tables stretched by the same scale |
