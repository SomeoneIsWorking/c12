# C-12 startup stock-libcd reads never complete; the title pumps a 60-field retry cycle

- status: resolved 2026-10-01
- state: S003, S004
- discovered: 2026-09-14

## Resolution (2026-10-01)

The guest waits for a BIOS CD-ROM interrupt that calls the function pointer in its own libcd
`CdReadyCallback` slot, `0x800EEEBC` (`FUN_800AC158` stores to it; `FUN_800AF7EC` installs
`FUN_800AF4A8` and `FUN_800AF4A8` restores the saved value). Four defects stood between that and a
delivered completion, all in framework owners, none of them address hacks:

1. **The title declared nothing to deliver to.** psxport's stand-in for the BIOS CD-ROM interrupt
   handler serves only a title that declares `GuestCdStreamCallbackLayout`; C-12 declared none, so
   the ready callback had no owner. It now declares its measured slot with `GuestInterrupt`
   delivery (the completion arrives as INT1, which is what the linked libcd contract says) and
   libcd's data-ready code 1, the value `FUN_800AF4A8` branches on.
2. **Both owners delivered the same sector.** `cd_drive_stock_read` burst the callback straight out
   of `CdControl(ReadN)` without touching the controller, so the controller's own data-ready
   response stayed owed and the interrupt arm delivered it again to whatever callback was installed
   by then. `Cd::pumpStream` already made the two owners exclusive; the finite-read path did not.
3. **A command the framework had already answered executed after the next one.** The
   synchronous command owner answers the guest immediately, but `cdc_issue_command` scheduled the
   controller's Pause on the guest clock; it then executed after the guest's `ReadN` and cancelled
   that read's own sector event. A framework-issued command now runs to completion in line.
4. **A chained read served one sector twice, then stopped.** The request register was a pure latch,
   so the repeated `BFRD` write stock libcd issues per sector presented nothing, and the read ran
   three sectors deep before stalling. A request now presents the announced sector once the guest
   holds the previous sector's payload, and that handoff re-arms the drive's own clock.

Result on the authenticated image: the retry cycle is crossed and stays crossed — a 2,000-turn run
contains no `CdRead: retry...` and no `CdRead: sector error`, 121-sector reads stream continuously,
and the title proceeds to load and execute its own disc-loaded module.

## Evidence

The authenticated USA probe with host display-field stepping (one shared `gpu_pace_frame` call per
executor turn) advances the guest's own libetc vs-count (`0x800EEB98`) — 597 at 600 turns — and
crosses the stock `0x800B4CA8` command wait, whose guest work area is `0x800EEED0..D4`.
Guest startup reaches its own
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

## Untested hypothesis (2026-09-29)

A stopped session left an unverified edit that removed `cdCommandAddress` and the stock work area
from `kPlatformHlePlan`, on the theory that C-12's threaded libcd must drive the emulated CDC through
its own guest body and that the synchronous shared command owner swallows the read completion. It
was dropped unverified. Test it as the discriminator above: unbind, run the authenticated probe, and
count `CdRead: retry...` lines and entries to `FUN_800af4a8` against the bound baseline.
