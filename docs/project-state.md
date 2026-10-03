# Project state

Baseline: the unmodified USA PlayStation release of *C-12: Final Resistance* on hardware or an
emulator. The intended product authenticates the user's image, runs native title-owned overrides,
and translates all remaining guest code at runtime through psxport's pinned Lightrec revision.

Current focus: past the title screen (on screen, drawn by the guest; palette dark vs console,
uncompared) through the Start-driven menu (all three items respond) into gameplay — NEW GAME stalls
on black at master 6→2 (`0x800F1B44=5`, resident-module wait; see open items). Ordered RE evidence
lives in `docs/re-frontier.md`.

| ID | Capability | State | Evidence or exact gap |
| --- | --- | --- | --- |
| S001 | USA disc resolves through `SYSTEM.CNF` to authenticated `SCUS_946.66` | verified | 921,600 bytes, SHA-256 in `title.json`; `tests/test_authenticated_image.cpp` covers bounded reads and refusals |
| S002 | Pre-migration execution boundary recorded (first VSync, then guest PC `0x800A7F90`) | verified | recorded discriminator retained as a fact, not a static plan |
| S003 | Authenticated program executes non-native guest code dynarec-first with bounded, reason-coded fallback | verified | `c12_boot_probe` runs the real image: 2,000 turns, 1,038 typed `FrameBoundary` exits, 92.5 M guest instructions, zero fallback blocks/instructions, zero faults; startup CD reads complete through the guest's registered ready callback and the title's own module (`RELOCS/GT.LVB`) executes from RAM |
| S004 | Program returns from first VSync and executes beyond `0x800A7F90` through Lightrec | verified | Same run: execution continues past the recorded static boundary, into the disc-loaded module, into the title's per-field loop (its VSync at `0x80121204`) and its drawing submissions |
| S005 | Representative interactive gameplay with input, timing, interrupts, devices, audio, rendering, frame time | partial | `c12_port` presents the title with text and menu (`C-12 FINAL RESISTANCE` / `PRESS START BUTTON` / LOAD GAME / NEW GAME / OPTIONS; menu opens on Start — Start verified to reach the guest's own pad words `0x80102EFC`/`F00`, and a 65 s no-press control proves the menu is Start-driven). LOAD GAME reaches its card screen and OPTIONS reaches its setup screen. NEW GAME stalls on black (master 6→2, `0x800F1B44=5`): 24–43k presented fields, 0 new translations, 0 fallback, 0 faults, display on — see the NEW GAME stall open item. Packet-pool attribution is no longer blind: the title declares the measured window `0x800E3DE4..0x801B0F9C` holding both parity ordering tables, and the `FramePresenter::capture OVERFLOW` is gone with the per-field `commit` that was its cause. The composite/present seam's alternate-raster-line loss is fixed at its cause in psxport (see the first open item). Still missing: the palette is dark vs console (uncompared), and gameplay past the menu |
| S006 | Offline translator, generated corpus, static dispatcher, and static-only checks deleted with no compatibility mode | verified | `tools/source_policy.py` rejects their paths and source markers; `c12_port` builds the player product and `c12_boot_probe` remains a bounded maintainer tool, with no compatibility or selector path |
| S007 | Hosted CI distinguishes repository policy from native product support | partial | Linux job passes (`tools.verify` on Clang); no Windows, macOS, Android, or WASM job exists |
| S008 | Widescreen renders additional source geometry with correct projection and culling | missing | projection and culling owners not recovered |
| S009 | Native scene construction produces the C-12 picture from recovered title state | missing | no scene/geometry/material owner exists; the picture so far is the GUEST's own drawing, which is the correct Phase-1 owner, and its remaining fidelity defect (dark palette) is the 15-bit GP0 rasterizer's, not a missing native renderer |
| S010 | 60 fps source-geometry interpolation preserves authored simulation timing | missing | title simulation cadence unmeasured |
| S011 | Asynchronous loading; logos/sequences support complete cancellation | missing | no loading owner; the guest's own disc module now loads and runs (see `docs/re-frontier.md`), but Start has not been driven into the logo sequence, so cancellation is unreached |
| S012 | No-terminal picker authenticates the complete user installation, including bounded nested ZIP | missing | launcher only validates a disc path |
| S013 | Saves and settings persist in OS application data | missing | no persistence owner |
| S014 | Physical controller and keyboard controls drive the title | missing | no player input path |
| S015 | Authored SVG touch controls with multitouch, cancellation, safe areas, controller handover | missing | no touch layer |
| S016 | Asset-free Linux AppImage installs and runs qualified gameplay | missing | no packaging target |
| S017 | Asset-free Windows package | missing | no Windows build or package |
| S018 | Asset-free macOS application | missing | no macOS build or package |
| S019 | Android APK with arm64 dynarec gameplay and SVG touch input | missing | no `android-port` integration |
| S020 | WASM/GitHub Pages release runs the shipping runtime | missing | no browser backend qualification |
| S021 | Independent oracle comparison diagnoses execution and gameplay divergence | missing | no C-12 oracle scenario |

## Open items in the current focus

- **CLOSED as a capture artefact: the "alternate-raster-line loss" was never in C-12's picture.** Every
  row measurement behind the claim came from `present_shot`, and that instrument loses alternate rows
  at some capture sizes on this host. Measured with an RGB-only row metric (the missing rows carry
  alpha 255, so RGBA means hide it): at `PSXPORT_PRESENT_SINK=512x480` the capture is balanced
  (even 0.0631 / odd 0.0633, ratio 1.004), and at 512x240 on llvmpipe it is balanced too (0.0139 /
  0.0146, ratio 1.047). At 512x240 on the default Vulkan driver it reports odd rows missing
  (ratio 0.000) and at 1280x720 odd rows at half brightness (0.500) — both instrument readings, not
  product defects. The product-side inputs are all measured sound: the composite texture is whole in
  BOTH interlaced field windows (the guest alternates `disp.y` 0 and 256, and only the even field had
  ever been checked — 240/240 non-blank rows in each), the present pass geometry is 1:1
  (512x240 target, 320x240 viewport, source rect 320x240, no interlace term in either shader), and
  presented row N matches composite row N rather than 2N, so the fragment shader is not
  double-sampling. The short-extent upload characterisation, the driver/validation/pitch matrix
  behind it, the Spyro 2/3 "regressions" and the Atlas/Gte rule that compensated for them are all
  withdrawn as measurements of this instrument; the whole-canvas copy is reverted to `HEAD`'s
  per-region copy, which produces a byte-identical image (same SHA-256). The residual defect is
  recorded, unfixed and not being fixed now, as psxport issue
  `0148-present-image-loses-alternate-rows-at-some-sink-sizes`.

- **That seam fix broke Spyro 2 and Spyro 3, and both are now fixed at the cause.** The first rule chose
  the whole-canvas copy from `guestVramIsPicture()`, which answers a different question (must `render_geom`
  leave the VRAM backdrop alone). Spyro 2 and 3 both declare it TRUE — "no native producer, every
  presented field is guest VRAM" — while running `RenderPath::Gte`, where the guest's GP0 primitives are
  rasterized into the PC composite and never into CPU VRAM, so the composite already held the finished
  frame. Staging the whole canvas over it replaced that frame with the guest's raw upload. Measured:
  Spyro 3's presented frames alternated full picture and solid black (25 black frames of 59 sampled,
  137 distinct colours against HEAD's 0 and 2322), and Spyro 2 showed every frame as pink plus raw
  VRAM. The shipped rule is now `vram_upload_extent()` in `runtime/psx/gpu_vk_present_policy.h`: the
  whole canvas only when no guest geometry is rasterized into the composite (not Gte) and this present
  rasterized nothing; otherwise the per-region copy. `guestVramIsPicture()` remains an input, not the
  decision. After it: Spyro 3 1 black of 59 with 2401 distinct colours, Spyro 2 and Spyro 1 at HEAD's
  profile, and C-12 unchanged at 62.5% / 0 dark rows — C-12 is the one title that takes the
  whole-canvas arm. Spyro 1/2/3 are `Gte`, so they now take the identical branch `HEAD` does. The
  decision has a unit test (`test_vram_persistence`, 11/11) because it was previously found by
  bisecting a live title.
- **SUPERSEDED 2026-10-02 by the item below ("Start reaches the guest...").** The RAM-diff
  reading was withdrawn already; the "no text / plasma scene" reading is withdrawn now too: captures
  from 25 s after boot show the full title (`C-12 / FINAL RESISTANCE / PRESS START BUTTON`) and, after
  Start, the menu (LOAD GAME / NEW GAME / OPTIONS). The earlier textless frame was an early-boot moment,
  not the settled screen. The static ownership behind the old "next step" is now recovered: the scene
  selector is master `0x800F1B40` (6 = GT-module title, 2/3 = scene loop) with scene index `0x800F1520`
  in `FUN_8003736c`; the pad IS polled every field (`FUN_80038094` → `FUN_80043dec` → `FUN_800a122c` →
  `FUN_800a129c`/`FUN_800a1580` into `0x80102EEC`+port*`0x30`, scanned against the button table at
  `0x800CCF1C` initialised by `FUN_80043d54`); the leave-scene path from the menu is the resident
  module's own `0x800F1B44=5` request. What remains is the NEW GAME stall item, not input delivery.
  (Remainder of the old item, kept for provenance; its conclusions are withdrawn above. Note one
  correction for future readers: the `0x800A0E14`/`0x800A0E4C`/`0x800A0EF8`/`0x800A0F24` continuations
  are NOT inside `FUN_800a0854` — Ghidra places them in `FUN_800a0e04` / `FUN_800a0e3c` / `FUN_800a0ebc`,
  and `FUN_800a0854` itself is a straight-line 172-instruction draw-table walker for entry
  `param_1` at `0x800E3E88`+`param_1`*0x10. An earlier run appeared
  to go solid black on a second press; it did not reproduce and ran while a stray `c12_port` and other
  agents' instances contended for the GPU, so it stays unconfirmed.
- **The title screen's palette is dark.** The boot loading screen at field 50 renders in full colour
  with its artwork, so the palette path works; the title screen's own colours have not been compared
  against the console, and semi-transparency (`0xC0` rect flag) and CLUT sampling for its `E1`
  texpages are unmeasured. Note that `E4` in this title's tables is the drawing-area bottom-right, not
  a drawing mask — an earlier reading of the same bytes as a mask was wrong.
- **Packet attribution inside the declared window is coarse.** The window
  `0x800E3DE4..0x801B0F9C` is where packets were measured being submitted from, and the band also
  holds the guest's own globals (`0x800EEEBC`, the heap cursor), so a store inside it is not yet known
  to be render data. Splitting it further needs the guest's own allocation discipline recovered.
- **Start reaches the guest and drives the menu; verified 2026-10-02.** Seven headless
  runs (`scratch/hold_hold1`, `nopress_np1`, `menu_m1`, `newgame_n2`, `load_l1`, `loadaud_a1`,
  `newaud_n1`, scripts beside them). Holding Start flips the guest's own pad words
  `0x80102EFC`/`0x80102F00` from `0` to `0x10001000` and back to `0` on release (the low-level
  pump `FUN_800a122c` ← `FUN_800a129c`/`FUN_800a1580` runs; the per-field scan is `FUN_80043dec`,
  called from `FUN_80038094`/`FUN_8005d438`). `tap start` opens the menu (LOAD GAME / NEW GAME /
  OPTIONS, default highlight NEW GAME); a 65 s no-press control stays on PRESS START, so the menu
  is Start-driven, not timed. `UP,X` reaches LOAD GAME ("Accessing MEMORY CARD", master 6,
  `0x800F1B4C=1`); `DOWN,X` reaches an options screen (CONTROLLER/SOUND/SCREEN, master 6);
  `X` alone takes the NEW GAME path below. Captures opened and inspected in all runs.
- **NEW GAME stalls on a black screen after the menu; owners narrowed, unfixed.** `X` on NEW GAME
  moves master `0x800F1B40` 6→2 with request `0x800F1B44=5` (written by the resident GT module —
  no `=5` store exists in the EXE) and freezes there across 24–43k presented fields: zero new
  translated blocks, zero fallback, zero faults, display ON (`disp` valid), CB slots intact
  (`0x800EEEBC=0x80057E8C`), status idle (`0x800EF194=2`), streamer pending `0x800F0758=5` /
  state `0x800F0770=0`, queue `0x800F4020=[0D,2,5,0C,6]` write=5 read=0 mode=0, GT still resident
  (`0x800EFEAC=0x8011F9BC`, whose sole static writer `FUN_8003777c` never stored — so scene-2 init
  never reached its `RELOCS/TI.LVB` load: no new `module_load`/DMA), teardown list head NULL
  (`*(0x800D1F8C)=0`), per-field submission one 2x1 COPY + env only. Ruled out with evidence:
  the `FUN_800582CC` spin (its gates need pending `>0x14`; pending is 5), `FUN_80038094`'s outer
  loop (`0x800F151C&0x40==0`, read live), display-off, audio-disable (audio-on stalls identically),
  missing card file (HLE creates blank; zero card-subsystem log lines in every run — the card is
  never even opened: `state save` refuses for "memory card is not open"). Static chain (all bodies
  verified through `decomp_pipeline.py`): `FUN_8003736c` master machine → `FUN_8003777c` scene init
  → `FUN_8003798c` update → `FUN_80038094` VSync wait → `FUN_80037ef8` teardown; streamer queue at
  `0x800F4020`, dispatcher `FUN_800577AC`, guest gp measured as `0x800EFC10` (b48=`0x800F0758`,
  b60=`0x800F0770`, b68=`0x800F0778`). Next step: the wait lives in resident module code
  (`RELOCS/GT.LVB` at `0x8011F9BC`, entry `0x8011FA64`) past EXE-static reach — provision its bytes
  from the disc, decompile at the measured base, and find what completion its loop polls.
  NARROWED 2026-10-03 (issue `0004`): the wait is NOT in the GT module — that overlay is decompiled
  (`c12_gt`, 14 functions) and contains no store to `0x800F1B40..0x800F1B50`; the arena and EXE contain
  none either. It is `FUN_800582CC`'s `while (*(int *)(gp + 0xB48) != 0)` spin, whose word is
  `0x800F0758` (`r28 = 0x800EFC10` measured at the store), rising to 5 at `f862` and never drained;
  the "pending > 0x14 gate" that ruled this function out is not in its body (only 3 of its 4 call
  sites have it). RESOLVED 2026-10-03 (second pass): `gp+0xB48` is the CD COMMAND-QUEUE DEPTH, not a
  response buffer. `FUN_800572c0` enqueues; `FUN_800573e4` is the pump and issues a command only when
  `state` (`gp+0xB68`) is 1 or 2; `FUN_800577ac` sets `state=0` as its first instruction and calls the
  seam this title already declares, `kCdCommandAddress = 0x800B4CA8`; the only `pending--` in the whole
  executable is in the ready callback `FUN_80057f7c`, reached solely from `FUN_800b4760`'s
  `(*DAT_800eeeb8)` poll gated on `FUN_800b41fc() & 2`. Measured stall state: `pending=5 read=0
  write=5 state=0 status=2 open=1 cb EEB8=0x80057F7C` — with `pending!=0` and `state==0` the pump
  issues nothing, so the callback never runs and the spin cannot exit. Still open: whether
  `FUN_800b41fc()` reports bits 2/4 after the instant `0x800B4CA8`, and the `0x800F1B44=5` store,
  which no `PSXPORT_WWATCH` range has yet seen. The fix belongs at the CD completion seam, never at
  the spin or the pump.
  (Runs: `menu_m1` first X→2 transition + byte-identical black frames; `gate_g1`/`newaud_n1` for the long-horizon numbers; `queue_q1`, `cb_b1`,
  `mod_h1`, `list_t1`, `cls_c1`, `attr_o1` for queue/CB-slot/handle/list/classifier reads;
  `loadaud_a1` for audio-on LOAD; Ghidra C under `scratch/decomp/c12/c/`.)
- **Black screen after the credits screen is a guest-side spin, unfixed.** Deterministic
  `PC=0x800582E4` (`FUN_800582CC`, a leaf loop) sampling identically over 10 pause/`step 1000` cycles;
  it spins while the pending counter `0x800F0758` is non-zero and reads `5`, frozen across 14 s.
  `PSXPORT_WWATCH` shows that word written only by `FUN_800b4760`'s 7-byte CD response copy at
  `0x800B47E8` (an indirect store Ghidra's `--refs` cannot see), last written once at field 1641 with
  value 5. The streamer state word `0x800F0770`, which the loop's work function `FUN_800573E4` branches
  on, is written only twice in the whole run and both times with 0, so the streamer is never armed and
  the decrementer `FUN_800577AC` never runs a completing case. Ruled out: the CD layer (reads complete
  at consecutive LBAs), the pad (93,268 complete correctly-ACKed polls), and `0x0A`/`0x0C`, which are
  the guest's `CdInit` (`FUN_800b5300`) and whose success conditions our HLE does satisfy. Next step:
  what is supposed to arm `0x800F0770` — the arming path's gate, the shape of the CD response the
  guest reads back, or the fire count of its `CdReadyCallback` at `0x800EEEBC`.
