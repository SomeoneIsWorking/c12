# RE frontier

Ordered evidence chain from the authenticated C-12 image to the native/dynarec gameplay product.
No further static product generation, build, or run is part of this chain.

## Runtime migration

### runtime.target — Authenticate the USA executable

- status: re-verified
- deps:
- evidence: The supplied USA CHD resolves through `SYSTEM.CNF` to `SCUS_946.66`.
- where: title provisioning metadata and `bootstrap.py`
- gap:
- notes: Identity only; no execution capability follows from it.

### runtime.recorded-boundary — Preserve the first reached execution fact

- status: re-verified
- deps: runtime.target
- evidence: The pre-migration path starts the executable, reaches the identified libetc VSync once,
  returns, and then refuses guest PC `0x800A7F90` because the former static dispatcher has no body.
- where: `docs/project-state.md`
- gap:
- notes: This records the discriminator, not a static implementation plan and not a claim that VSync
  policy caused the stop.

### runtime.retire-static — Remove the old execution path before replacement work

- status: re-verified
- deps: runtime.recorded-boundary
- evidence: The tracked emitter/bootstrap path and generated dispatcher are deleted; the ignored
  generated corpus and static build tree are absent; the source-policy test rejects their return.
- where: `CMakeLists.txt`, `tools/source_policy.py`, the `c12_source_policy` CTest
- gap:
- notes: CMake and the player launcher name the missing title field lifecycle/native presentation.
  There is no static compatibility product or interpreter-only player selector.

### runtime.lightrec — Pass the first dynamic discriminator

- status: crossed 2026-10-01
- deps: runtime.recorded-boundary, psxport per-`Core` dynarec executor
- evidence: `C12Runtime` publishes the recorded libetc VSync entry `0x800A1758`, its negative-query
  vs-count `0x800EEB98` (zeroed by VSync setup `0x800B0DF8`, incremented by the callback dispatcher
  `0x800B0E50`), the stock libcd command entry `0x800B4CA8`, and its guest work area
  `0x800EEED0..D4` through the title-owned `PlatformHlePlan`. Three measured title facts joined it:
  the libcd `CdReadyCallback` slot `0x800EEEBC` (written by `FUN_800AC158`, read by
  `FUN_800AF7EC`/`FUN_800AF4A8`) with `GuestInterrupt` delivery and completion code 1; the RAM
  arena `0x80105E40..0x801FFFF0` its loader allocates relocatable modules in; the 2D packet pool
  `0x800E3DE4..0x801B0F9C`; and that the guest's own VRAM is the picture. The pool is ONE window
  holding both parity ordering tables (`0x801AFF98`, `0x801B0F98`, 0x10000 apart, inside the guest's
  heap) with packets descending from them, measured by walking each parity table over 3,000 turns —
  neither an array of two equal halves nor two reallocating pools, so the framework gained the
  representation that says so. On the authenticated USA image the probe runs 2,000 turns with 1,038
  typed frame boundaries, 92.5 M guest instructions, zero fallback blocks and zero faults. The read
  pump at `0x800AFB70` crosses its retry cycle and stays crossed: `CdRead: retry...` and
  `CdRead: sector error` are both absent from a 2,000-turn run. `c12_port` is now a real product
  executable: `GuestFieldLoop` runs the guest to each VSync boundary and crosses the framework's ONE
  presentation fence per field, which is what the `FramePresenter::capture OVERFLOW ... > RQ_MAX`
  refusal was about — the probe never committed a fence, so every field's prims accumulated.
  1,420 presented fields, 1,326 VSync boundaries, 0 fallback, 0 faults, with the loopback control
  channel answering `shot`/`r`/`w32` against the live Core.
- where: `external/psxport` (`cd_ready_delivery.*`, `cdc_native.cpp`, `cd_override.cpp`,
  `guest_code_module.*`, `guest_packet_pool_windows.*`), `game/facts/title_facts.h`,
  `game/frame/guest_field_loop.*`, `game/entry/player_entry.cpp`
