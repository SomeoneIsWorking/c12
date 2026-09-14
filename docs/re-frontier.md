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
- where: `CMakeLists.txt`, `tools/source_policy.py`, `tests/test_source_policy.py`
- gap:
- notes: CMake and the player launcher name the missing title field lifecycle/native presentation.
  There is no static compatibility product or interpreter-only player selector.

### runtime.lightrec — Pass the first dynamic discriminator

- status: todo
- deps: runtime.recorded-boundary, psxport per-`Core` dynarec executor
- evidence: `C12Runtime` publishes the recorded libetc VSync entry `0x800A1758`, its negative-query
  vs-count `0x800EEB98` (zeroed by VSync setup `0x800B0DF8`, incremented by the callback dispatcher
  `0x800B0E50`), the stock libcd command entry `0x800B4CA8`, and its guest work area
  `0x800EEED0..D4` through the title-owned `PlatformHlePlan`. The diagnostic now steps the host
  display-field clock through the shared `gpu_pace_frame` owner per executor turn: on the
  authenticated USA image 600 turns raised 597 guest vs-counts, the previously repeating
  `VSync(-1)`/stock-command wait is crossed, and the guest reads back the published
  `CdLastPos` bytes through its own accessor. Fallback and faults remain zero.
- where: `external/psxport`, `psxport.pin`, `game/runtime/c12_platform_facts.*`, and the startup probe
- gap: Deliver the startup CD read completion to the guest's registered ready callback so the title
  read pump at `0x800AFB70` crosses its retry cycle (issue 0003); then observe the first positive
  VSync `FrameBoundary` exit under field stepping and recover the rest of the title startup and
  field lifecycle toward a native frame driver, then execute beyond `0x800A7F90` through Lightrec
  without letting guest VSync own product time. Audit the gameplay
  link and selector surfaces to exclude interpreter-only execution; count the shared backend's
  bounded compilation/fetch fallback by reason and executed instructions/blocks.
- notes: Do not invoke an offline translator or generated guest corpus.

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
