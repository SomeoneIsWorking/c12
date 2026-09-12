# Codemap

This map records ownership and placement only. Intent lives in `docs/project-goals.md`, factual state
in `docs/project-state.md`, and evidence order in `docs/re-frontier.md`.

| Responsibility | Current owner / location | New responsibility goes |
| --- | --- | --- |
| Player launch and authenticated image provisioning | `run.sh`, `bootstrap.py`, `tools/config.py`, `tools/launcher.py`, `pyproject.toml`, `uv.lock` | `run.sh` remains a slim shim; the Python launcher resolves and validates the user image, then refuses at the named missing executor boundary until the gameplay product exists |
| Exact USA executable identity | `title.json`, `tools/title_identity.py`, `game/image/authenticated_image.*`, `game/image/title_identity.h.in` | C-12 owns the accepted boot name, complete executable SHA-256, and revision size; CMake materializes non-executable identity constants from that authority, while psxport owns CHD extraction and bounded executable mapping |
| Framework version and PSX execution engine | `psxport.pin`, `external/psxport` | psxport owns the maintained per-`Core` Lightrec integration, CPU synchronization, bounded exits, executable-memory invalidation, and runtime diagnostics; C-12 pins a revision only after its own gates pass |
| Direct runtime identity and title policy | `game/runtime/c12_runtime.*`, `game/runtime/c12_platform_facts.*` | C-12 supplies immutable executable and measured hardware-service facts to `GameRuntime`; future cohesive title owners implement native lifecycle and presentation |
| Bounded startup observation | `game/app/boot_probe.cpp`, `game/runtime/c12_platform_facts.*` | Authenticate the title bytes, map them through psxport, install the measured typed VSync boundary, and observe `executeUntilExit` continuations without inventing a native frame driver |
| Interpreter-only diagnostics and bounded fallback | Separate psxport oracle targets; psxport Lightrec fallback owner | Interpreter-only selection remains outside gameplay; backend compilation/fetch refusal policy and per-reason accounting belong to psxport |
| Native override and original-call policy | Future cohesive title-owned modules under `game/`; psxport runtime API | Overrides are keyed by runtime image identity plus guest address; a native owner calls original behavior only through psxport's scoped Lightrec original-call operation |
| First dynamic discriminator | Title runner plus psxport executor | Report first VSync exit/resume and subsequent execution beyond `0x800A7F90`, with translated execution and explicit fallback denominators |
| Representative gameplay evidence | Future title-owned bounded scenario and diagnostics | Prove meaningful player control, state/memory, interrupt/timing, devices, audio, rendering, and frame time |
| Static-path exclusion | `tools/source_policy.py`, `tests/test_source_policy.py` | Reject the deleted offline translator, generated corpus, static dispatcher, and compatibility markers before any dynarec implementation |
| Player integration boundary | `CMakeLists.txt`, `tools/launcher.py` | Name the unimplemented title field lifecycle/native presentation boundary until the player product can supply them |
| Asset-free verification | `tools/verify.py`, `tools/ci_framework.py`, `tests/test_authenticated_image.cpp`, `tests/test_title_identity.py`, `CMakeLists.txt`, `.github/workflows/ci.yml` | The title bootstrap fetches its immutable framework pin before the shared setup action exists locally; psxport then owns Linux dependency provisioning and build/test/link policy; admission tests exercise the probe's byte authentication |
| Project facts and evidence order | `docs/project-state.md`, `docs/re-frontier.md`, `docs/codemap.md` | State changes go to project state, ordered RE evidence to the frontier, and ownership changes to this map |
