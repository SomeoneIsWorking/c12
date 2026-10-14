// c12_frame_driver.h — the C-12 display-field owner.

#pragma once

#include "game_runtime.h"

#include <cstdint>

class Core;
class Game;

namespace c12 {

// One step is one display field: the logic-frame start (clock, producer census, pad), one bounded guest turn, the VSync
// continuation, one presentation commit, then audio.
class C12FrameDriver final : public psx::game::FrameDriver {
public:
  static constexpr std::uint64_t kCyclesPerTurn = 4'000'000;

  explicit C12FrameDriver(Game &game);

  void stepFrame(Core &core, std::uint32_t frame) override;

  std::uint64_t fields() const {
    return fields_;
  }
  std::uint64_t boundaries() const {
    return boundaries_;
  }

private:
  Game &game_;
  std::uint64_t fields_ = 0;
  std::uint64_t boundaries_ = 0;
};

} // namespace c12