- CLOSED as a capture artefact, not a product defect: the "alternate-raster-line loss" was measured
  only through `present_shot`, which loses alternate rows at some sink sizes on this host (512x240
  ratio 0.000, 1280x720 ratio 0.500, 512x480 ratio 1.004 on the default Vulkan driver; 512x240 on
  llvmpipe ratio 1.047). Measuring RGB only is what exposes it — the missing rows carry alpha 255,
  the present pass's clear colour. The picture's own inputs are all measured whole: the composite
  texture is non-blank in 240/240 rows of BOTH interlaced field windows (the guest alternates
  `disp.y` 0 and 256, and only the even field had ever been checked), the present pass is 1:1
  (512x240 target, 320x240 viewport, source rect 320x240, no interlace term in either shader), and
  presented row N matches composite row N rather than 2N. Per-region and whole-canvas uploads give a
  byte-identical `s_present_img` (same SHA-256), so the copy extent never mattered and the
  whole-canvas arm is reverted. Residual, unfixed, tracked as psxport issue
  `0148-present-image-loses-alternate-rows-at-some-sink-sizes`.

- notes: Do not invoke an offline translator or generated guest corpus.

### runtime.module-load — Execute the code the title's own loader reads

- status: verified 2026-10-01
- deps: runtime.lightrec
- evidence: The title is not one image. `FUN_80059108` opens `RELOCS_GT_LVB`, allocates it in its
  own arena, relocates it with `FUN_800615F4`, and calls the module entry. Measured on the
  authenticated image: 8 sectors DMA3 from LBA 14261..14268 into `0x8011F9BC..0x801239BC`, entry
  dispatched at `0x8011FA64` (blob + 0xA8). The framework refused it — `ambiguous code-image
  identity` — because no active image claimed the heap. It now does: the title declares the arena,
  and a CD transfer landing inside it establishes the residency (`guest_code_module.*`), after which
  the module executes and the title reaches its per-field loop and its drawing submissions.
- where: `runtime/psx/guest_code_module.*`, `runtime/psx/mem.cpp` DMA3 landing, `GameRuntime::guestCodeModuleWindow`
- gap: The identity is established by the first landing and covers the whole arena, so keying a
  native override on a module's content identity still needs the load's boundaries measured.
- notes: The arena's upper bound is the executable's own stack base (`0x801FFFF0`), not the guest's
  declared heap end (`0x8011FFF4`): the measured module load runs past that end.

### runtime.gameplay — Reach representative interactive gameplay

- status: first mission reached 2026-10-09 (spawn street, player moves under pad); combat and mission exit unreached
- deps: runtime.lightrec
- evidence: NEW GAME → `GRISHAM'S LOG` → briefing cutscene (`X SKIP`) → first mission, master `0x800F1B40=3`; the character moves under `hold` and under `replays/first-mission/menu-to-mission-run-right-left.pad`; recordcheck 2,462 presents, 0 mismatched at 1x 4:3. No further CD, completion or input blocker was met past the briefing: the earlier 'stops at the briefing' reading was the cutscene waiting for `X`.
- where: future title-owned driver, native overrides, and bounded gameplay scenario
- gap: Reach and drive an interactive scenario with correct input, guest state, memory,
  interrupt/timing, relevant devices, audio, rendering, and declared frame-time evidence on every
  released host architecture.
- notes: Boot, first VSync, logos, menus, attract loops, and FMV are checkpoints only. An independent
  emulator or the interpreter in a separately built test target, including diagnostics, may diagnose a
  divergence but never enters gameplay.
- notes 2026-10-02: the menu is fully driven (Start→menu, UP-X→card screen, DOWN-X→options screen;
  evidence in `docs/project-state.md`), but NEW GAME stalls at master 6→2 with `0x800F1b44=5`.
- notes 2026-10-03 (issue `0004`): the module step is DONE and the attribution it rested on is wrong.
  `RELOCS/GT.LVB` is decompiled — its post-relocation 16 KiB at `0x8011F9BC..0x801239BC` is manifest
  image `c12_gt` and analyses to 14 functions — and it contains no store to `0x800F1B40..0x800F1B50`
  at all, nor does the arena or the EXE. The wait is in the EXE: `FUN_800582cc` is a 16-instruction
  `while (*(int *)(gp + 0xB48) != 0)` spin, and `PSXPORT_WWATCH_GPR` measures `r28 = 0x800EFC10`, so
  its word IS `0x800F0758`, which rises 0→5 by `f862` and is never decremented. `FUN_800582cc` was
  wrongly ruled out before (the "pending > 0x14 gate" is not in its body; only 3 of its 4 call sites
  have it).
