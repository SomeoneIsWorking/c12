# Project state

Baseline: the unmodified USA PlayStation release of *C-12: Final Resistance* on hardware or an
emulator. The intended product authenticates the user's image, runs native title-owned overrides,
and translates all remaining guest code at runtime through psxport's pinned Lightrec revision.

Current focus: the presented picture's fidelity — C-12's title screen is on screen, drawn by the
guest, with every other raster line missing and the palette dark — and then driving Start into the
logo sequence. Ordered RE evidence lives in `docs/re-frontier.md`.

| ID | Capability | State | Evidence or exact gap |
| --- | --- | --- | --- |
| S001 | USA disc resolves through `SYSTEM.CNF` to authenticated `SCUS_946.66` | verified | 921,600 bytes, SHA-256 in `title.json`; `tests/test_authenticated_image.cpp` covers bounded reads and refusals |
| S002 | Pre-migration execution boundary recorded (first VSync, then guest PC `0x800A7F90`) | verified | recorded discriminator retained as a fact, not a static plan |
| S003 | Authenticated program executes non-native guest code dynarec-first with bounded, reason-coded fallback | verified | `c12_boot_probe` runs the real image: 2,000 turns, 1,038 typed `FrameBoundary` exits, 92.5 M guest instructions, zero fallback blocks/instructions, zero faults; startup CD reads complete through the guest's registered ready callback and the title's own module (`RELOCS/GT.LVB`) executes from RAM |
| S004 | Program returns from first VSync and executes beyond `0x800A7F90` through Lightrec | verified | Same run: execution continues past the recorded static boundary, into the disc-loaded module, into the title's per-field loop (its VSync at `0x80121204`) and its drawing submissions |
| S005 | Representative interactive gameplay with input, timing, interrupts, devices, audio, rendering, frame time | partial | `c12_port` is a real product executable: 1,420 presented fields, 1,326 guest VSync boundaries, 0 fallback, 0 faults, and the title screen on screen ("C-12 FINAL RESISTANCE" / "PRESS START BUTTON", 40.68% non-black at field 1400). Packet-pool attribution is no longer blind: the title declares the measured window `0x800E3DE4..0x801B0F9C` holding both parity ordering tables, and the `FramePresenter::capture OVERFLOW` is gone with the per-field `commit` that was its cause. Still missing: every other raster line is missing (the guest's per-field drawing mask) and the palette is dark, and Start has not been driven |
| S006 | Offline translator, generated corpus, static dispatcher, and static-only checks deleted with no compatibility mode | verified | `tools/source_policy.py` rejects their paths and source markers; `c12_port` builds the player product and `c12_boot_probe` remains a bounded maintainer tool, with no compatibility or selector path |
| S007 | Hosted CI distinguishes repository policy from native product support | partial | Linux job passes (`tools.verify` on Clang); no Windows, macOS, Android, or WASM job exists |
| S008 | Widescreen renders additional source geometry with correct projection and culling | missing | projection and culling owners not recovered |
| S009 | Native scene construction produces the C-12 picture from recovered title state | missing | no scene/geometry/material owner exists; the picture so far is the GUEST's own drawing, which is the correct Phase-1 owner, and its two fidelity defects (missing alternate raster lines, dark palette) are the 15-bit GP0 rasterizer's, not a missing native renderer |
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

- **Every second raster line is lost between guest VRAM and the presented frame.** Measured at present
  1400 of the same run: a CPU capture of the declared display rect (`0x800E3DE4`-style read of VRAM
  rows 256..496, columns 96..416 of the 512-wide framebuffer) has **0** of 240 rows below mean 8, while
  the presented frame has **149**, alternating bright/black. The upload is not the loss — the same run
  reports `upload_vram 480 KiB` per frame (all 240 rows of the full width). So the row loss is between
  the uploaded 1555 texture and the composited present, and it is a framework rendering question, not
  a C-12 one. Start at the decode/encode fullscreen passes over the 1024x512 texture with a 512x240
  display window at y=256.
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
