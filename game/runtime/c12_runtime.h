#pragma once

#include "game_runtime.h"
#include "widescreen_policy.h"

namespace c12 {

// Runs the guest startup unmodified from the executable entry; C12FrameDriver owns each field.
class C12Runtime final : public GameRuntime {
public:
  C12Runtime();
  void *createContext(Core &) override;
  void destroyContext(void *) override;
  void registerOverrides(Game &) override;
  std::span<const TitleIntSetting> titleIntSettings() const override;
  void bootInit(Core &) override;
  const GuestProgramImage *guestProgramImage() const override;
  const PlatformHlePlan *platformHlePlan() const override;
  const GuestCdStreamCallbackLayout *guestCdStreamCallbackLayout() const override;
  GuestAddressRange guestCodeModuleWindow() const override;
  const GuestPacketPoolWindows *guestPacketPoolWindows() const override;
  RenderCapabilities renderCapabilities() const override;
  bool guestVramIsPicture(const Game &) const override;
  const GuestWidescreenProjection *guestWidescreenProjection() const override;
  std::unique_ptr<FrameDriver> createFrameDriver(Game &game) override;
  void reportRun(Core &core) const override;

private:
  GuestProgramImage image_;
  WidescreenPolicy widescreenPolicy_;
};

} // namespace c12
