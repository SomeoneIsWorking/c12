// guest_field_loop.h — the C-12 display-field owner.
//
// C-12 is a guest-loop title: its own `main` owns the simulation and calls libetc `VSync`, which the
// platform HLE turns into a typed frame-boundary exit. The HOST still owns display time, so this
// class owns the one thing the guest cannot: a finite field step. Each step runs the guest until its
// VSync boundary, resumes the continuation the guest left in `r31`, and crosses the framework's one
// presentation fence — exactly once, after the field the guest just finished.
//
// WHY IT IS NOT THE PROBE'S LOOP. `c12_boot_probe` paced a field and executed a turn, but never
// committed a fence, so the neutral FramePresenter accumulated every field's prims until it refused
// (`FramePresenter::capture OVERFLOW: 65524 captured + 23 this flush > RQ_MAX 65536`). That overflow
// was this missing fence, not a guest that draws too much. The commit is the product's field
// boundary; presenting here is what makes the run a picture rather than a ledger.
#pragma once

#include <cstdint>

class Core;
class Game;

namespace c12 {

class GuestFieldLoop {
public:
  // The measured per-turn cycle budget. Bounded so a guest that stops taking its VSync boundary is a
  // reported stall, not a hung product.
  static constexpr std::uint64_t kCyclesPerTurn = 4'000'000;

  GuestFieldLoop(Game &game, Core &core);

  // One display field. Returns false when the run must stop.
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