#pragma once

#include "c12_platform_facts.h"
#include "game_runtime.h"
#include "platform_hle.h"

#include <array>

namespace c12::test {

inline constexpr std::uint32_t kAfterVSync = 0x800A7F90u;
inline constexpr std::uint32_t kParameterAddress = 0x80110000u;
inline constexpr std::uint32_t kQueriedFields = 3u;
inline constexpr std::uint8_t kSentinel = 0xA5u;
inline constexpr std::uint8_t kMode = 0xE0u;
inline constexpr std::array<std::uint8_t, 4> kPosition{0x00u, 0x02u, 0x16u, 0x01u};

// Publishes the real title binding with no declared guest work area. This exercises the shared
// handler's negative contract without mutating the immutable production plan.
class UndeclaredCdWorkAreaRuntime final : public GameRuntime {
public:
  UndeclaredCdWorkAreaRuntime() : plan_(c12::platformHlePlan()) {
    plan_.stockCdWorkArea = {};
  }
  void *createContext(Core &) override {
    return nullptr;
  }
  void destroyContext(void *) override {
  }
  void registerOverrides(Game &) override {
  }
  void bootInit(Core &) override {
  }
  RenderCapabilities renderCapabilities() const override {
    return RenderCapabilities::direct();
  }
  bool guestVramIsPicture(const Game &) const override {
    return false;
  }
  const PlatformHlePlan *platformHlePlan() const override {
    return &plan_;
  }

private:
  PlatformHlePlan plan_;
};

} // namespace c12::test
