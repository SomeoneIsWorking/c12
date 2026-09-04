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

### runtime.lightrec — Pass the first dynamic discriminator

- status: todo
- deps: runtime.recorded-boundary, psxport per-`Core` dynarec executor
- evidence:
- where: `external/psxport`, `psxport.pin`, title composition under `game/`
- gap: Execute nonzero blocks from authenticated `SCUS_946.66` through psxport's pinned Lightrec,
  cross first VSync through an explicit bounded exit/resume, and continue beyond `0x800A7F90`. Audit
  the gameplay link, selector, and fallback surfaces to prove that the interpreter in the separately
  built test target, including diagnostics, is absent and unreachable.
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

### runtime.retire-static — Delete the old execution path

- status: todo
- deps: runtime.gameplay
- evidence:
- where: build, provisioning, launcher, and static-only artifacts
- gap: After representative gameplay passes, delete the offline translator, generated corpus, static
  dispatcher, seed-only metadata, and static-only checks together, then prove a fresh checkout builds
  and launches from the authenticated user image without them.
- notes: The retained files are not built or run while awaiting this gate. Removal leaves no
  compatibility mode, fallback, selector, or tombstone.
