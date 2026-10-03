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