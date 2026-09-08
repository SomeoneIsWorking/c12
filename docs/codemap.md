# Codemap

This map records ownership and placement only. Intent lives in `docs/project-goals.md`, factual state
in `docs/project-state.md`, and evidence order in `docs/re-frontier.md`.

| Responsibility | Current owner / location | New responsibility goes |
| --- | --- | --- |
| Player launch and authenticated image provisioning | `run.sh`, `bootstrap.py`, `tools/config.py`, `tools/launcher.py`, `pyproject.toml`, `uv.lock` | `run.sh` remains a slim shim; the Python launcher resolves and validates the user image, then refuses at the named missing executor boundary until the gameplay product exists |
| Exact USA executable identity | `tools/title_identity.py`, `tests/test_title_identity.py` | C-12 owns the accepted boot name, complete executable SHA-256, and revision size; psxport owns CHD extraction and the eventual bounded executable mapper |
| Framework version and PSX execution engine | `psxport.pin`, `external/psxport` | psxport owns the maintained per-`Core` Lightrec integration, CPU synchronization, bounded exits, executable-memory invalidation, and runtime diagnostics; C-12 pins a revision only after its own gates pass |
| Gameplay composition and title policy | `game/` | C-12 owns its authenticated title identity, lifecycle composition, future frame driver, and deliberately native overrides; every other guest path enters Lightrec |
| Test-only interpreter | Separately built psxport test target, including diagnostics | psxport test ownership only; never linked into, selected by, or entered from C-12 gameplay |
| Native override and original-call policy | Future cohesive title-owned modules under `game/`; psxport runtime API | Overrides are keyed by runtime image identity plus guest address; a native owner calls original behavior only through psxport's scoped Lightrec original-call operation |
| First dynamic discriminator | Title runner plus psxport executor | Report first VSync exit/resume and subsequent execution beyond `0x800A7F90`, with nonzero Lightrec blocks and no interpreter in gameplay |
| Representative gameplay evidence | Future title-owned bounded scenario and diagnostics | Prove meaningful player control, state/memory, interrupt/timing, devices, audio, rendering, and frame time |
| Static-path exclusion | `tools/source_policy.py`, `tests/test_source_policy.py` | Reject the deleted offline translator, generated corpus, static dispatcher, and compatibility markers before any dynarec implementation |
| Missing gameplay integration | `CMakeLists.txt`, `tools/launcher.py` | Fail by naming the unconnected Lightrec backend and missing title typed-exit runner until both sides of that boundary are integrated |
| Project facts and evidence order | `docs/project-state.md`, `docs/re-frontier.md`, `docs/codemap.md` | State changes go to project state, ordered RE evidence to the frontier, and ownership changes to this map |
