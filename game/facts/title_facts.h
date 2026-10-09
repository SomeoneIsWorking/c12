// title_facts.h — measured guest facts C-12 publishes to psxport.
//
// Evidence in docs/re-frontier.md and docs/project-state.md.

#pragma once

#include "guest_cd_stream_callback_layout.h"
#include "guest_packet_pool_windows.h"
#include "guest_program_image.h"
#include "platform_hle.h"

#include <cstdint>

namespace c12 {

// libetc VSync entry and vs-count, libcd CD command entry, title's CdReadySync wrapper.
inline constexpr std::uint32_t kVSyncAddress = 0x800A1758u;
inline constexpr std::uint32_t kVSyncQueryCounterAddress = 0x800EEB98u;
inline constexpr std::uint32_t kCdCommandAddress = 0x800B4CA8u;
inline constexpr std::uint32_t kCdReadySyncAddress = 0x800ABD98u;

// libcd work area written by CdLastPos: position, then mode.
inline constexpr std::uint32_t kCdLastPositionAddress = 0x800EEED0u;
inline constexpr std::uint32_t kCdLastModeAddress = 0x800EEED4u;

// libcd CdReadyCallback slot and data-ready status.
inline constexpr std::uint32_t kCdReadyCallbackSlotAddress = 0x800EEEBCu;
inline constexpr std::uint8_t kCdReadyStatus = 1u;

// Command-status callback slot; the guest installs FUN_80057f7c, which sets 0x800F0778 and decrements 0x800F0758.
inline constexpr std::uint32_t kCdCommandStatusCallbackSlotAddress = 0x800EEEB8u;

// VSync and CD command entries only, as one-instruction windows. CdReadySync is a native override instead.
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

// InitHeap base to stack base; the loader places relocatable modules here (GT.LVB at 0x8011F9BC).
inline constexpr GuestAddressRange kGuestCodeModuleWindow = {0x00105E40u, 0x001FFFF0u};

// Sectors are read through stock libcd; completion comes from the INT1 handler.
inline constexpr GuestCdStreamCallbackLayout kCdStreamCallbackLayout = [] {
  GuestCdStreamCallbackLayout layout{};
  layout.readyCallbackPointer = kCdReadyCallbackSlotAddress;
  layout.owner = GuestCdStreamCallbackLayout::DeliveryOwner::GuestInterrupt;
  layout.readyStatus = kCdReadyStatus;
  return layout;
}();

// One window holding both parity OTs (0x801AFF98, 0x801B0F98) and their nodes from 0x800E3DE4.
inline constexpr std::uint32_t kPacketPoolLow = 0x800E3DE4u;
inline constexpr std::uint32_t kPacketPoolHigh = 0x801B0F9Cu;
inline constexpr GuestPacketPoolWindows kPacketPoolWindows = [] {
  GuestPacketPoolWindows windows{};
  windows.representation = GuestPacketPoolWindows::Representation::SingleWindow;
  windows.base = kPacketPoolLow;
  windows.end = kPacketPoolHigh;
  return windows;
}();

// World view frustum corner builder, its table (four 8-byte s16 x, y, z entries) and the zone words it reads.
inline constexpr std::uint32_t kViewFrustumBuilderAddress = 0x800660D8u;
inline constexpr std::uint32_t kViewFrustumCornerTableAddress = 0x800F9378u;
inline constexpr std::uint32_t kFrustumCornerCount = 4u;
inline constexpr std::uint32_t kFrustumCornerStride = 8u;
// 16.16 words: projection distance H in the high half, frustum reach in the full word.
inline constexpr std::uint32_t kZoneProjectionDistanceAddress = 0x800F921Cu;
inline constexpr std::uint32_t kZoneFrustumReachAddress = 0x800F9220u;
// Normalizes the (x, y, z) word triple at a0, writing the result to a1.
inline constexpr std::uint32_t kVectorNormalizeAddress = 0x800A1F10u;

// World terrain polygon pass and the guest words it reads; the header is the zone/visibility state block.
inline constexpr std::uint32_t kWorldMeshPassAddress = 0x8006769Cu;
inline constexpr std::uint32_t kWorldHeaderAddress = 0x800F91E4u;
inline constexpr std::uint32_t kWorldAttributeTableAddress = 0x800F08E0u;
inline constexpr std::uint32_t kWorldShadeTablesAddress = 0x800F9E00u;
inline constexpr std::uint32_t kWorldPassSaveAreaAddress = 0x800F9E10u;
// Subdivision tables in the executable's data: midpoint index pairs per shape, packet slot tables per shape and level.
inline constexpr std::uint32_t kQuadMidpointPairsAddress = 0x800D8728u;
inline constexpr std::uint32_t kTriMidpointPairsAddress = 0x800D875Au;
inline constexpr std::uint32_t kQuadCoarseSlotsAddress = 0x800D8774u;
inline constexpr std::uint32_t kQuadFineSlotsAddress = 0x800D87E0u;
inline constexpr std::uint32_t kTriCoarseSlotsAddress = 0x800D890Cu;
inline constexpr std::uint32_t kTriFineSlotsAddress = 0x800D8954u;

} // namespace c12