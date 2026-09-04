# Project state

## Comparison baseline

The baseline is the unmodified USA PlayStation release of *C-12: Final Resistance* on original
hardware or a PS1 emulator. The intended product authenticates the user's image, executes any
title-owned native overrides directly, and translates every remaining MIPS instruction at runtime
through psxport's pinned Lightrec revision.

## Current focus

**S003** — Adopt psxport's per-`Core` Lightrec executor and prove that the gameplay product contains
no interpreter. Do not regenerate, build, or run the static product. Its retained files are not a
product, fallback, or oracle.

## Capability inventory

| ID | Capability / observable outcome | State | Dependencies | Goals |
| --- | --- | --- | --- | --- |
| S001 | The USA disc resolves through `SYSTEM.CNF` to authenticated `SCUS_946.66` | verified | — | G001 |
| S002 | The pre-migration execution boundary is recorded through first VSync and guest PC `0x800A7F90` | verified | S001 | G001 |
| S003 | The gameplay product executes all non-native guest code through psxport's pinned Lightrec dynarec and contains no interpreter | missing | S001, shared psxport executor | G001 |
| S004 | The authenticated program returns from first VSync and executes beyond `0x800A7F90` through Lightrec | missing | S003 | G001 |
| S005 | Representative interactive gameplay passes with correct input, timing, interrupts, devices, audio, rendering, and per-host frame time | missing | S004 | G001 |
| S006 | The offline translator, generated corpus, static dispatcher, seed-only metadata, and static-only checks are deleted without a compatibility mode | missing | S005 | G001 |

## Evidence and exact gaps

### S001 — Authenticated target

Evidence: The supplied USA CHD resolves through `SYSTEM.CNF` to `SCUS_946.66`. This proves title
selection and runtime-image input, not gameplay execution.

### S002 — Recorded migration boundary

Evidence: The recorded pre-migration path starts `SCUS_946.66`, reaches the previously identified
libetc VSync boundary once, and then refuses guest PC `0x800A7F90`. That address is retained as a
discriminator because it identifies the first path the former static dispatcher did not own. It is
not evidence for a title frame-loop defect or a VSync configuration defect.

### S003 — Native/dynarec product

Missing capability: consume psxport's maintained, pinned Lightrec revision as a per-`Core` gameplay
executor. The runtime must synchronize CPU and machine state, own bounded exits and invalidation, and
key future native overrides by complete runtime image identity plus address. Product link and selector
inspection must prove that the interpreter in the separately built test target, including diagnostics,
is absent and unreachable.

### S004 — First dynamic discriminator

Missing capability: execute the authenticated whole program with nonzero Lightrec blocks, cross the
first VSync as an explicit executor exit/resume, and continue beyond `0x800A7F90`. A pass reports both
the reached VSync boundary and subsequent guest execution; silence or merely starting the executable
cannot pass.

### S005 — Representative gameplay conformance

Missing capability: establish and drive a bounded interactive gameplay scenario. Boot, first VSync,
logos, menus, attract mode, and FMV remain checkpoints only. The scenario must prove meaningful input
response and cover relevant guest state, memory, interrupt/timing, devices, audio, rendering, and
frame-time behavior on each released host architecture, using an independent emulator or separately
built test oracle for divergence diagnosis.

### S006 — Static-path removal

Missing capability: only after S005 passes, remove every offline translation and generated-product
surface in the same milestone. Until then the existing files are retention-only: do not regenerate,
build, run, select, or use them as an oracle. Removal leaves no legacy mode, fallback, or tombstone.
