---
id: 0004-new-game-stalls-on-the-gp-b48-streamer-pending-spin
title: NEW GAME stalls on FUN_800582CC's spin while the streamer pending counter 0x800F0758 never drains
status: open
labels: [blocker, loading, gameplay]
---

`X` on NEW GAME reaches master `0x800F1B40` 6→2 with request `0x800F1B44=5` and stays there for
24–43k presented fields on black. This issue narrows the wait to a single 16-instruction loop and
names the word it polls, with the register evidence attached. **Not fixed.**

## The wait, from Ghidra (EXE, `c12` manifest entry verified correct)

`FUN_800582cc` (`scratch/x1/gh5/c/800582CC.c`, 16 instructions, `ret=1`, 2 calls) is a bare spin:

```c
void FUN_800582cc(void) {
  iVar1 = *(int *)(unaff_gp + 0xb48);
  while (iVar1 != 0) {
    FUN_800a1758(0);
    FUN_800573e4();
    iVar1 = *(int *)(unaff_gp + 0xb48);
  }
}
```

No gate, no timeout, no other condition — it returns only when that one word is 0.

## The word is `0x800F0758`, measured, not assumed

`PSXPORT_WWATCH="800F0758,800F075C" PSXPORT_WWATCH_GPR=1` at the store that leaves the counter at 5:

```
f1225 store [800F0758]=00000005 by pc=800B47E8
  gprs: r28=800EFC10 ... r18=800EF194 r19=00000002 ...
```

`r28` is the guest `gp`, and `0x800EFC10 + 0xB48 = 0x800F0758`. The project's existing note already
recorded `gp=0x800EFC10`; this confirms the `+0xB48` mapping rather than inferring it.

## Who writes it, and why it never comes back

Only one writer, `0x800B47E8` inside `FUN_800b4760` — the 7-byte CD response copy. The whole run:

```
f0    [800F0758]=0 by pc=800A8454   (EXE entry, init)
f409  =0   =1  =2  =3  =4           by pc=800B47E8
f862  =5                              by pc=800B47E8
      (nothing after)
```

It rises one per CD response and is never decremented, so from `f862` on, `FUN_800582cc` cannot return.
Its six callers are `FUN_80057d5c` (twice), `FUN_8005830c`, `FUN_800583c4`, `FUN_80058528`,
`FUN_8005858c`.

## Correcting two earlier claims

1. **The wait is NOT in the GT module.** `RELOCS/GT.LVB` is now analysed: its post-relocation 16 KiB
   (`0x8011F9BC..0x801239BC`) is declared as manifest image `c12_gt` and analyses to 14 functions, and
   **no instruction word in it builds `0x800F1B40`/`0x800F1B44`/`0x800F1B4C` at all** — the low
   halfword `0x1B40`/`0x1B44`/`0x1B4C` does not appear in any word of the blob. A register-tracking scan
   of the whole arena `0x80105E40..0x801FFFF0` and of the EXE text likewise finds no store to
   `0x800F1B40..0x800F1B50`. So the "written by the resident GT module" attribution is wrong.
2. **`FUN_800582CC` was wrongly ruled out.** The open item dismisses it because "its gates need pending
   `> 0x14`". The decompiled body has no such gate: it spins on `pending != 0` alone, and pending is 5.

## The whole queue, resolved (2026-10-03, second pass)

`gp = 0x800EFC10`. Everything below is measured at the stall with `rw`/`r` over 13.5k–86.7k fields:

```
pending B48 = 5     read B58 = 0   write B5C = 5   state B68 = 0
bits    B60 = 0x201 status B3C = 2  open 28C = 1   cb EEBC = 0x80057E8C  cb EEB8 = 0x80057F7C
```

### The contract, from Ghidra

| function | role | evidence |
|---|---|---|
| `FUN_800572c0(id,arg)` | **enqueue** a CD command: `pending++`, push `{id,arg}` to the 0x20 ring at `0x800F4020`, bump write index | `game/...` `scratch/x2/g5/c/800572C0.c` |
| `FUN_800573e4()` | the **pump**, called on every `FUN_800582cc` iteration. Issues a queued command **only** when `state==1` (`FUN_800577ac(queue[read])`) or `state==2` (advance read, issue). `state==0` takes no branch | `scratch/x2/g8/c/800573E4.c` |
| `FUN_800577ac(entry)` | the **dispatcher**: sets `state = 0` on entry (`DAT_800f0778 = 0`), then `FUN_800abd98(id, table, &DAT_800f0740)` | `scratch/x2/g8/c/800577AC.c`, `.../g10/c/800ABD98.c` |
| `FUN_800abd98` = `CdReadySync` | saves `DAT_800eeeb8`, **zeroes it**, retries `FUN_800b4ca8` 3× | `.../g10/c/800ABD98.c` |
| `FUN_800b4ca8` = `CdlSync` | the seam — already declared by this title as `kCdCommandAddress = 0x800B4CA8`, i.e. psxport's instant CD owner | `game/facts/title_facts.h` (`kCdCommandAddress`) |
| `FUN_800ac008` = `CdSync` | same shape, then **`FUN_800b4760(0, result)`** | `.../g11/c/800AC008.c` |
| `FUN_800b4760(timeout, buf)` | the **poll loop** that delivers completion: calls `(*DAT_800eeeb8)(DAT_800ef194, &DAT_80105790)` when `FUN_800b41fc() & 2`, and `(*DAT_800eeebc)` when `& 4`, until `FUN_800b41fc()` returns 0 | `scratch/x2/gh/c/800B4760.c` |
| `FUN_80057f7c(stat, buf)` | the **ready callback**: `gp[0xB3C] = *buf`, then `state = 2` and **`pending--`** at `0x800582AC..0x800582B8` | `scratch/x2/g5/c/80057F7C.c`, `scratch/x2/g8/c/800577AC.c` tail |
| `FUN_800582cc` | `while (*(int *)(gp + 0xB48) != 0) { VSync; FUN_800573e4(); }` | `scratch/x1/gh5/c/800582CC.c` |

