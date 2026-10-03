// title_facts.h — the measured guest facts C-12 publishes to psxport.
//
// Every value here was recovered from the authenticated USA executable or measured in a run of it;
// the evidence lives in `docs/re-frontier.md` and `docs/project-state.md`. The framework owns the
// behavior these facts select; the title owns only the values.

#pragma once

#include "guest_cd_stream_callback_layout.h"
#include "guest_packet_pool_windows.h"
#include "guest_program_image.h"
#include "platform_hle.h"

#include <cstdint>

namespace c12 {

// The linked libetc VSync entry, the vs-count global its setup zeroes and the hardware VSync
// callback dispatcher increments, and the linked libcd CD command entry.
inline constexpr std::uint32_t kVSyncAddress = 0x800A1758u;
inline constexpr std::uint32_t kVSyncQueryCounterAddress = 0x800EEB98u;
inline constexpr std::uint32_t kCdCommandAddress = 0x800B4CA8u;

// The stock libcd work area `CdLastPos` writes: position, then mode.
inline constexpr std::uint32_t kCdLastPositionAddress = 0x800EEED0u;
inline constexpr std::uint32_t kCdLastModeAddress = 0x800EEED4u;

// The guest RAM slot the linked libcd's `CdReadyCallback` pointer occupies, and libcd's
// data-ready completion status.
inline constexpr std::uint32_t kCdReadyCallbackSlotAddress = 0x800EEEBCu;
inline constexpr std::uint8_t kCdReadyStatus = 1u;

// Only the measured VSync and CD command entries are admitted, as one-instruction windows, so an
// unrecovered neighbouring C-12 address can never become an HLE dispatch by accident.
inline constexpr PlatformHlePlan kPlatformHlePlan = [] {
  PlatformHlePlan plan{};
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

// The title's own RAM arena, from its `InitHeap` base to the executable's stack base: where the
// loader allocates the relocatable modules it reads from the disc (`RELOCS/GT.LVB` lands at
// `0x8011F9BC`), which is above the declared heap end.
inline constexpr GuestAddressRange kGuestCodeModuleWindow = {0x00105E40u, 0x001FFFF0u};

// The title reads its sectors through stock libcd: `CdlRead` reaches the controller and each
// per-sector completion arrives as INT1, which is the guest's own delivery path.
inline constexpr GuestCdStreamCallbackLayout kCdStreamCallbackLayout = [] {
  GuestCdStreamCallbackLayout layout{};
  layout.readyCallbackPointer = kCdReadyCallbackSlotAddress;
  layout.owner = GuestCdStreamCallbackLayout::DeliveryOwner::GuestInterrupt;
  layout.readyStatus = kCdReadyStatus;
  return layout;
}();

// The 2D packet pool, measured over 3,000 fields: one window holding both parity ordering tables at
// its top (`0x801AFF98`, `0x801B0F98`) and every node they descend to (`0x800E3DE4`), just above the
// drawing environment the guest passes to `ResetGraph`. A store inside it is a packet's address, not
// proof of render data — the attribution limit is an open item in `docs/project-state.md`.
inline constexpr std::uint32_t kPacketPoolLow = 0x800E3DE4u;
inline constexpr std::uint32_t kPacketPoolHigh = 0x801B0F9Cu;
inline constexpr GuestPacketPoolWindows kPacketPoolWindows = [] {
  GuestPacketPoolWindows windows{};
  windows.representation = GuestPacketPoolWindows::Representation::SingleWindow;
  windows.base = kPacketPoolLow;
  windows.end = kPacketPoolHigh;
  return windows;
}();

} // namespace c12