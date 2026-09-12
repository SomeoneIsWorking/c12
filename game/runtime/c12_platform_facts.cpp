#include "c12_platform_facts.h"

#include "platform_hle.h"

namespace c12 {
namespace {

const PlatformHlePlan kPlatformHlePlan = [] {
  PlatformHlePlan plan{};
  // Keep the admitted window exact until the linked library's complete body extent is recovered.
  // Registration only needs the measured entry; accepting neighboring C-12 code would turn an
  // incomplete fact into an accidental HLE dispatch.
  plan.vsyncAddress = kVSyncAddress;
  plan.windowLo[0] = kVSyncAddress;
  plan.windowHi[0] = kVSyncAddress + 4u;
  return plan;
}();

} // namespace

const PlatformHlePlan &platformHlePlan() {
  return kPlatformHlePlan;
}

} // namespace c12
