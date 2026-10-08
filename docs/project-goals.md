# Project goals

## G001 — A faithful native/dynarec C-12 port

C-12 should run from the user's authenticated USA game image as one native/dynarec product. C-12
owns title identity, title policy, and any deliberately native overrides; psxport dynamically
translates every remaining guest instruction through its maintained, pinned Lightrec integration.

Success conditions:

- The zero-argument launcher authenticates the user-supplied game, builds, and launches without an
  offline guest translator, generated guest corpus, or maintainer-only RE dependency.
- The gameplay target offers every executable block to the JIT first and exposes no interpreter
  selector. Only the shared backend's bounded, reason-coded compilation/fetch refusal fallback is
  permitted; translated and fallback instruction/block counts remain measurable. Interpreter-only
  execution belongs to separately built diagnostics.
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
static product or interpreter-first gameplay mode ships; copyrighted game bytes remain untracked.

Contributing state items: S001-S006.

## G002 — Native presentation and responsive play

The port should render an authentic C-12 scene natively while adding true widescreen and smooth
60 fps presentation without changing the authored gameplay rate.

Success conditions:

- Native picture construction derives geometry, materials, camera, and authored draw order from
  recovered title state. A guest-picture path does not substitute for the native product.
- Widescreen projects the same source geometry into a wider viewport and resolves proven culling
  boundaries; it never stretches a completed frame or selects coverage from adjacent frames.
- Temporal interpolation produces 60 fps presentation from explicitly matched source geometry,
  preserving authored simulation timing and rejecting discontinuities.
- Loading work is asynchronous. Loading-only screens are absent, while logos and retained authored
  sequences support complete Start/Cross cancellation transitions.
- Physical controls, rendering, timing, audio, and progression pass representative interactive
  gameplay observations against an independent emulator/oracle.

Constraints and non-goals: faster simulation, fabricated cancellation state, and image-space
interpolation are not implementations of these outcomes.

Contributing state items: S005, S008-S011, S014, S021.

## G003 — Accessible, asset-free releases

Players should launch an installable C-12 application on each intended host, select their own game
files without a terminal, and retain saves and settings in their platform's application data.

Success conditions:

- Publish asset-free Windows, macOS `.app`, Linux AppImage, Android APK, and WebAssembly/GitHub Pages
  releases with a qualified native/dynarec execution path on every claimed host architecture.
- First-run setup uses a native file picker, authenticates supported game inputs (including one
  bounded nested ZIP), and preserves the previous valid installation on every rejected selection.
- Persist saves/settings in OS user data; document reset/reselection and supply licensed assets only.
- Android uses the shared Lucent and `android-port` owners. Authored SVG touch controls drive the
  same action policy as physical input and handle multitouch, cancellation, safe areas, and a
  connected controller. Qualify rendering/audio/performance on named devices.
- Hosted CI exercises redistributable synthetic runtime and package boundaries on each applicable
  platform. Real-title gameplay/oracle qualification remains local and never uploads game bytes.

Constraints and non-goals: fallback-dominated runs, cross-compilation alone, and boot-only evidence
do not qualify platform support or a release.

Contributing state items: S007, S012-S021.
