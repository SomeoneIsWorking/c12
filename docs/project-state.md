# Project state

Baseline: the unmodified USA PlayStation release of *C-12: Final Resistance* on hardware or an
emulator. The intended product authenticates the user's image, runs native title-owned overrides,
and translates all remaining guest code at runtime through psxport's pinned Lightrec revision.

Current focus: the first mission is reached and playable — NEW GAME shows the guest's `GRISHAM'S LOG`
page, then a briefing cutscene (`X SKIP`), then the first mission with the player character under
pad control — and it presents through psxport's record path at 4:3 with `recordcheck` at 0 mismatched.
The next step is what lies past that spawn (enemies, weapons, mission exit) and 16:9 on the record
path. Ordered RE evidence lives in `docs/re-frontier.md`.

| ID | Capability | State | Evidence or exact gap |
| --- | --- | --- | --- |
| S001 | USA disc resolves through `SYSTEM.CNF` to authenticated `SCUS_946.66` | verified | 921,600 bytes, SHA-256 in `title.json`; `tests/test_authenticated_image.cpp` covers bounded reads and refusals |
| S002 | Pre-migration execution boundary recorded (first VSync, then guest PC `0x800A7F90`) | verified | recorded discriminator retained as a fact, not a static plan |
| S003 | Authenticated program executes non-native guest code dynarec-first with bounded, reason-coded fallback | verified | `c12_boot_probe` runs the real image: 2,000 turns, 1,038 typed `FrameBoundary` exits, 92.5 M guest instructions, zero fallback blocks/instructions, zero faults; startup CD reads complete through the guest's registered ready callback and the title's own module (`RELOCS/GT.LVB`) executes from RAM |
| S004 | Program returns from first VSync and executes beyond `0x800A7F90` through Lightrec | verified | Same run: execution continues past the recorded static boundary, into the disc-loaded module, into the title's per-field loop (its VSync at `0x80121204`) and its drawing submissions |
| S005 | Representative interactive gameplay with input, timing, interrupts, devices, audio, rendering, frame time | partial | `c12_port` presents the title, the Start-driven menu (LOAD GAME / NEW GAME / OPTIONS) and, past NEW GAME, the game's own story page and a briefing cutscene and the first mission. **NEW GAME's black stall is FIXED at its cause**: the guest's CD command queue was deadlocked because the framework's declared `CdlSync` seam replaced the guest's own libcd body, which is where the guest's queue learns a command finished. Measured before: queue depth `0x800F0758=4..5` with read 0, write 4..5 and state `0x800F0778=0`, frozen over 20k presented fields. Measured after: depth 0, read 3, write 4, state 2 at the title (the console's own values) and read 9 / write 10 after the briefing is continued, with the guest still translating new blocks. The fix is this title's own owner at its measured `CdReadySync` entry `0x800ABD98`, which runs the guest's body and then delivers the guest's own command-status completion; no guest RAM word is written to release the wait. **Past the briefing, measured 2026-10-09**: no stop exists — the briefing is a skippable cutscene (`X SKIP`), one more `X` tap lands in the first mission (master `0x800F1B40=3`, `HEALTH` bar, the player character standing in a ruined street), and the character runs right then left under held pad bits (`hold`) and under the recorded replay `replays/first-mission/menu-to-mission-run-right-left.pad` (2,037 frames, 2,037 delivered, before/after shots at fields 1776/1884/2019 show the position change). Still `partial`: nothing beyond the spawn street is reached (no combat, weapons, objective or mission exit), and LOAD GAME / OPTIONS stop at their own screens. Both the title screen's and the gameplay screen's palettes are measured against the console and agree (see the two palette items) |
| S006 | Offline translator, generated corpus, static dispatcher, and static-only checks deleted with no compatibility mode | verified | `tools/source_policy.py` rejects their paths and source markers; `c12_port` builds the player product and `c12_boot_probe` remains a bounded maintainer tool, with no compatibility or selector path |
| S007 | Hosted CI distinguishes repository policy from native product support | partial | Linux job passes (`tools.verify` on Clang); no Windows, macOS, Android, or WASM job exists |
| S008 | Widescreen renders additional source geometry with correct projection and culling | partial | `PSXPORT_C12_WIDESCREEN` (default on, a `pc_enh` knob) declares 16:9 through `c12::WidescreenPolicy`; the frustum corners (`FUN_800660D8`) and the terrain polygon screen reject (`FUN_8006769C`) are native and widened, the guest OFX/OFY/H stay retail so the picture shows more world, not a stretch. 4:3 with the knob off: 5,089 recordcheck lines, 0 mismatched. Gaps: `FUN_800684C0` cell collector reject and object culling not widened; no 16:9 capture beyond the first mission spawn |
| S009 | Native scene construction produces the C-12 picture from recovered title state | partial | the picture is presented through psxport's record path (`C12Runtime::renderCapabilities` declares `RenderPath::Record`, no native producers, no interpolation), and at 1x 4:3 (`aspect=0`, `ires=1`, `PSXPORT_DEBUG=recordcheck`) from boot through the menu, story page, briefing and the first mission's spawn with movement, all 2,462 present lines report `mismatched=0` (every `complete=1`, 0 composed). `c12::Machine` never called `render_path_install`, so before 2026-10-09 the declared capability was ignored and no path was announced; the 16:9 margins and any native scene construction remain missing: the picture so far is the GUEST's own drawing, which is the correct Phase-1 owner, and its palette matches the console (see the palette items) |
| S010 | 60 fps source-geometry interpolation preserves authored simulation timing | missing | title simulation cadence unmeasured |
| S011 | Asynchronous loading; logos/sequences support complete cancellation | missing | no loading owner; the guest's own disc module now loads and runs (see `docs/re-frontier.md`), but Start has not been driven into the logo sequence, so cancellation is unreached |
| S012 | No-terminal picker authenticates the complete user installation, including bounded nested ZIP | missing | launcher only validates a disc path |
| S013 | Saves and settings persist in OS application data | missing | no persistence owner |
| S014 | Physical controller and keyboard controls drive the title | partial | The path exists and is the shipping one: `GuestFieldLoop::stepField` calls `Pad::serviceFrame`, which calls `Pad::pollHostInput` (`HostInput::poll` with the window answer) before every guest turn, and the guest reads the mask through its own SIO0 chain (menu, story page, briefing skip and mission movement all respond to `tap`, `hold` and `PSXPORT_PAD_REPLAY` through it). The earlier "no player input path" wording was stale. Gap: a physical SDL key or controller press in a live window has not been exercised by an agent (headless legs have no host input by design), so the keyboard/controller mapping is unverified on this title |
| S015 | Authored SVG touch controls with multitouch, cancellation, safe areas, controller handover | missing | no touch layer |
| S016 | Asset-free Linux AppImage installs and runs qualified gameplay | missing | no packaging target |
| S017 | Asset-free Windows package | missing | no Windows build or package |
| S018 | Asset-free macOS application | missing | no macOS build or package |
| S019 | Android APK with arm64 dynarec gameplay and SVG touch input | missing | no `android-port` integration |
| S020 | WASM/GitHub Pages release runs the shipping runtime | missing | no browser backend qualification |
| S021 | Independent oracle comparison diagnoses execution and gameplay divergence | missing | no C-12 oracle scenario |
| S022 | Increased draw distance draws more authored geometry with matching fog/fade | partial | `PSXPORT_C12_DRAW_DISTANCE` (default 50, a persistable `pc_enh` knob; 0..1000 in steps of 25, the INCREASE in percent, 0 is retail) is a stepped slider in the in-game menu (Display, Enhancements). The visible-cell collector (`FUN_800684C0`) is native (`c12::collectVisibleCells`): the override differential over the whole first-mission replay at every call reports no mismatch at 0, and 4:3 recordcheck at 0 is 4,588 presents, 0 mismatched. Above 0 it keeps a host cell list and the terrain pass draws it with the far clip and the stretched fade tables scaled by the same factor; the guest's 150-cell buffer, flags and GTE state stay retail, so gameplay RAM at fields 1900 and 2030 is identical between 0 and 300 apart from the packet and primitive buffers (and one timing word). Frame cost in the harness (fields/s from field 1900, uncapped): 0 27.2, 25 24.6, 50 21.0, 100 14.0, 200 14.7, 300 13.1, 1000 13.0. Gaps: the first-mission street ends near 300 percent, so 1000 shows no more than 300 there; the guest packet pool is fixed so dense scenes drop the farthest cells; object and actor culling is not scaled; the settings row was unit tested but not seen in a window; the native terrain pass differs from the guest at call 92 of the replay (see `world.cell-collector`) |

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
- **The title screen's palette is NOT dark versus the console — measured 2026-10-04, and the gap
  closes.** Same USA image, Beetle full-console reference with SCPH-1001 firmware, both captures at
  the settled title screen, compared over the same 512x240 window (the console framebuffer is 560
  wide, so its centre 512 is used): console mean RGB 42.9/17.8/12.8, peak 248/248/248, 52.0% non-black,
  943 distinct colours; port mean RGB 41.8/15.9/11.6, peak 248/248/248, 43.5% non-black, 910 distinct.
  The means agree within 3% and the peak is identical. What made the port's capture LOOK dark is
  measured too: its non-black coverage is 86.9% of EVEN rows and 0.0% of ODD rows — the capture holds
  ONE INTERLACED FIELD, which the console's `weave` output combines and this product's 512x240 sink
  presents one field of at a time (the guest alternates `disp.y` 0 and 256). So the earlier "the title
  screen's palette is dark" reading was the field, not the palette. Semi-transparency (`0xC0` rect
  flag) and CLUT sampling for this title's `E1` texpages remain unmeasured on a gameplay screen, and
  the gameplay screen's colour is now compared (next item). Note that `E4` in this title's tables is the
  drawing-area bottom-right, not a drawing mask — an earlier reading of the same bytes as a mask was
  wrong.
- **The gameplay screen's palette matches the console too (2026-10-04).** On the screen NEW GAME now
  reaches — the guest's `GRISHAM'S LOG` page — both sides sampled on the same 8-pixel grid give the
  same colours: port dominant `002000` (1558 samples) / `000000` (314) / accent `086028` (29) /
  highlight `08B858` (13); console dominant `002000` (1546) / `000000` (481) / accent `086030` (30).
  The dominant colour is byte-identical and the accent differs by 8 in one channel, which is the
  15-bit-to-8-bit rounding of one step. Both pictures were opened and read side by side and carry the
  same text and the same `SCROLL UP / DOWN` and `X CONTINUE` prompts. Captures:
  `scratch/nga_a1/shot_newgame.ppm` and `scratch/cons_d2/console_ng_26.png`.
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
- **NEW GAME's stall is CLOSED at its cause (2026-10-04); the mechanism is recorded here because the
  corrected reading supersedes two earlier claims in it.** The word at `0x800F0758` (`gp+0xB48`,
  `gp = 0x800EFC10`) is this title's CD COMMAND-QUEUE DEPTH, and the wait `FUN_800582CC` polls is
  `while (depth != 0) { VSync; FUN_800573E4(); }`. Measured through the control channel, not
  inferred: at the title screen, BEFORE any menu input, the queue already held four entries
  `[0x0D, 0x02(arg 0x20000000), 0x05(arg 2), 0x0C]` with depth 4, read index 0, write index 4 and
  state `0x800F0778` = 0; NEW GAME only appended a fifth entry (`0x06`) and set `0x800F1B44=5`, and
  all of it stayed frozen over 20k presented fields. With depth non-zero and state 0 the pump's three
  branches (`depth==0` / `state==1` / `state==2`) issue nothing, and state 0 is written by exactly one
  instruction - the first of the queue's dispatcher `FUN_800577AC` - so no completion could set it
  back. **What the real game does**, measured on the full-console reference over 7,200 fields: the
  guest's own libcd response poll `FUN_800b41fc` is entered 1,706 times and the guest's own
  queue-completion callback `FUN_80057f7c` - which the title installs at libcd's second
  ready-callback slot `0x800EEEB8` - 13 times, leaving those same four commands retired
  (depth 0, read 3, write 4, state 2). **The cause is that the guest's completion lives inside the body
  this title declares as its CD command seam.** `FUN_800abd98` (0x800ABD98, `CdReadySync`) issues
  through `FUN_800b4ca8` (0x800B4CA8, the title's `cdCommandAddress`) and relies on that body's own
  poll to read the controller and call the registered callbacks; `CdSync` (`FUN_800ac008`) instead runs
  `FUN_800b4760` as separate guest code, which is why that path always worked. **The fix is
  `game/cd/cd_command_completion.cpp`**: an owner at the measured `0x800ABD98` that runs the guest's
  body - so the command is still issued by the guest through the framework's seam - and then performs
  the completion that body's poll would have performed, one call to the guest's CURRENT registered
  command-status callback with libcd's completion code 2 and the response buffer the framework
  published. The guest's own callback then sets the state word and decrements the depth. Nothing
  writes a guest RAM word to release the wait, and neither `FUN_800582CC` nor `FUN_800573E4` is
  touched. **Measured after**: depth 0 / read 3 / write 4 / state 2 at the title (the console's own
  values), master 6->3 instead of 6->2, the guest's `GRISHAM'S LOG` page and then a rendered mission
  briefing, read 9 / write 10 after the briefing is continued, 0 fallback and 0 faults throughout.
  Two earlier claims are withdrawn: the pending counter was never written by `FUN_800b4760`'s
  response copy (that PC was the framework's block-boundary `pc`, not the storing instruction -
  `PSXPORT_WWATCH` prints `Core::pc`, which for translated code is the block start), and "no store to
  `0x800F1B40..0x800F1B50`" was a gp-relative scan against the game's `gp` alone; libcd uses a
  different one (`0x800F0000`, so its `-0x1148(gp)` resolves to `0x800EEEB8`), which is how that scan
  missed writers it should have found.
  (Runs: `ngm_measure1` before, `nga_a1` after with its captures opened and inspected, `cons_tl1` and
  `cons_d1` for the console timeline, `fix_dbg3` for the owner's deliveries.)
- **SUPERSEDED 2026-10-04 by the NEW GAME item above — this is the SAME wait, on the same word.**
  What follows is the parked reading, kept for provenance, and two of its statements are now known
  to be wrong: the `0x800B47E8` write attribution (that PC is the framework's block-boundary `pc`,
  not the storing instruction) and the reading of `0x800F0758` as a streamer counter rather than the
  command queue's depth. It is not re-tested here because the credits screen is no longer reachable
  before the first scene.
- **Black screen after the credits screen is a guest-side spin (parked reading).** Deterministic
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
