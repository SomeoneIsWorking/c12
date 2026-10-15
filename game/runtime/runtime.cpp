#include "runtime/runtime.h"

#include "cd/cd_command_completion.h"
#include "facts/title_facts.h"
#include "frame/frame_driver.h"
#include "render/cell_collector.h"
#include "render/view_frustum.h"
#include "render/world_mesh_pass.h"
#include "runtime/session.h"
#include "title/enhancements.h"

#include "core.h"
#include "game.h"

#include <lucent/log.h>

namespace c12 {

// Only CD_cw is stock; CdReadySync is the title's own completion owner (cd_command_completion).
Runtime::Runtime()
    : vsync_(kVSyncAddress, kVSyncQueryCounterAddress),
      libcd_({.command = kCdCommandAddress},
             {.lastPositionAddress = kCdLastPositionAddress, .lastModeAddress = kCdLastModeAddress},
             kCdReadyCallbackSlotAddress,
             {.status = kCdReadyStatus}) {
}

void Runtime::registerOverrides(Game &game) {
  vsync_.bind(game);
  libcd_.bind(game);
  installTitleOverrides(game);
  installViewFrustumOverride(game);
  installCellCollectorOverride(game);
  installWorldMeshPassOverride(game);
}

std::unique_ptr<psx::game::TitleSessionState> Runtime::createSession(Game &) {
  return std::make_unique<Session>();
}

std::unique_ptr<psx::game::FrameDriver> Runtime::createFrameDriver(Game &game) {
  return std::make_unique<FrameDriver>(game);
}

GuestAddressRange Runtime::guestCodeModuleWindow() const {
  return kGuestCodeModuleWindow;
}

std::optional<psx::gpu::PacketPoolExtent> Runtime::packetPool(Core &core) const {
  return kPacketPoolWindows.extent(core);
}

RenderCapabilities Runtime::renderCapabilities() const {
  // The picture is the guest's GP0 output replayed from the frame record; no native producers.
  return RenderCapabilities{
      .defaultPath = RenderPath::Record,
      .nativeRenderPath = false,
  };
}

bool Runtime::guestVramIsPicture(const Game &) const {
  // No native producer: the guest's GP0 stream is the whole picture.
  return true;
}

const GuestWidescreenProjection *Runtime::guestWidescreenProjection() const {
  return &widescreenPolicy_;
}

void Runtime::reportRun(Core &core) const {
  const auto *driver = core.game != nullptr ? dynamic_cast<const FrameDriver *>(core.game->frameDriver.get()) : nullptr;
  if (driver == nullptr) {
    return;
  }
  lucent::info(
      "c12.fields", "presented {} field(s) across {} guest VSync boundar(ies)", driver->fields(), driver->boundaries());
}

std::span<const psx::game::TitleIntSetting> Runtime::titleIntSettings() const {
  return c12::titleIntSettings();
}

} // namespace c12
