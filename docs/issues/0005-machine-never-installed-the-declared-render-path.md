# `c12::Machine` never installed the title's render path

- status: resolved 2026-10-09
- state: S005, S009
- discovered: 2026-10-09

## Resolution

`c12::Machine` composes its own boot spine and never called `render_path_install`, the step every
other spine runs, so `C12Runtime::renderCapabilities` was not consulted: no `render path = ...` line
was logged and the Core stayed on its default mode. Declaring `RenderPath::Record` alone changed
nothing (`recordcheck` printed no lines). Fix at the owner: `game/boot/machine.cpp` calls
`render_path_install(&core)` after the device binds, and `C12Runtime::renderCapabilities` declares the
record path with no native producers and no interpolation. Tests:
`test_c12_declares_the_record_path_without_producers` and
`test_c12_machine_installs_the_declared_render_path` in `tests/test_runtime_services.cpp` (the second
failed before the call was added). Measured after: 2,462 `recordcheck` lines, 0 mismatched, boot to
the first mission at 1x 4:3.
