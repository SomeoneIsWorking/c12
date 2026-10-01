#pragma once

#include "guest_cd_stream_callback_layout.h"
#include "guest_program_image.h"
#include "platform_hle.h"

#include <cstdint>

namespace c12 {

// The linked libetc VSync entry recorded by the pre-migration C-12 run. The direct runtime
// publishes this title fact to psxport; the framework supplies only the typed frame-boundary exit.
// Its body returns the vs-count global for negative queries: zeroed by libetc VSync setup at
// 0x800B0DF8 and incremented by the hardware VSync callback dispatcher at 0x800B0E50.
inline constexpr std::uint32_t kVSyncAddress = 0x800A1758u;
inline constexpr std::uint32_t kVSyncQueryCounterAddress = 0x800EEB98u;
inline constexpr std::uint32_t kCdCommandAddress = 0x800B4CA8u;
inline constexpr std::uint32_t kCdLastPositionAddress = 0x800EEED0u;
inline constexpr std::uint32_t kCdLastModeAddress = 0x800EEED4u;

// The guest RAM slot the linked libcd's CdReadyCallback writes: `FUN_800ac158` stores its argument
// to `DAT_800eeebc` and returns the previous value. `FUN_800af7ec` installs the title's sector
// callback `0x800AF4A8` there and `FUN_800af4a8` restores the saved value, so this is the stock
// ready-callback pointer the BIOS CD-ROM interrupt handler calls.
inline constexpr std::uint32_t kCdReadyCallbackSlotAddress = 0x800EEEBCu;

// `FUN_800af4a8` branches on its status argument for the data-ready case (`param_1 == '\x01'`),
// which is libcd's sector-ready completion code.
inline constexpr std::uint8_t kCdReadyStatus = 1u;

inline constexpr PlatformHlePlan kPlatformHlePlan = [] {
  PlatformHlePlan plan{};
  // Keep the admitted window exact until the linked library's complete body extent is recovered.
  // Registration only needs the measured entry; accepting neighboring C-12 code would turn an
  // incomplete fact into an accidental HLE dispatch.
  plan.vsyncAddress = kVSyncAddress;
  plan.vsyncQueryCounterAddress = kVSyncQueryCounterAddress;
  plan.windowLo[0] = kVSyncAddress;
  plan.windowHi[0] = kVSyncAddress + 4u;
  plan.cdCommandAddress = kCdCommandAddress;
  plan.windowLo[1] = kCdCommandAddress;
  plan.windowHi[1] = kCdCommandAddress + 4u;
  plan.stockCdWorkArea = {kCdLastPositionAddress, kCdLastModeAddress};
  return plan;
}();

// The title's own RAM arena, between the resident image and its stack: the guest's own `InitHeap`
// reports base `0x80105E40` (size `0xF81B4`), and this executable's header puts the stack base at
// `0x801FFFF0`. That is where the loader allocates the relocatable modules it reads from the disc:
// `FUN_80059108` opens `RELOCS_GT_LVB`, allocates, relocates the image with `FUN_800615F4`, and
// calls the module entry. Measured: 8 sectors of `RELOCS/GT.LVB` DMA3 into `0x8011F9BC..0x801239BC`
// and the entry dispatched at `0x8011FA64` (blob + 0xA8) — ABOVE the guest's declared heap end, so
// the window is the arena and not the heap alone.
inline constexpr GuestAddressRange kModuleArena = {0x00105E40u, 0x001FFFF0u};
inline constexpr GuestAddressRange kGuestCodeModuleWindow = kModuleArena;

const PlatformHlePlan &platformHlePlan();

inline constexpr GuestCdStreamCallbackLayout kCdStreamCallbackLayout = [] {
  GuestCdStreamCallbackLayout layout{};
  layout.readyCallbackPointer = kCdReadyCallbackSlotAddress;
  // The title reads through stock libcd: `FUN_800abed4(6,0)` sends CdlRead to the controller and
  // the per-sector completion arrives as INT1, exactly as the linked CdReadyCallback contract says.
  layout.owner = GuestCdStreamCallbackLayout::DeliveryOwner::GuestInterrupt;
  layout.readyStatus = kCdReadyStatus;
  return layout;
}();

} // namespace c12
