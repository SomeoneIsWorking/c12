# Codemap — C-12: Final Resistance

Where C-12's code lives and what each owner owns. Feature chains are in `docs/features.md`; the
framework side is `external/psxport/docs/codemap.md`.

## Owners

| owner | path | owns |
|---|---|---|
| `createCatalog` | `game/entry/catalog.*` | The one-title catalog: `TitleIdentity` from `title.json` and a `c12::Runtime` |
| `c12::Runtime` | `game/runtime/runtime.*` | The `GameRuntime`: stock VSync and libcd objects bound in `registerOverrides`, the native overrides, session, frame driver, module window, packet pool, Record path, run report |
| `c12::Session`, `worldVisibility` | `game/runtime/session.*` | Per-Core title state: the frame's extended cell list |
| `c12::FrameDriver` | `game/frame/frame_driver.*` | One field: logic frame, one bounded guest turn, one commit, audio |
| `resumeVsyncContinuation` | `game/frame/vsync_continuation.*` | Resuming at a VSync continuation in `r31`, or refusing it |
| `completeCdReadySyncCommand` | `game/cd/cd_command_completion.*` | `CdReadySync` (`0x800ABD98`) and the command-status completion it owes |
| `rebuildViewFrustumCorners` | `game/render/view_frustum.*` | `FUN_800660D8`, widened with the canvas |
| `collectVisibleCells`, `CellCollector` | `game/render/cell_collector.*` | `FUN_800684C0`: the retail cell buffer and the extended host list |
| `CellOutput`, `WorldVisibility` | `game/render/cell_output.*` | Guest and host cell outputs; the published list, nearest first |
| `drawWorldMeshPass` | `game/render/world_mesh_pass.*` | `FUN_8006769C`: terrain polygons, the widened screen reject, the extended far clip |
| `DrawDistance` | `game/render/draw_distance.*` | The one percent-to-scale rule for the draw distance knob |
| `FogTables` | `game/render/fog_table.*` | The fade tables, stretched by the draw distance scale |
| `WidescreenPolicy` | `game/widescreen/widescreen_policy.*` | The title's `GuestWidescreenProjection`: 16:9 when the knob is on |
| `widescreenCvar`, `drawDistanceCvar`, `titleIntSettings` | `game/title/enhancements.*` | The title's `pc_enh` knobs and its settings-menu row (`docs/config.md`) |
| measured addresses | `game/facts/title_facts.h` | Guest addresses the title's own code reads; evidence in `docs/re-frontier.md` |
| executable identity | `game/facts/executable_identity.h.in` | Generated from `title.json` |
| launcher | `tools/launcher.py`, `tools/config.py`, `tools/title_identity.py` | Disc resolution, extraction and authentication for the zero-argument launch |
| framework fetch, verify | `tools/psxport_fetch.py`, `tools/verify.py` | The `external/psxport` link and the asset-free gate |
| tests | `tests/` | Runtime services and bindings, the frame driver over synthetic guests, draw distance, the terrain pass |
| replays | `replays/first-mission/` | Boot to the first mission and a run right then left (`PSXPORT_PAD_REPLAY`) |

## Where new work goes

- A guest function C-12 replaces → `game/<subsystem>/`, installed from `c12::Runtime::registerOverrides`.
- Per-Core title state → `c12::Session`.
- A measured guest address → `game/facts/title_facts.h`, with its evidence in `docs/re-frontier.md`.
- A title knob → `game/title/enhancements.*` and `docs/config.md`.
- Anything every title shares → psxport, not here.
