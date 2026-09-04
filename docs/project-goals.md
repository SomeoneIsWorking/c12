# Project goals

## G001 — A faithful native/dynarec C-12 port

C-12 should run from the user's authenticated USA game image as one native/dynarec product. C-12
owns title identity, title policy, and any deliberately native overrides; psxport dynamically
translates every remaining guest instruction through its maintained, pinned Lightrec integration.

Success conditions:

- The zero-argument launcher authenticates the user-supplied game, builds, and launches without an
  offline guest translator, generated guest corpus, or maintainer-only RE dependency.
- The gameplay target links no interpreter, exposes no interpreter selector, and contains no
  interpreter fallback. An interpreter may exist only in a separately built test target, including
  diagnostics.
- Native overrides, when added, are keyed by complete runtime image identity and address and can call
  the original authenticated guest body through psxport's scoped Lightrec original-call boundary.
- The offline translator, generated corpus, static dispatcher, and static-only configuration are
  absent before dynarec implementation begins and cannot be restored as a bridge.
- The first dynamic discriminator executes `SCUS_946.66` through its first VSync return and continues
  beyond guest PC `0x800A7F90`, the first former static-dispatch miss.
- The hybrid product reaches representative interactive gameplay with correct input, timing,
  interrupts, devices, audio, rendering, and measured frame time.
- An independent emulator, binary analysis, or a separately built test target, including diagnostics,
  remains available for
  first-divergence diagnosis without retaining the static product as an oracle.

Constraints and non-goals: boot or first VSync alone is not gameplay conformance; no compatibility
static product or interpreter-backed gameplay mode ships; copyrighted game bytes remain untracked.

Contributing state items: S001-S006.
