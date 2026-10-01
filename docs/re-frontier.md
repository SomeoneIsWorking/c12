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
  `guest_code_module.*`, `guest_packet_pool_windows.*`), `game/runtime/c12_platform_facts.*`,
  `game/field/guest_field_loop.*`, `game/app/player_entry.cpp`
- gap: The packet pool is declared and the product presents, so the next frontier is the PICTURE's
  fidelity and what the title does after Start. At field 1400 the presented frame is C-12's title
  screen ("C-12 FINAL RESISTANCE" / "PRESS START BUTTON") at 40.68% non-black, drawn entirely by the
  guest; the boot loading screen at field 50 renders in full colour. What is wrong with the title
  screen is measured and located to a framework seam: every second raster line is lost between guest
  VRAM and the presented frame (CPU capture 0 of 240 rows dark, presented 149 of 240, alternating),
  while the upload itself carries all 240 rows (`upload_vram 480 KiB`). That is the next step, in the
  composite's decode/encode passes, not in the title. Then drive Start through the pad and reach the
  logo sequence. Audit the gameplay link and selector surfaces to exclude interpreter-only execution;
  count the shared backend's bounded compilation/fetch fallback by reason and executed
  instructions/blocks.
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

- status: todo
- deps: runtime.lightrec
- evidence:
- where: future title-owned driver, native overrides, and bounded gameplay scenario
- gap: Reach and drive an interactive scenario with correct input, guest state, memory,
  interrupt/timing, relevant devices, audio, rendering, and declared frame-time evidence on every
  released host architecture.
- notes: Boot, first VSync, logos, menus, attract loops, and FMV are checkpoints only. An independent
  emulator or the interpreter in a separately built test target, including diagnostics, may diagnose a
  divergence but never enters gameplay.
