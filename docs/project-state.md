# Project state

Baseline: the unmodified USA PlayStation release of *C-12: Final Resistance* on hardware or an
emulator. The intended product authenticates the user's image, runs native title-owned overrides,
and translates all remaining guest code at runtime through psxport's pinned Lightrec revision.

Current focus: the 2D packet pool / ordering-table geometry the title's loaded module draws through,
so packet attribution is not blind and the frame presenter stops overflowing; then the title field
lifecycle and native presentation that make `c12_port` a real executable. Ordered RE evidence lives
in `docs/re-frontier.md`.

| ID | Capability | State | Evidence or exact gap |
| --- | --- | --- | --- |
| S001 | USA disc resolves through `SYSTEM.CNF` to authenticated `SCUS_946.66` | verified | 921,600 bytes, SHA-256 in `title.json`; `tests/test_authenticated_image.cpp` covers bounded reads and refusals |
| S002 | Pre-migration execution boundary recorded (first VSync, then guest PC `0x800A7F90`) | verified | recorded discriminator retained as a fact, not a static plan |
| S003 | Authenticated program executes non-native guest code dynarec-first with bounded, reason-coded fallback | verified | `c12_boot_probe` runs the real image: 2,000 turns, 1,038 typed `FrameBoundary` exits, 92.5 M guest instructions, zero fallback blocks/instructions, zero faults; startup CD reads complete through the guest's registered ready callback and the title's own module (`RELOCS/GT.LVB`) executes from RAM |
| S004 | Program returns from first VSync and executes beyond `0x800A7F90` through Lightrec | verified | Same run: execution continues past the recorded static boundary, into the disc-loaded module, into the title's per-field loop (its VSync at `0x80121204`) and its drawing submissions |
| S005 | Representative interactive gameplay with input, timing, interrupts, devices, audio, rendering, frame time | missing | the guest submits drawing primitives, but no player field lifecycle or native picture exists and packet-pool attribution is blind (`FramePresenter::capture OVERFLOW ... > RQ_MAX`) |
| S006 | Offline translator, generated corpus, static dispatcher, and static-only checks deleted with no compatibility mode | verified | `tools/source_policy.py` rejects their paths and source markers; `c12_port` still names the missing presentation boundary |
| S007 | Hosted CI distinguishes repository policy from native product support | partial | Linux job passes (`tools.verify` on Clang); no Windows, macOS, Android, or WASM job exists |
| S008 | Widescreen renders additional source geometry with correct projection and culling | missing | projection and culling owners not recovered |
| S009 | Native scene construction produces the C-12 picture from recovered title state | missing | no scene/geometry/material owner exists |
| S010 | 60 fps source-geometry interpolation preserves authored simulation timing | missing | title simulation cadence unmeasured |
| S011 | Asynchronous loading; logos/sequences support complete cancellation | missing | no lifecycle or loading owner |
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
