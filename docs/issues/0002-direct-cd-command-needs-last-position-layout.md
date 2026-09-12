# Direct-runtime CD command binding must preserve C-12's stock libcd state

- status: open
- state: S003, S004
- discovered: 2026-09-12

## Evidence and cause

The authenticated `SCUS_946.66` image maps the repeated `VSync(-1)` call at `0x800B4EBC` to
`FUN_800B4CA8` (`0x800B4CA8..0x800B50B3`). Ghidra found 13 direct calls to that stock libcd
command function from four callers. Its blocking path polls VSync for CD command completion, while
its fire-and-forget caller supplies mode 1. `PlatformHlePlan::cdCommandAddress` can bind the existing
shared synchronous `cd_command_stock_sync`, but that mapping alone is incomplete.

For CdlSetloc (`0x02`), the guest function copies four parameter bytes into `0x800EEED0..D3`; for
CdlSetmode (`0x0E`), it writes the mode byte to `0x800EEED4`. The executable's
`FUN_800B83F0` returns `&0x800EEED0`, and `FUN_800B8420` reads `0x800EEED4`. Thus these bytes are
guest-observable state, not incidental scratch data. The shared `cd_apply_command` currently mirrors
them only through `core.cfg->cdLastPosBuf`. A direct C-12 runtime has `core.cfg == nullptr`, so a
bare CD-command binding would acknowledge the command while leaving both values stale. The candidate
binding was removed before landing.

## Required transition

The shared CD owner needs a typed direct-runtime last-position buffer fact and must use it for both
Setloc and Setmode in the same command path that updates native drive state. C-12 can then publish
the measured base `0x800EEED0` and measured command entry `0x800B4CA8` through its title plan.
A discriminator must call the production handler in a direct runtime and prove the four position
bytes and adjacent mode byte match the guest command contract, while an undeclared layout must
refuse or remain untouched without falsely claiming complete CD service. An authenticated-image
check must compare C-12's shipped entry address to a real callsite. Only then should the bounded
real-image probe test whether this CD wait is crossed without a guest VSync loop.
