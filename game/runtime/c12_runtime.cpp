#include "c12_runtime.h"

#include "cd_command_completion.h"
#include "cell_collector.h"
#include "cell_output.h"
#include "enhancements.h"
#include "title_facts.h"
#include "view_frustum.h"
#include "world_mesh_pass.h"

namespace c12 {

C12Runtime::C12Runtime(const psx::cpu::PsxExeImage &header) {
  image_.residentText = header.physicalText;
  image_.crt0Entry = header.entry;
  image_.globalPointer = header.globalPointer;
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

} // namespace c12