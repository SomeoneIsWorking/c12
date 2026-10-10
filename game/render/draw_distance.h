// draw_distance.h - the increased draw distance (S022): one percentage that scales every distance the world pass
// culls and fades by.
//
// The knob holds the INCREASE: 0 is the retail distance, 100 doubles it. Evidence in docs/re-frontier.md
// (world.distance).

#pragma once

#include "stepped_range.h"

#include <cstdint>

namespace c12 {

inline constexpr psx::config::SteppedRange kDrawDistanceRange{0, 1000, 25};
inline constexpr long kDrawDistanceDefaultPercent = 50;

// The increase, snapped onto the slider's grid. Zero is the retail distance.
class DrawDistance {
public:
  explicit DrawDistance(long percent) : percent_(kDrawDistanceRange.clamp(percent)) {
  }

  long percent() const {
    return percent_;
  }

  bool increased() const {
    return percent_ > 0;
  }

  // `retail` grown by the increase; the retail value itself at zero.
  std::int64_t scale(std::int64_t retail) const {
    return retail * (100 + percent_) / 100;
  }

  // Like scale, rounded up: the size of a table stretched by the increase.
  std::int64_t scaleUp(std::int64_t retail) const {
    return (retail * (100 + percent_) + 99) / 100;
  }

  // The retail value a grown one stands for, rounded down.
  std::int64_t unscale(std::int64_t grown) const {
    return grown * 100 / (100 + percent_);
  }

private:
  long percent_;
};

// The live increase; zero in a comparison run.
DrawDistance currentDrawDistance();

} // namespace c12
