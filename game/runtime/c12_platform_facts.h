#pragma once

#include <cstdint>

struct PlatformHlePlan;

namespace c12 {

// The linked libetc VSync entry recorded by the pre-migration C-12 run. The direct runtime
// publishes this title fact to psxport; the framework supplies only the typed frame-boundary exit.
inline constexpr std::uint32_t kVSyncAddress = 0x800A1758u;

const PlatformHlePlan &platformHlePlan();

} // namespace c12
