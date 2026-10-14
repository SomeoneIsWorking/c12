# C-12: Final Resistance

C-12 is being migrated to a native/Lightrec hybrid over the authenticated USA PlayStation release.
The offline translator, generated source corpus, static dispatcher, prior build tree, and generated
gameplay executable have been deleted before replacement work. They are not retained as a bridge or
oracle.

The product `c12_port` is psxport's framework `main` over C-12's one-title catalog
(`game/entry/c12_catalog.cpp`): `ProductHost` authenticates the executable, `TitleSession` boots it,
and `C12FrameDriver` owns one display field per step and presents the guest's own picture.
`./run.sh` validates the configured user disc path and launches it with the extracted executable;
with no argument the product provisions `scratch/bin/c12/SCUS_946.66` from the disc itself.

Configuration is resolved once in `tools/config.py` from `--disc`, `PSXPORT_DISC`, `.env`, or one
root-level `.chd`, in that order. `bootstrap.py` is a slim entry point and all non-trivial launcher
logic is modular Python under `tools/`.

The title identity check accepts a directory extracted by psxport's `discdump`:

```sh
uv run --frozen python -m tools.title_identity scratch/c12-identity
```

It requires `SYSTEM.CNF` to select `SCUS_946.66` exactly once and checks all 921,600 bytes of
that executable against the supported USA revision's SHA-256. `title.json` is the single revision
authority consumed by Python and by the catalog's `TitleIdentity`, which the framework checks (size,
header words, SHA-256) before it maps the bytes.

Run the independent verification command with a resolved psxport checkout and its documented native
dependencies. This uses Clang, Ninja, and the frozen Python interpreter:

```sh
CC=clang CXX=clang++ uv run --frozen python -m tools.verify
```

Linux CI resolves `external/psxport` through the framework's own `tools/psxport_fetch.py` — there is no
per-port framework pin; a bare clone or CI gets psxport `main` — before invoking that framework's shared
setup action and the same verifier. Its synthetic admission/style/execution checks require no game files. Dependency checkouts
and compiler outputs remain under `build/`; hosted success is distinct from real-title qualification.

A bounded headless boot runs the built product with a step cap:

```sh
PSXPORT_NATIVE_FRAMES=600 build/maintainer/c12_port
```

The run-end report gives the executor counters, present intervals, audio queue and the title's
field and VSync-boundary counts. On the real image the startup CD reads complete through the
guest's own registered ready callback, the title loads and executes its disc-resident module
(`RELOCS/GT.LVB`) out of its RAM arena, and the guest runs its per-field loop and submits drawing
primitives. `c12_frame_driver` proves the field step over synthetic guests with no game files.
First boot execution alone is not gameplay conformance. Intended enhancements and platform releases
are listed separately in [project state](docs/project-state.md).

`c12_cpp_style` is the shared formatter, clang-tidy and ownership-policy check.
