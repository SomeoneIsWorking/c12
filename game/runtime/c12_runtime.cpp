#include "c12_runtime.h"

#include "title_facts.h"

namespace c12 {

C12Runtime::C12Runtime(const psx::cpu::PsxExeImage &header) {
  image_.residentText = header.physicalText;
  image_.crt0Entry = header.entry;
  image_.globalPointer = header.globalPointer;
}

void *C12Runtime::createContext(Core &) {
  return nullptr;
}

void C12Runtime::destroyContext(void *) {
}
void C12Runtime::registerOverrides(Game &) {
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
  return RenderCapabilities::direct();
}

bool C12Runtime::guestVramIsPicture(const Game &) const {
  // Measured: this title has no native picture producer. Its own ordering tables feed the guest's
  // GP0 packets, the framework's rasterizer draws them into guest VRAM, and nothing else writes the
  // frame, so guest VRAM is the picture.
  return true;
}

} // namespace c12