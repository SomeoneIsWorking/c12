#include "c12_runtime.h"

namespace c12 {

C12Runtime::C12Runtime(const psx::cpu::PsxExeImage &header) {
  image_.residentText = header.physicalText;
  image_.crt0Entry = header.entry;
  image_.globalPointer = header.globalPointer;
}

void *C12Runtime::createContext(Core &) {
  return nullptr;
}

void C12Runtime::destroyContext(void *) {}
void C12Runtime::registerOverrides(Game &) {}
void C12Runtime::bootInit(Core &) {}

const GuestProgramImage *C12Runtime::guestProgramImage() const {
  return &image_;
}

RenderCapabilities C12Runtime::renderCapabilities() const {
  return RenderCapabilities::direct();
}

bool C12Runtime::guestVramIsPicture(const Game &) const {
  // This initial runtime is exercised by a non-presenting boot probe. No player
  // renderer or source-geometry reconstruction has been qualified yet.
  return false;
}

} // namespace c12
