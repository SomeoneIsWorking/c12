# Direct-runtime CD command binding must preserve C-12's stock libcd state

- status: resolved
- state: S003, S004
- discovered: 2026-09-12
- resolved: 2026-09-14

## Evidence and cause

The authenticated `SCUS_946.66` image maps the repeated `VSync(-1)` call at `0x800B4EBC` to
`FUN_800B4CA8` (`0x800B4CA8..0x800B50B3`). Ghidra found 13 direct calls to that stock libcd
command function from four callers. Its blocking path polls VSync for CD command completion, while
its fire-and-forget caller supplies mode 1. `PlatformHlePlan::cdCommandAddress` can bind the existing
shared synchronous `cd_command_stock_sync`, but that mapping alone is incomplete.

For CdlSetloc (`0x02`), the guest function copies four parameter bytes into `0x800EEED0..D3`; for
CdlSetmode (`0x0E`), it writes the mode byte to `0x800EEED4`. The executable's
`FUN_800B83F0` returns `&0x800EEED0`, and `FUN_800B8420` reads `0x800EEED4`. Thus these bytes are
guest-observable state, not incidental scratch data. A direct C-12 runtime has `core.cfg == nullptr`, so a command binding must declare the work area
instead of relying on `GameConfig::cdLastPosBuf`. The current shared `cd_apply_command` calls
`psx::cd::publishStockCommandWorkArea`, whose direct-runtime source is
`PlatformHlePlan::stockCdWorkArea`; the shared contract is present. C-12 had not published these facts.

## Current implementation and remaining evidence

C-12's existing platform-facts owner now declares the measured command entry and its exact four-byte
admission window, with `lastPositionAddress=0x800EEED0` and `lastModeAddress=0x800EEED4`. The
production-handler test checks all four position bytes, the mode byte, native drive positioning,
adjacent-byte preservation, and an undeclared-layout negative through the same installed shared
handler. Native build and execution are pending the serialized verification slot.

A bounded read authenticated against `title.json` on 2026-09-12 scanned 229,888 loaded words and
found 13 JAL encodings targeting the shipped `kCdCommandAddress`, including startup callsite
`0x800B5444`. This agrees with the recorded call graph and checks the source address against the
actual image. It does not prove that the native handler crosses the retail startup wait; that still
requires the bounded real-image probe after focused contract verification.

## Close (2026-09-14)

The Clang/Ninja consumer gate passed 4/4 CTest, including the 6/6 production-handler checks: the
installed shared `cd_command_stock_sync` publishes all four Setloc bytes and the Setmode byte of the
declared `0x800EEED0..D4` work area, updates native drive positioning, preserves adjacent bytes, and
the undeclared-layout negative refuses publication. On the authenticated USA executable the bounded
probe crosses the stock `0x800B4CA8` startup wait that previously repeated, and the title's own retry
state machine reads the published state back through `FUN_800b83f0`. Not verified by this close:
that the read itself completes — tracked as issue 0003.
