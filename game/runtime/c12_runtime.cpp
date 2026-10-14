#include "c12_runtime.h"

#include "c12_frame_driver.h"
#include "cd_command_completion.h"
#include "cell_collector.h"
#include "cell_output.h"
#include "enhancements.h"
#include "executable_identity.h"
#include "title_facts.h"
#include "view_frustum.h"
#include "world_mesh_pass.h"

#include "core.h"
#include "game.h"

#include <lucent/log.h>

namespace c12 {

C12Runtime::C12Runtime() {
  // The driver enters the guest at the executable's own entry; the framework applies no crt0.
  image_.bootEntry = BootEntry::DriverOwned;
  const std::uint32_t textPhysical = identity::kTextAddress & 0x1FFFFFFFu;
  image_.residentText = {textPhysical, textPhysical + identity::kTextBytes};
  image_.crt0Entry = identity::kEntry;
  image_.globalPointer = identity::kGlobalPointer;
}

void *C12Runtime::createContext(Core &) {
  return new WorldVisibility();
}

void C12Runtime::destroyContext(void *context) {
  delete static_cast<WorldVisibility *>(context);
}

std::span<const TitleIntSetting> C12Runtime::titleIntSettings() const {
  return c12::titleIntSettings();
}

void C12Runtime::registerOverrides(Game &game) {
  installTitleOverrides(game);
  installViewFrustumOverride(game);
  installCellCollectorOverride(game);
  installWorldMeshPassOverride(game);
}

void C12Runtime::bootInit(Core &) {
}

const GuestProgramImage *C12Runtime::guestProgramImage() const {
  return &image_;
}

const PlatformHlePlan *C12Runtime::platformHlePlan() const {
  return &kPlatformHlePlan;
}

const GuestCdStreamCallbackLayout *C12Runtime::guestCdStreamCallbackLayout() const {
  return &kCdStreamCallbackLayout;
}

GuestAddressRange C12Runtime::guestCodeModuleWindow() const {
  return kGuestCodeModuleWindow;
}

const GuestPacketPoolWindows *C12Runtime::guestPacketPoolWindows() const {
  return &kPacketPoolWindows;
}

RenderCapabilities C12Runtime::renderCapabilities() const {
  // The picture is the guest's GP0 output replayed from the frame record; no native producers, no interpolation yet.
  return RenderCapabilities{
      .defaultPath = RenderPath::Record,
      .nativeRenderPath = false,
      .temporalInterpolation = false,
  };
}

bool C12Runtime::guestVramIsPicture(const Game &) const {
  // No native producer: the guest's GP0 stream is the whole picture.
  return true;
}

const GuestWidescreenProjection *C12Runtime::guestWidescreenProjection() const {
  return &widescreenPolicy_;
}

std::unique_ptr<FrameDriver> C12Runtime::createFrameDriver(Game &game) {
  return std::make_unique<C12FrameDriver>(game);
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

} // namespace c12
