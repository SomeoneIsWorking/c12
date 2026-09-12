# C-12: Final Resistance

C-12 is being migrated to a native/Lightrec hybrid over the authenticated USA PlayStation release.
The offline translator, generated source corpus, static dispatcher, prior build tree, and generated
gameplay executable have been deleted before replacement work. They are not retained as a bridge or
oracle.

The player target is unavailable until C-12 owns its field lifecycle and native presentation.
`./run.sh` validates the configured user disc path and then names that boundary. The separate
`c12_boot_probe` diagnostic authenticates and maps the original executable, then drives bounded
Lightrec turns without a display scheduler or a player picture.

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

Linux CI fetches `psxport.pin` before invoking that framework's shared setup action and the same
verifier. Its synthetic admission/style/execution checks require no game files. Dependency checkouts
and compiler outputs remain under `build/`; hosted success is distinct from real-title qualification.

The boot observation takes an extracted executable, cycles per turn, and maximum turns:

```sh
build/maintainer/c12_boot_probe scratch/c12-identity/SCUS_946.66 100000 3
```

It reports every typed exit plus translated/guest/fallback counters. At a typed VSync boundary it
observes the guest return address and resumes another bounded Lightrec turn. The probe does not
advance display fields, present frames, or open an audio stream. Its current real-image route reaches
VSync but polls it repeatedly; a native field lifecycle is still needed to reach `0x800A7F90`.
First boot execution alone is not gameplay conformance. Intended enhancements and platform releases
are listed separately in [project state](docs/project-state.md).
