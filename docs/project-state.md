# Project state

Baseline: the unmodified USA PlayStation release of *C-12: Final Resistance* on hardware or an
emulator. The intended product authenticates the user's image, runs native title-owned overrides,
and translates all remaining guest code at runtime through psxport's pinned Lightrec revision.

Current focus: the presented picture's fidelity — C-12's title screen is on screen, drawn by the
guest, with the palette dark — and then driving Start into the logo sequence. Ordered RE evidence
lives in `docs/re-frontier.md`.

| ID | Capability | State | Evidence or exact gap |
| --- | --- | --- | --- |
| S001 | USA disc resolves through `SYSTEM.CNF` to authenticated `SCUS_946.66` | verified | 921,600 bytes, SHA-256 in `title.json`; `tests/test_authenticated_image.cpp` covers bounded reads and refusals |
| S002 | Pre-migration execution boundary recorded (first VSync, then guest PC `0x800A7F90`) | verified | recorded discriminator retained as a fact, not a static plan |
| S003 | Authenticated program executes non-native guest code dynarec-first with bounded, reason-coded fallback | verified | `c12_boot_probe` runs the real image: 2,000 turns, 1,038 typed `FrameBoundary` exits, 92.5 M guest instructions, zero fallback blocks/instructions, zero faults; startup CD reads complete through the guest's registered ready callback and the title's own module (`RELOCS/GT.LVB`) executes from RAM |
| S004 | Program returns from first VSync and executes beyond `0x800A7F90` through Lightrec | verified | Same run: execution continues past the recorded static boundary, into the disc-loaded module, into the title's per-field loop (its VSync at `0x80121204`) and its drawing submissions |
| S005 | Representative interactive gameplay with input, timing, interrupts, devices, audio, rendering, frame time | partial | `c12_port` is a real product executable: 1,420 presented fields, 1,326 guest VSync boundaries, 0 fallback, 0 faults, and the title screen on screen ("C-12 FINAL RESISTANCE" / "PRESS START BUTTON", 40.68% non-black at field 1400). Packet-pool attribution is no longer blind: the title declares the measured window `0x800E3DE4..0x801B0F9C` holding both parity ordering tables, and the `FramePresenter::capture OVERFLOW` is gone with the per-field `commit` that was its cause. The composite/present seam's alternate-raster-line loss is fixed at its cause in psxport (see the first open item). Still missing: the palette is dark, and Start has not been driven |
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
- **Start has NOT been shown to reach the guest, and the title screen has not been observed.** Two
  readings were wrong and are withdrawn: a RAM diff around a forced press looked like ~2,900 bytes of
  tap-only change, but the churning words are the game's own scene table — `FUN_80043d54` is an
  initialiser that fills a 16-byte-per-entry table at `0x800CCF1C` downwards with `-0x3E8` counters,
  and it updates every field whether or not anything is pressed. The presented frame also looked static
  by aggregate and animated on close reading (mean absolute difference against the pre-tap frame
  12.5, 17.0, 18.7, 21.5, 22.1, 21.2 over six seconds at a constant 76,585 non-black), so both readings
  were artefacts of the wrong instrument. What IS established: the guest's per-field entry is a
  four-address cycle in `FUN_800a0854` (continuations `0x800A0E14`/`0x800A0E4C`/`0x800A0EF8`/
  `0x800A0F24`, 15 transitions in ~700 fields), which walks a 16-byte-per-entry draw table at
  `0x800E3E88` — inside the declared packet window — and issues draws through `FUN_800b0174` /
  `FUN_800b00b4` / `FUN_800b03fc`. The screen it presents is an animated full-screen plasma with no
  text, no logo and no prompt, so it is a SCENE, not a title screen. CD streams throughout (740
  data-ready callbacks into the title's own `0x800AF4A8` pump by t≈27 s), so the port is not stalled.
  The next step is to find the state that selects this scene and the routine that leaves it — the scene
  table's owner and whatever advances it, whether input or the CD stream — and to establish whether the
  guest's pad is polled at all in this state, before any more input is injected. An earlier run appeared
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
- **Start has not been driven.** The pad reaches the guest through the real SIO0 chain
  (`Pad::serviceFrame` per field) and the loopback control channel is live, but no run has yet pressed
  Start to reach the logo sequence.
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
