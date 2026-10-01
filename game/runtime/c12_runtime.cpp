#include "c12_runtime.h"

#include "c12_platform_facts.h"

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
  return &c12::platformHlePlan();
}

const GuestCdStreamCallbackLayout *C12Runtime::guestCdStreamCallbackLayout() const {
  return &c12::kCdStreamCallbackLayout;
}

GuestAddressRange C12Runtime::guestCodeModuleWindow() const {
  return c12::kGuestCodeModuleWindow;
}

const GuestPacketPoolWindows *C12Runtime::guestPacketPoolWindows() const {
  return &c12::kPacketPoolWindows;
}

RenderCapabilities C12Runtime::renderCapabilities() const {
  return RenderCapabilities::direct();
}

bool C12Runtime::guestVramIsPicture(const Game &) const {
  // MEASURED: this title has no native picture producer. Its own ordering tables feed the guest's
  // GP0 packets, the framework's rasterizer draws them into guest VRAM, and nothing else writes the
  // frame — at field 1400 a direct capture of the declared display rect (0,256) 512x240 held real
  // picture while the presented frame was 0/691,200 non-black, because the present policy was told
  // the guest's VRAM was not part of the picture and so never composited it.
  return true;
}

} // namespace c12