`pending` has exactly **seven** writers in the whole executable: `+1` at `0x800572E4` (enqueue),
`= 0` at `0x80057C78` (CD open, `FUN_80057c50`), `-1` at `0x80058200` and `0x800582B8` (both inside
`FUN_80057f7c`), and two inside `FUN_80057f7c`'s error paths. **Nothing else can drain it.**

### The deadlock, stated exactly

`pending = 5` with `state = 0`. The pump's three branches are `pending==0` / `state==1` / `state==2`;
with `state == 0` and `pending != 0` it falls off the end of the function having issued nothing. The
dispatcher is never re-entered, `FUN_800b41fc()` is never consulted, `FUN_800b4760`'s poll loop never
runs, `FUN_80057f7c` is never called, and `pending` cannot reach 0. `FUN_800582cc` spins forever.

`state == 0` is written in exactly one place: **the first instruction of `FUN_800577ac`**. So the last
dispatcher entry cleared it and the completion that was supposed to set it back to 1 or 2 never came.

So the lead-(b) reading is right and the original "pending count" note was wrong in an important way:
`0x800F0758` is **not** a CD response buffer and not written by `FUN_800b4760`'s copy loop. It is the
**command-queue depth**, and the only thing that decrements it is the ready callback
`FUN_80057f7c` — which reaches it only through `FUN_800b4760`'s `(*DAT_800eeeb8)` poll, gated on
`FUN_800b41fc() & 2`.

### What is still open, and it is one function

Whether `FUN_800b41fc()` — the 345-instruction controller-status poll — reports bits 2/4 after psxport's
instant `FUN_800b4ca8`. Everything else is measured. That single answer decides the fix:

* if the poll reports the bits, the callback fires and there is no stall, so it must not; then the fix is
  that the CD owner delivers the completion **through the game's own `FUN_800b4760` poll** (the sync
  path, not the interrupt path), or a title-native owner at `FUN_800b4ca8` performs the game's own
  post-command completion that retail's BIOS interrupt handler delivered;
* the fix is NOT at `FUN_800582cc` and NOT at `FUN_800573e4`. Forcing `pending = 0`, forcing
  `state = 2`, or skipping the spin would each mask the missing completion and leave the game believing
  five CD commands had run.

## Where the request-5 write actually happens — still open

`0x800F1B44` does change (0→5) during the run, yet **no store to `0x800F1B40..0x800F1B50` is ever seen by
`PSXPORT_WWATCH`** after `f56`, even though `wwatch_check` runs on every `writeGuestMemory` including
Lightrec's. `0x800F1B44` is also outside the EXE's imported window (text ends `0x800F0800`), so Ghidra
reports it as OUTSIDE and `--refs` cannot see it. Three explanations remain open and are not guessed
between here: a write path that bypasses `writeGuestMemory` (DMA or a bulk copy), a KSEG1 alias the
watchpoint range test misses, or a pointer-computed store. Until that is settled, the *next* step is
that question, not an owner for `FUN_800582CC`.

## What a fix must not be

Not a drive-timing change, not a phase or timer write. CD reads are instant and synchronous by
design; a guest wait that cannot complete under them is adapted at the waiting function with a native
owner that completes the same work synchronously. The candidate owner is `FUN_800582CC` — but only once
the request-5 write is attributed, because a native owner that drains `0x800F0758` while the request
producer is still unknown would mask the real stall rather than fix it.

## Evidence

`scratch/x1/` (drivers), `scratch/x1_d1/` (RAM dump, stall snapshot), `scratch/x1_w2/`, `scratch/x1_w3/`
(wwatch runs), `scratch/x1/gh3..gh5/` (Ghidra C for `FUN_8003736c`, `38094`, `37bfc`, `582cc`),
`scratch/x1/gt_module.bin`.