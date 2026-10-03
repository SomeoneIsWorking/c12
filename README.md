# C-12: Final Resistance

C-12 is being migrated to a native/Lightrec hybrid over the authenticated USA PlayStation release.
The offline translator, generated source corpus, static dispatcher, prior build tree, and generated
gameplay executable have been deleted before replacement work. They are not retained as a bridge or
oracle.

The player target `c12_port` runs the authenticated image: it composes the machine, owns one
display field at a time, and presents the guest's own picture. `./run.sh` validates the configured
user disc path and launches it. The separate `c12_boot_probe` maintainer tool boots the same machine
and drives bounded Lightrec turns without presenting a frame or opening an audio stream.

Configuration is resolved once in `tools/config.py` from `--disc`, `PSXPORT_DISC`, `.env`, or one
root-level `.chd`, in that order. `bootstrap.py` is a slim entry point and all non-trivial launcher
logic is modular Python under `tools/`.

The title identity check accepts a directory extracted by psxport's `discdump`:

```sh
uv run --frozen python -m tools.title_identity scratch/c12-identity
```

It requires `SYSTEM.CNF` to select `SCUS_946.66` exactly once and checks all 921,600 bytes of
that executable against the supported USA revision's SHA-256. `title.json` is the single revision
authority consumed by Python and the C++ admission code. The native probe hashes its own bounded
byte buffer with Lucent, then gives those same bytes to psxport's validated executable mapper.

Run the independent verification command with a resolved psxport checkout and its documented native
dependencies. This uses Clang, Ninja, and the frozen Python interpreter:

```sh
CC=clang CXX=clang++ uv run --frozen python -m tools.verify
```

Linux CI resolves `external/psxport` through the framework's own `tools/psxport_fetch.py` — there is no
per-port framework pin; a bare clone or CI gets psxport `main` — before invoking that framework's shared
setup action and the same verifier. Its synthetic admission/style/execution checks require no game files. Dependency checkouts
and compiler outputs remain under `build/`; hosted success is distinct from real-title qualification.

The boot observation takes an extracted executable, cycles per turn, and maximum turns:

```sh
build/maintainer/c12_boot_probe scratch/c12-identity/SCUS_946.66 100000 3
```

It reports every typed exit plus translated/guest/fallback counters. At a typed VSync boundary it
observes the guest return address and resumes another bounded Lightrec turn. The probe steps the
host display-field clock but does not present frames or open an audio stream. On the real image the
startup CD reads complete through the guest's own registered ready callback, the
title loads and executes its disc-resident module (`RELOCS/GT.LVB`) out of its RAM arena, and the
guest runs its per-field loop and submits drawing primitives.
First boot execution alone is not gameplay conformance. Intended enhancements and platform releases
are listed separately in [project state](docs/project-state.md).

`c12_source_policy` rejects the retired offline translator, generated corpus, static dispatcher, and
their source markers; `c12_cpp_style` is the shared formatter check.
