#pragma once

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

const PlatformHlePlan &platformHlePlan();

} // namespace c12
