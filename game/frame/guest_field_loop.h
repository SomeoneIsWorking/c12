// guest_field_loop.h — the C-12 display-field owner.
//
// C-12 is a guest-loop title: its own `main` owns the simulation and calls libetc `VSync`, which the
// platform HLE turns into a typed frame-boundary exit. The host still owns display time, so this
// class owns the one thing the guest cannot: a finite field step and the single presentation fence
// that follows it. The startup probe does not use it — it observes turns without committing a
// frame.

#pragma once

#include <cstdint>

class Core;
class Game;

namespace c12 {

class GuestFieldLoop {
public:
  // The measured per-turn cycle budget, so a guest that stops taking its VSync boundary is a
  // reported stall rather than a hung product.
  static constexpr std::uint64_t kCyclesPerTurn = 4'000'000;

  GuestFieldLoop(Game &game, Core &core);

  // Steps fields until the frame limit (no limit when not positive) or a turn the host must not
  // resume. The control channel is serviced between fields, so it stays live for the whole run.
  void run(int frameLimit);

  // One display field. False when the run must stop.
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