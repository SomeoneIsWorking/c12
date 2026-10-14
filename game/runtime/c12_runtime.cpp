#include "c12_runtime.h"

#include "c12_frame_driver.h"
#include "c12_session.h"
#include "cd_command_completion.h"
#include "cell_collector.h"
#include "enhancements.h"
#include "title_facts.h"
#include "view_frustum.h"
#include "world_mesh_pass.h"

#include "core.h"
#include "game.h"

#include <lucent/log.h>

namespace c12 {

// Only CD_cw is stock; CdReadySync is the title's own completion owner (cd_command_completion).
C12Runtime::C12Runtime()
    : vsync_(kVSyncAddress, kVSyncQueryCounterAddress),
      libcd_({.command = kCdCommandAddress},
             {.lastPositionAddress = kCdLastPositionAddress, .lastModeAddress = kCdLastModeAddress},
             kCdReadyCallbackSlotAddress,
             {.status = kCdReadyStatus}) {
}

void C12Runtime::registerOverrides(Game &game) {
  vsync_.bind(game);
  libcd_.bind(game);
  installTitleOverrides(game);
  installViewFrustumOverride(game);
  installCellCollectorOverride(game);
  installWorldMeshPassOverride(game);
}

std::unique_ptr<psx::game::TitleSessionState> C12Runtime::createSession(Game &) {
  return std::make_unique<C12Session>();
}

std::unique_ptr<psx::game::FrameDriver> C12Runtime::createFrameDriver(Game &game) {
  return std::make_unique<C12FrameDriver>(game);
}

GuestAddressRange C12Runtime::guestCodeModuleWindow() const {
  return kGuestCodeModuleWindow;
}

std::optional<psx::gpu::PacketPoolExtent> C12Runtime::packetPool(Core &core) const {
  return kPacketPoolWindows.extent(core);
}

RenderCapabilities C12Runtime::renderCapabilities() const {
  // The picture is the guest's GP0 output replayed from the frame record; no native producers.
  return RenderCapabilities{
      .defaultPath = RenderPath::Record,
      .nativeRenderPath = false,
  };
}

bool C12Runtime::guestVramIsPicture(const Game &) const {
  // No native producer: the guest's GP0 stream is the whole picture.
  return true;
}

const GuestWidescreenProjection *C12Runtime::guestWidescreenProjection() const {
  return &widescreenPolicy_;
}

void C12Runtime::reportRun(Core &core) const {
  const auto *driver =
      core.game != nullptr ? dynamic_cast<const C12FrameDriver *>(core.game->frameDriver.get()) : nullptr;
  if (driver == nullptr) {
    return;
  }
  lucent::info(
      "c12.fields", "presented {} field(s) across {} guest VSync boundar(ies)", driver->fields(), driver->boundaries());
}

std::span<const psx::game::TitleIntSetting> C12Runtime::titleIntSettings() const {
  return c12::titleIntSettings();
}

} // namespace c12
