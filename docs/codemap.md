# Codemap

This map records ownership and placement only. Intent lives in `docs/project-goals.md`, factual state
in `docs/project-state.md`, and evidence order in `docs/re-frontier.md`.

| Responsibility | Current owner / location | New responsibility goes |
| --- | --- | --- |
| Player launch and authenticated image provisioning | `run.sh`, `bootstrap.py`, `pyproject.toml`, `uv.lock` | `run.sh` remains a slim shim; `bootstrap.py` resolves and validates the user image, resolves the pinned framework, builds, and launches without offline guest translation |
| Framework version and PSX execution engine | `psxport.pin`, `external/psxport` | psxport owns the maintained per-`Core` Lightrec integration, CPU synchronization, bounded exits, executable-memory invalidation, and runtime diagnostics; C-12 pins a revision only after its own gates pass |
| Gameplay composition and title policy | `game/` | C-12 owns its authenticated title identity, lifecycle composition, future frame driver, and deliberately native overrides; every other guest path enters Lightrec |
| Test-only interpreter | Separately built psxport test target, including diagnostics | psxport test ownership only; never linked into, selected by, or entered from C-12 gameplay |
| Native override and original-call policy | Future cohesive title-owned modules under `game/`; psxport runtime API | Overrides are keyed by runtime image identity plus guest address; a native owner calls original behavior only through psxport's scoped Lightrec original-call operation |
| First dynamic discriminator | Title runner plus psxport executor | Report first VSync exit/resume and subsequent execution beyond `0x800A7F90`, with nonzero Lightrec blocks and no interpreter in gameplay |
| Representative gameplay evidence | Future title-owned bounded scenario and diagnostics | Prove meaningful player control, state/memory, interrupt/timing, devices, audio, rendering, and frame time before deleting the static path |
| Static-path deletion gate | Existing build, provisioning, launcher, and static-only surfaces | Delete the complete offline translation path only after representative gameplay passes; leave no compatibility mode or tombstone |
| Retained generated guest corpus | `generated/` | Retirement target only: do not regenerate, build, run, select, or use it as an oracle; delete the directory after representative native/dynarec gameplay passes |
| Project facts and evidence order | `docs/project-state.md`, `docs/re-frontier.md`, `docs/codemap.md` | State changes go to project state, ordered RE evidence to the frontier, and ownership changes to this map |
