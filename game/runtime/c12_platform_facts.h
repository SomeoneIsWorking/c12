#pragma once

#include "guest_cd_stream_callback_layout.h"
#include "guest_packet_pool_windows.h"
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

inline constexpr GuestCdStreamCallbackLayout kCdStreamCallbackLayout = [] {
  GuestCdStreamCallbackLayout layout{};
  layout.readyCallbackPointer = kCdReadyCallbackSlotAddress;
  // The title reads through stock libcd: `FUN_800abed4(6,0)` sends CdlRead to the controller and
  // the per-sector completion arrives as INT1, exactly as the linked CdReadyCallback contract says.
  layout.owner = GuestCdStreamCallbackLayout::DeliveryOwner::GuestInterrupt;
  layout.readyStatus = kCdReadyStatus;
  return layout;
}();

// The 2D packet pool, measured over 3,000 fields (2,033 presented frames) of the authenticated
// startup: walking each parity ordering table from its head reaches the same lowest node every
// time, and no node ever appears above the second head. The parity heads are `0x801AFF98` and
// `0x801B0F98` — 0x10000 apart, inside the guest's own heap (`InitHeap` base `0x80105E40`, end
// `0x8017FFF4`) — while the packets descend from them down to `0x800E3DE4`, just above the drawing
// environment the guest passes to `ResetGraph` (`jtb=0x800E3CF0`). So the pool is ONE window with
// both ordering tables at its top, not two equal halves and not two reallocating pools, and it is
// declared as the measured extent it is.
//
// COARSENESS, stated rather than hidden: that lowest address is the same fixed node on every walk, so
// it is a static packet in the executable's data segment, and the band between it and the heads also
// holds the guest's own globals (`0x800EEEBC`, the heap cursor). The window is therefore the addresses
// packets were actually submitted from, which is what attribution needs, but a store inside it is not
// yet known to be render data. Recorded as an open item in docs/project-state.md.
inline constexpr std::uint32_t kPacketPoolLow = 0x800E3DE4u;
inline constexpr std::uint32_t kPacketPoolHigh = 0x801B0F9Cu;
inline constexpr GuestPacketPoolWindows kPacketPoolWindows = [] {
  GuestPacketPoolWindows windows{};
  windows.representation = GuestPacketPoolWindows::Representation::SingleWindow;
  windows.base = kPacketPoolLow;
  windows.end = kPacketPoolHigh;
  return windows;
}();

const PlatformHlePlan &platformHlePlan();

} // namespace c12
