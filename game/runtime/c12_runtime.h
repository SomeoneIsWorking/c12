#pragma once

#include "game_runtime.h"
#include "psx_exe_image.h"

namespace c12 {

// Runs the guest startup unmodified; no native CRT0 or frame driver.
class C12Runtime final : public GameRuntime {
public:
  explicit C12Runtime(const psx::cpu::PsxExeImage &header);
  void *createContext(Core &) override;
  void destroyContext(void *) override;
  void registerOverrides(Game &) override;
  void bootInit(Core &) override;
  const GuestProgramImage *guestProgramImage() const override;
  const PlatformHlePlan *platformHlePlan() const override;
  const GuestCdStreamCallbackLayout *guestCdStreamCallbackLayout() const override;
  GuestAddressRange guestCodeModuleWindow() const override;
  const GuestPacketPoolWindows *guestPacketPoolWindows() const override;
  RenderCapabilities renderCapabilities() const override;
  bool guestVramIsPicture(const Game &) const override;

private:
  GuestProgramImage image_;
};

} // namespace c12
