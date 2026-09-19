# 0004 — the overlay-owner test was written against a type that does not exist

Status: open

## What was there

An uncommitted change in this worktree removed the direct runtime's stock CD command binding
(`plan.cdCommandAddress`, `windowLo/Hi[1]`, `plan.stockCdWorkArea`) from `kPlatformHlePlan`, because
C-12's libcd is threaded and drives the emulated controller through the guest body itself, so the
title must not bind the synchronous shared command owner. That part is complete and is the change
still present in `game/runtime/c12_platform_facts.h`.

The same change also replaced the CD work-area tests in `tests/test_runtime_services.cpp` with a new
`test_c12_overlay_owner_activates_and_replaces_blocks`, and swapped the fixture's CD constants for
block-registrar/module ones.

## Why it could not land

It never compiled. `clang++` reported, against the tree as found:

    tests/test_runtime_services.cpp:111:35: error: no type named 'OverlayOwner' in namespace 'c12'
    tests/test_runtime_services.cpp:109:34: error: member access into incomplete type
                                            'psx::cpu::NativeDispatcher'

`c12::OverlayOwner` does not exist anywhere in this repository — not in `game/`, not in `tests/`. The
test was written against an owner that was never implemented, and `core.nativeDispatcher()` needs the
`NativeDispatcher` definition, which `core.h` only forward-declares. So this was work in flight, not
work waiting to be committed.

## What the test asserted

Recovered verbatim from the tree before it was lost (see the note below); the tail past the last
line here was not captured:

```cpp
void test_c12_overlay_owner_activates_and_replaces_blocks() {
  psx::cpu::PsxExeImage image{};
  c12::C12Runtime runtime(image);
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  auto &core = game->core;
  core.imageCatalog().activate("test-resident", {0x10000u, 0xF0E00u}, 1u);
  const auto registrarRange = core.currentImageIdentity(c12::test::kBlockRegistrarAddress);
  require(registrarRange.has_value(), "resident fixture image does not own the block registrar");
  runtime.registerOverrides(*game);
  require(core.nativeDispatcher().isInstalled({*registrarRange, c12::test::kBlockRegistrarAddress}),
          "C-12 did not install the block-registrar override");
  auto &owner = *static_cast<c12::OverlayOwner *>(core.gameCtx);

  for (std::uint32_t offset = 0; offset < kModuleSize; offset += 4u) {
    core.mem_w32(kModuleBase + offset, 0x11223344u + offset);
  }
  owner.activateRegisteredBlock(core, kModuleBase, kModuleSize);
  require(owner.placementCount() == 1, "block activation did not record exactly one placement");
  auto first = core.currentImageIdentity(kModuleBase + 8u);
  require(first.has_value(), "registered block range does not resolve to an image");
  // ... then: a second activation of the same range must produce a NEW identity while the
  // placement count stays 1, and an out-of-range activation (0x801FFFF0, 0x1000) must not
  // activate at all. Those three requires were captured by grep, their surrounding code was not.
```

Fixture constants it used: `kBlockRegistrarAddress = 0x8009BE54u`, `kModuleBase = 0x8011F800u`,
`kModuleSize = 0x100u`, plus the retained `kAfterVSync = 0x800A7F90u` and `kQueriedFields = 3u`.

## How it was lost

While reviewing this worktree on 2026-09-19 I ran `git checkout -- tests/runtime_service_fixture.h
tests/test_runtime_services.cpp` intending to undo one cosmetic edit of my own, and instead discarded
the whole uncommitted test change. It was unstaged, so git holds no copy; the block above is
everything that had been printed into the review session. The `.o` under `build/c12-vsync/` predates
the change and does not contain it.

## Next step

Implement `c12::OverlayOwner` — the per-`Core` game context that registers overlay blocks, activates
them into the image catalog with a fresh identity per activation, and refuses an out-of-range block —
then rewrite the test against it, including the `#include` that completes `psx::cpu::NativeDispatcher`.
Do not restore the CD work-area tests: their binding is deliberately gone (issue 0002).