- notes 2026-10-03 (issue `0004`, second pass) — THE QUEUE IS RESOLVED: `gp+0xB48` is the CD
  COMMAND-QUEUE DEPTH, not a response buffer. `FUN_800572c0` enqueues (`pending++`, ring `0x800F4020`);
  `FUN_800573e4` is the pump and issues a command ONLY when `state` (`gp+0xB68`) is 1 or 2;
  `FUN_800577ac` sets `state=0` as its FIRST instruction and calls the CD seam this title already
  declares, `kCdCommandAddress = 0x800B4CA8`; the ONLY `pending--` in the executable is in the ready
  callback `FUN_80057f7c`, reached solely from `FUN_800b4760`'s `(*DAT_800eeeb8)` poll gated on
  `FUN_800b41fc() & 2`. Measured at the stall: `pending=5 read=0 write=5 state=0 status=2 open=1`.
  With `pending!=0` and `state==0` the pump issues nothing, so the callback never runs and the spin
  cannot exit — `state==0` is written only by `FUN_800577ac`'s first instruction, so the last
  dispatcher entry cleared it and the completion that should set it back to 1 or 2 never arrived.
  Next: does `FUN_800b41fc()` report bits 2/4 after psxport's instant `0x800B4CA8`. That one answer
  picks the fix, which is at the CD completion seam — never at `FUN_800582cc` or `FUN_800573e4`,
  whose forcing would mask the missing completion.

### runtime.cd-command-completion — Deliver the completion the replaced libcd body delivered

