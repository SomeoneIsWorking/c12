# Configuration

C-12's own knobs. Each is a psxport CVar (`runtime/psx/config`): launch environment overrides the saved
settings file, which overrides the default, and `enh()`/`enh_int()` read 0/off in a comparison run
(`PSXPORT_DIAGNOSTIC_RUN`), so a recordcheck or oracle run never sees an enhancement. Framework knobs are
in `external/psxport/docs/config.md`.

| Variable | Default | Meaning |
| --- | --- | --- |
| `PSXPORT_C12_WIDESCREEN` | on | A 16:9 canvas that shows more world, not a stretch (`c12::WidescreenPolicy`). Persistable. |
| `PSXPORT_C12_DRAW_DISTANCE` | 50 | The INCREASE of the world draw distance and its fog, in percent. 0 is the retail distance, 100 doubles it, 1000 is eleven times. Range 0..1000 in steps of 25; an off-grid value snaps to the nearest step. Persistable. |

## Draw distance

`c12::DrawDistance` (`game/render/draw_distance.h`) is the one place the percentage turns into a scale
factor, `retail * (100 + percent) / 100`. Everything the world pass culls or fades by scales through it:

- the view frustum footprint the visible-cell collector walks (`game/render/cell_collector.cpp`), and the
  ground distance limit at `0x800F0838`;
- the terrain pass's far clip, the fog far word at `0x800F9238` (`game/render/world_mesh_pass.cpp`);
- the fade tables (`game/render/fog_table.cpp`), stretched so the whole ramp moves out in proportion.

The setting is in the in-game menu, Display pane, under Enhancements, as a stepped slider (psxport's generic
`TitleIntSetting` row; the title declares it from `c12::titleIntSettings`). It is saved with the other
settings and applies on the next frame.

At 0 the collector and the terrain pass produce the retail buffers, and nothing else changes.

Above 0 the collector first collects the grown footprint into a host list ordered nearest first, then runs
the retail pass, so the guest's 150-cell buffer, its pointer list, the visible flags and the GTE state are
exactly the retail result for every other guest reader. The terrain pass draws the host list instead of the
guest buffer and keeps its visited-polygon list host side. The guest's packet pool is still fixed
(`kPacketPoolWindows`): when a frame draws more than it holds, the pass stops, and because the list is
nearest first the farthest terrain is the part dropped.

The default is 50: half again the retail distance, which costs about 30 percent more frame time in the
first-mission street (harness fields/s from field 1900: 0 27.2, 25 24.6, 50 21.0, 100 14.0, 300 13.1). 100 and
up roughly double the frame time there, because the extra terrain is drawn by the guest's own GPU stream.
