#include "view_frustum.h"

#include "core.h"
#include "execution_control.h"
#include "game.h"
#include "guest_call.h"
#include "guest_widescreen_projection.h"
#include "native_dispatch.h"
#include "title_facts.h"

#include <lucent/log.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace c12 {

namespace {

constexpr std::string_view kOwnerName = "c12 view frustum";
constexpr std::uint32_t kV0 = 2;
constexpr std::uint32_t kV1 = 3;
constexpr std::uint32_t kStackPointer = 29;
constexpr std::uint32_t kScratchFrameBytes = 0x20;
constexpr std::uint32_t kScratchVectorOffset = 0x10;

struct CornerSign {
  std::int32_t x;
  std::int32_t y;
};

// Guest order: left-top, left-bottom, right-bottom, right-top.
constexpr std::array<CornerSign, kFrustumCornerCount> kCornerSigns = {{{-1, 1}, {-1, -1}, {1, -1}, {1, 1}}};

} // namespace

std::int32_t widenedFrustumHalfWidth(std::int32_t retailHalfWidth, const CanvasWidths &canvas) {
  if (canvas.native <= 0 || canvas.presentation <= canvas.native) {
    return retailHalfWidth;
  }
  const std::int64_t scaled = static_cast<std::int64_t>(retailHalfWidth) * canvas.presentation;
  return static_cast<std::int32_t>((scaled + canvas.native - 1) / canvas.native);
}

void installViewFrustumOverride(Game &game) {
  psx::cpu::installNativeOverride(
      game.core, kViewFrustumBuilderAddress, "c12::rebuildViewFrustumCorners", rebuildViewFrustumCorners);
  lucent::info("c12-wide", "installed view frustum owner at 0x{:08X}", kViewFrustumBuilderAddress);
}

void rebuildViewFrustumCorners(Core *host) {
  Core &core = *host;
  const GuestProjectionPlan &plan = core.game->guestDisplay.plan();
  const std::int32_t halfWidth =
      widenedFrustumHalfWidth(kRetailFrustumHalfWidth, {plan.nativeExtent.width, plan.presentationExtent.width});
  const std::int32_t distance = core.mem_r16s(kZoneProjectionDistanceAddress + 2);
  const std::int32_t farDepth = static_cast<std::int32_t>(core.mem_r32(kZoneFrustumReachAddress));

  const std::uint32_t savedSp = core.r[kStackPointer];
  const std::uint32_t frame = savedSp - kScratchFrameBytes;
  const std::uint32_t vector = frame + kScratchVectorOffset;
  core.r[kStackPointer] = frame;
  for (std::uint32_t corner = 0; corner < kFrustumCornerCount; ++corner) {
    core.mem_w32(vector, static_cast<std::uint32_t>(kCornerSigns[corner].x * halfWidth));
    core.mem_w32(vector + 4, static_cast<std::uint32_t>(kCornerSigns[corner].y * kRetailFrustumHalfHeight));
    core.mem_w32(vector + 8, static_cast<std::uint32_t>(distance));
    psx::cpu::callGuestNow(core, kOwnerName, kVectorNormalizeAddress, vector, vector);
    const std::int32_t x = static_cast<std::int32_t>(core.mem_r32(vector));
    const std::int32_t y = static_cast<std::int32_t>(core.mem_r32(vector + 4));
    const std::int32_t z = static_cast<std::int32_t>(core.mem_r32(vector + 8));
    const std::uint32_t entry = kViewFrustumCornerTableAddress + corner * kFrustumCornerStride;
    if (z == 0) {
      lucent::error("c12-wide", "view frustum corner {} normalized to a zero depth", corner);
      std::abort();
    }
    const std::int32_t reach = farDepth / z;
    core.r[kV1] = static_cast<std::uint32_t>(reach);
    core.mem_w16(entry, static_cast<std::uint16_t>((x * reach) >> 16));
    core.mem_w16(entry + 2, static_cast<std::uint16_t>((y * reach) >> 16));
    core.mem_w16(entry + 4, static_cast<std::uint16_t>((z * reach) >> 16));
  }
  core.r[kStackPointer] = savedSp;
  // The guest loop leaves its exit test and last quotient in v0/v1.
  core.r[kV0] = 0;
}

} // namespace c12
