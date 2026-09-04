# C-12: Final Resistance

C-12 is being migrated to a native/Lightrec hybrid over the authenticated USA PlayStation release.
The offline translator, generated source corpus, static dispatcher, prior build tree, and generated
gameplay executable have been deleted before replacement work. They are not retained as a bridge or
oracle.

The gameplay target is intentionally unavailable until psxport supplies its per-`Core` Lightrec
executor with typed VSync, exception, fault, and termination exits. `./run.sh` currently validates
the configured user disc and then refuses by naming that missing boundary; it never emits or builds
guest source.

Configuration is resolved once in `tools/config.py` from `--disc`, `PSXPORT_DISC`, `.env`, or one
root-level `.chd`, in that order. `bootstrap.py` is a slim entry point and all non-trivial launcher
logic is modular Python under `tools/`.

Run the source-boundary checks without invoking the removed product:

```sh
uv run --frozen python tests/test_source_policy.py
CC=clang CXX=clang++ cmake -S . -B build/maintainer
ctest --test-dir build/maintainer --output-on-failure
```

The next product milestone integrates the shared executor, resumes after the first typed VSync exit,
and proves translated execution beyond `0x800A7F90`. Boot or first VSync alone is not gameplay
conformance.
