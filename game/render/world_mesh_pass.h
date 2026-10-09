// world_mesh_pass.h — the world terrain polygon pass (guest FUN_8006769C), ported with a widened screen reject.
//
// The pass projects each visible cell's polygons and drops those entirely outside a fixed 512x256 window before it
// builds GT3/GT4 packets. Evidence in docs/re-frontier.md (world.polygon-pass).

#pragma once

#include <cstdint>
#include <span>

class Core;
class Game;

namespace c12 {

struct ScreenVertex {
  std::int16_t x = 0;
  std::int16_t y = 0;
};

// Retail trivial-reject window: columns 0..0x1FF and rows 0..0xFF.
inline constexpr std::int32_t kRetailWindowWidth = 0x200;
inline constexpr std::int32_t kRetailWindowHeight = 0x100;

// True when no vertex of the polygon can reach the window: all left of it or all above it, or none inside its
// right and bottom edges. `margin` extends the left and right edges by that many columns and is zero at 4:3.
bool polygonOutsideWindow(std::span<const ScreenVertex> vertices, std::int32_t margin);

// Guest columns the widened canvas adds on each side of the 512-column window, rounded up; zero when not wider.
std::int32_t widenedWindowMargin(std::int32_t nativeWidth, std::int32_t presentationWidth);

// Replaces the guest terrain polygon pass; at margin zero it produces the guest body's exact state.
void installWorldMeshPassOverride(Game &game);

void drawWorldMeshPass(Core *core);

} // namespace c12
