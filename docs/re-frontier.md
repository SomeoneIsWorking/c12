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
- evidence: `C12Runtime` now publishes the recorded libetc VSync entry `0x800A1758` through a
  title-owned `PlatformHlePlan`; `c12_runtime_services` proves the shared typed frame-boundary
  registration and exact admission window.
- where: `external/psxport`, `psxport.pin`, `game/runtime/c12_platform_facts.*`, and the startup probe
- gap: Execute nonzero blocks from authenticated `SCUS_946.66` through psxport's pinned Lightrec,
  cross first VSync through an explicit bounded exit/resume, and continue beyond `0x800A7F90`. Audit
  the gameplay link and selector surfaces to exclude interpreter-only execution; count the shared
  backend's bounded compilation/fetch fallback by reason and executed instructions/blocks.
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
