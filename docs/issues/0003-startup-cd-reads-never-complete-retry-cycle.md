# C-12 startup stock-libcd reads never complete; the title pumps a 60-field retry cycle

- status: open
- state: S003, S004
- discovered: 2026-09-14

## Evidence

The authenticated USA probe with host display-field stepping (one shared `gpu_pace_frame` call per
executor turn) advances the guest's own libetc vs-count (`0x800EEB98`) — 597 at 600 turns — and
crosses the stock `0x800B4CA8` command wait recorded by issue 0002. Guest startup reaches its own
memory report (`Code: 677 Kb`) and enters `FUN_800afb70` (`0x800AFB70..0x800AFC73`), a title-owned
pump that ticks `FUN_800af7ec(1)` per field.

That tick is the title's CD-read state machine: it pauses (`CdControl 0x09`), re-Setlocs from the
published guest work area through `FUN_800b83f0` (`CdLastPos` at `0x800EEED0`), Setmodes
(`0x0E`), registers a ready callback (`FUN_800ac158(FUN_800af4a8)`), and kicks the read
(`FUN_800abed4(6,0)`). It then waits on its per-field state. Across 600 turns it printed
`CdRead: retry...` ten times at exactly the 60-field (`0x3C`) timeout spacing of its own pump
condition, and never advanced. Every command is acknowledged by the installed shared
`cd_command_stock_sync`; the read completion never reaches the guest.

Zero positive VSync waits occurred in 600 turns (`frame-boundaries=0`), so the first typed
`FrameBoundary` exit remains unobserved in the current image too.

## What is already known good

The shared prerequisites landed: the CDC command phase machine (`psxport 8611d756`) and the
guest-interrupt-owned stream callback order (`psxport b2510d06`). C-12 declares
`cdCommandAddress=0x800B4CA8` and its stock work area; both are exercised by the shipping
`c12_runtime_services` test and this probe.

## Next discriminator (not yet chosen from evidence)

Observe at the read boundary on the real image: whether the CDC delivers sectors and latches the
CD-IRQ (I_STAT bit 2), whether the guest's IRQ dispatcher claims it (the one logged `no SysEnq
element claimed it (0 in chain)` line was the VBlank edge via custom exception exit), and whether
`FUN_800af4a8` is ever entered. The retry period proves field timing is live; the missing link is
between read-kick acknowledgement and guest callback entry.
