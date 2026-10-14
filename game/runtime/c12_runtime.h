#pragma once

#include "game_runtime.h"
#include "stock_libcd.h"
#include "stock_vsync.h"
#include "widescreen_policy.h"

namespace c12 {

// Runs the guest startup unmodified from the executable entry; C12FrameDriver owns each field.
class C12Runtime final : public psx::game::GameRuntime {
public:
  C12Runtime();
  void registerOverrides(Game &game) override;
  std::unique_ptr<psx::game::TitleSessionState> createSession(Game &game) override;
  std::unique_ptr<psx::game::FrameDriver> createFrameDriver(Game &game) override;
  GuestAddressRange guestCodeModuleWindow() const override;
  std::optional<psx::gpu::PacketPoolExtent> packetPool(Core &core) const override;
  RenderCapabilities renderCapabilities() const override;
  bool guestVramIsPicture(const Game &game) const override;
  const GuestWidescreenProjection *guestWidescreenProjection() const override;
  void reportRun(Core &core) const override;
  std::span<const psx::game::TitleIntSetting> titleIntSettings() const override;

private:
  psx::hle::StockVSync vsync_;
  psx::cd::GuestInterruptLibcd libcd_;
  WidescreenPolicy widescreenPolicy_;
};

} // namespace c12