- status: crossed 2026-10-04
- deps: runtime.module-load
- evidence: The word `FUN_800582CC` waits on is this title's CD COMMAND-QUEUE DEPTH
  (`gp+0xB48` = `0x800F0758`, `gp = 0x800EFC10` measured at the store), and the guest's queue
  dispatcher `FUN_800577AC` clears its state word `0x800F0778` (`gp+0xB68`) as its FIRST
  instruction, so only a completion can move it back to 1 or 2. Measured through the product's
  control channel at the settled title screen, before any input: the queue at `0x800F4020` held
  `[0x0D, 0x02(arg 0x20000000), 0x05(arg 2), 0x0C]` with depth 4, read 0, write 4, state 0, frozen
  over 20k presented fields; NEW GAME appended `0x06` and set `0x800F1B44=5` without changing any
  of it, and master went 6->2 instead of reaching the scene.
  On the full-console reference (same USA image, SCPH-1001 firmware, no input) the same four
  commands are RETIRED: over 7,200 fields the guest's own libcd response poll `FUN_800b41fc` is
  entered 1,706 times and the guest's own queue-completion callback `FUN_80057f7c` 13 times, and the
  queue settles at depth 0 / read 3 / write 4 / state 2 — the values the port now reaches.
  The cause is a seam boundary, not a missing device: `FUN_800abd98` (`CdReadySync`, 0x800ABD98)
  issues through `FUN_800b4ca8` (`CdlSync`, 0x800B4CA8, this title's declared `cdCommandAddress`)
  and expects that body's own poll to read the controller and call the registered callbacks;
  `CdSync` (`FUN_800ac008`, 0x800AC008) instead runs `FUN_800b4760` as separate guest code, which is
  why that path always completed. The completion therefore has to be delivered at the guest's own
  command-status slot `0x800EEEB8`, which `FUN_800abd98` saves and zeroes before issuing and
  restores before the real command — so the owner re-reads the slot's CURRENT value.
  After the owner: master 6->3, the guest's `GRISHAM'S LOG` page, then a rendered mission briefing,
  and read 9 / write 10 once the briefing is continued.
- where: `game/cd/cd_command_completion.{h,cpp}`, `game/facts/title_facts.h`
  (`kCdReadySyncAddress`, `kCdCommandStatusCallbackSlotAddress`),
  `game/runtime/c12_runtime.cpp` (`registerOverrides`)
- gap: the owner reproduces libcd's completion for the command-status response only. The
  command-acknowledge and error response types are still not delivered to the guest's own poll, so a
  command that fails reports failure only through the wrapper's own return value.
- notes: two earlier attributions are withdrawn. `PSXPORT_WWATCH` prints `Core::pc`, which for
  translated code is the BLOCK START, so its `pc=800B47E8` line never identified a storing
  instruction — a store watchpoint's PC is not an instruction-level attribution. And the
  "no store to `0x800F1B40..0x800F1B50`" scan was run against the game's `gp` only; libcd's own code
  uses `gp = 0x800F0000` (its `-0x1148(gp)` resolves to `0x800EEEB8`), so a single-gp register scan
  misses that half of the image. One Ghidra reference count is analyzer scope, not execution.

## World projection, culling and distance (main executable, image `c12`)

### camera.projection — GTE projection setup

- status: verified 2026-10-10
- deps: runtime.gameplay
- evidence: Gameplay is a 512x240 hires picture with OFX 256, OFY 120 and H 0x17C. The camera struct
  (pointer in scratchpad `0x1F800060`) holds OFX<<16 at +0x68, OFY<<16 at +0x6A, H at +0xC4 and the fog
  near/far at +0xD0/+0xD4 (0x1800/0x1FFE). `FUN_8009C830` and `FUN_8009CA4C` load it into the GTE;
  `FUN_800AF364(near, far, H)` derives DQA/DQB through `FUN_800B8380`/`FUN_800B8390`. The per-zone H
  word `0x800F920C` is interpolated into the camera by `FUN_80065890`.
- where: `game/facts/title_facts.h` (`kZoneProjectionDistanceAddress`)
- notes: the title leaves OFX, OFY and H at retail values at 16:9; the canvas gains margin columns and
  the cull below is widened. Projected coordinates are therefore identical to 4:3, so no gameplay word
  moves.

### world.frustum — View frustum corner builder

- status: verified 2026-10-10
- deps: camera.projection
- evidence: `FUN_800660D8` builds four frustum corners at `0x800F9378` (stride 8, s16 x, y, z). Each
  corner is (+-0xA0, +-0x78, H), normalized to length 4096 by `FUN_800A1F10`, then scaled by the zone
  reach (`0x800F9220`, 0x2000 in 16.16) divided by the normalized z. `FUN_800684C0` rasterizes the
  corners into the visible cells of the 64x64, 1024-unit grid (`FUN_80066234` is dead code, see world.cell-collector). The 0xA0 half width is a 320-wide
  window, so the corners are narrower than the 512-wide picture. Native owner
  `c12::rebuildViewFrustumCorners` reproduces the guest at 4:3 (override differential: match, no
  mismatch) and scales the half width by presentation/native width at 16:9.
- where: `game/render/view_frustum.{h,cpp}`
- gap: the corner table is rebuilt only when the zone changes, so a window resize between zone changes
  keeps the previous plan's corners.

### world.polygon-pass — Terrain polygon pass and its screen reject

- status: verified 2026-10-10
- deps: world.frustum
- evidence: `FUN_800657B4` calls `FUN_8006769C`, the terrain polygon pass. Per polygon it projects with
  RTPT/RTPS, rejects when all x < 0, all y < 0, none has x < 0x200 or none has y < 0x100, rejects
  average z < 4 and, past the zone far (`0x800F91E4+0x54`, the fog far, 0x1FFE), clamps the OT slot to
  `otLength-3`, shades from the `TerrainFog` tables at `0x800F9E00`, and tessellates through the tables at
  `0x800D8728..0x800D8954`. Widening the frustum corners alone did not change the picture: this fixed
  512x256 window reject is the binding cull. Patching its four `slti 0x200` immediates filled the right
  margin, which located it. `c12::drawWorldMeshPass` is a native port of the whole function; at 4:3 the
  override differential reports match with no mismatch and the 4:3 picture is unchanged, at 16:9 the
  reject extends by `widenedWindowMargin` columns per side and the margin voids are filled.
- where: `game/render/world_mesh_pass.{h,cpp}`, `game/facts/title_facts.h`
- gap: `FUN_800684C0` (below) and object/actor culling were not widened, so voids beyond the terrain margin
  may remain.

### world.cell-collector — Visible cell footprint (`Capture`)

- status: verified 2026-10-10
- deps: world.frustum
- evidence: `FUN_8006572C` clears the visible flag (cell record byte 6, bit 0) of every cell in the
  zero-terminated pointer list at `0x800F9AA4`, then calls `FUN_800684C0(cam, 0x96, 0x800F9398)`, which is
  the live collector: it rotates the four frustum corners (`0x800F9378`), clips them to the ground plane
  (`0x800F0830/34`, limit `0x800F0838`), rasterizes the footprint into the row tables
  (`0x800F9E34` first column, `0x800F9EB4` last column, 64 s16 rows each, via `FUN_80069500`), then walks the
  rows and keeps every cell whose box survives a GTE RTPT/RTPS screen test, as a 12-byte entry
  (count, skips-tail, top y, polygon list) in the 150-entry buffer ending at `0x800F9AA4`, a list pointer
  and the visible flag. It returns the count, stored at `0x800F9274`. `FUN_80066234` and its tables at
  `0x800F9D00` are an older collector nothing calls.
  Native owner `c12::collectVisibleCells` reproduces it: the override differential over the whole first-mission
  replay at every call reports no mismatch (4:3, increase 0).
- survey of the readers and writers (`decomp_pipeline.py --refs`): cell buffer `0x800F9398`: written by
  `FUN_800684C0`, read by `FUN_8006769C` (terrain pass, native); pointer list `0x800F9AA4`: written by
  `FUN_800684C0`, `FUN_80066234` (dead) and `FUN_800655E8` (level init, terminator), read by `FUN_8006572C`;
  count `0x800F9274`: written by `FUN_8006572C`, read by `FUN_800657B4` and `FUN_8006769C`; row tables
  `0x800F9E34/0x800F9EB4`: written by `FUN_80068334` (init), `FUN_800684C0` and `FUN_80069500`, read by
  `FUN_800684C0`; visited-polygon list (header `+0x88`, capacity header `+0x08`): written by `FUN_800655E8`,
  used only inside `FUN_8006769C`; fog words `0x800F9234..0x800F9240` and bias `0x800F1588`: read by
  `FUN_800675B8` (called from three sites on zone load); `TerrainFog` descriptor `0x800F9E00..`: written by
  `FUN_800674C8`, read by `FUN_8006769C`.
- where: `game/render/cell_collector.{h,cpp}`, `game/render/cell_output.{h,cpp}`, `game/facts/title_facts.h`
- gap: the every-call differential compares the native pass with the retail body, so it is exact only with
  `PSXPORT_C12_WIDESCREEN=0` and the draw distance at 0: 1167 of 1167 calls of the first-mission replay match.
  With widescreen on, the first mismatch is call 92: a polygon wholly right of column 512 that the widened reject
  (`widenedWindowMargin`) keeps and the retail body drops. Above 0 percent the far clip, fog and host cell list
  differ by design. The differential cannot compare either enhanced path.

### world.distance — Far clip, fog and LOD distance

- status: verified 2026-10-10 (route B)
- deps: world.polygon-pass, world.cell-collector
- evidence: the zone record (`0x800D86F0 + idx*0x18`, loaded by `FUN_80065F68`, interpolated per frame by
  `FUN_80065890` into `0x800F9204..`) holds the visible far (`0x800F920E`, word `0x800F9220`) and the fog
  near/far (`0x800F9214/16`, words `0x800F9234/38`). The fog far is also the polygon pass far clip. The
  fog is a per-level table built by `FUN_800675B8` and allocated by `FUN_800674C8` with
  `(header+0x72 >> 6)` entries plus 0x40 padding entries (pool name `TerrainFog`), indexed by `z >> 6`.
  There is no separate LOD distance for the terrain; subdivision follows polygon area.
  The three fixed structures: the cell buffer (150 entries, full against the pointer list), the
  `TerrainFog` allocation and the visited-polygon list are all reached only by the collector and the terrain
  pass, both native now. The 0x40-row footprint tables are clamped to the grid (64 rows) by `FUN_80069500` and
  stay guest-owned, so no larger table is needed.
- route: B. Only a native collector can emit more than 150 cells, because the guest hardcodes the list
  address and cap, so `c12::collectVisibleCells` runs a grown pass first (frustum corners, ground limit and
  far scaled by `DrawDistance`; cells whose box does not fit the GTE's signed 16-bit input are kept without a
  screen test) into a host list, then the retail pass, which leaves the guest buffers, flags, row tables and
  GTE state exactly retail. The terrain pass draws the host list, nearest first so a full packet pool drops
  the farthest terrain, scales its far clip and polygon cap, and draws from fade tables stretched by the same
  factor (`FogTables`). Route A (relocating the structures) was rejected: the guest addresses are immediates
  in the collector and the terrain pass.
- where: `game/render/draw_distance.h`, `game/render/cell_collector.cpp`, `game/render/fog_table.cpp`,
  `game/render/world_mesh_pass.cpp`
- gap: past about 300 percent the first mission's street shows no more geometry (the level ends); depth is a
  16-bit z, so very large increases saturate; the guest packet pool is fixed, so dense scenes drop the farthest
  cells first; object and actor culling is not scaled.
