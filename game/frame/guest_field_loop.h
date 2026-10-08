// guest_field_loop.h — the C-12 display-field owner.

#pragma once

#include <cstdint>

class Core;
class Game;

namespace c12 {

class GuestFieldLoop {
public:
  static constexpr std::uint64_t kCyclesPerTurn = 4'000'000;

  GuestFieldLoop(Game &game, Core &core);

  // Steps fields until frameLimit (none when <= 0) or a turn that cannot resume.
  void run(int frameLimit);

  // One display field; false when run must stop.
  bool stepField();

  std::uint64_t fields() const {
    return fields_;
  }
  std::uint64_t boundaries() const {
    return boundaries_;
  }

private:
  Game &game_;
  Core &core_;
  std::uint64_t fields_ = 0;
  std::uint64_t boundaries_ = 0;
};

} // namespace c12