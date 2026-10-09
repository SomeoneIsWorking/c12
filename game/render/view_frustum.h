// view_frustum.h — the corner rays of the world view frustum (guest FUN_800660D8) widened with the canvas.
//
// The corners feed the guest's visible-cell walk (FUN_80066234) and its ground-plane footprint
// (FUN_800684C0); evidence in docs/re-frontier.md (world.frustum).

#pragma once

#include <cstdint>

class Core;
class Game;

namespace c12 {

// Retail half extent of the frustum at the projection plane: a 320x240 window.
inline constexpr std::int32_t kRetailFrustumHalfWidth = 0xA0;
inline constexpr std::int32_t kRetailFrustumHalfHeight = 0x78;

// Native and presented picture widths in guest columns.
struct CanvasWidths {
  std::int32_t native;
  std::int32_t presentation;
};

// The retail half width scaled by how much wider than native the presented picture is, rounded up so the
// frustum never undershoots the canvas. Equal widths return the retail value.
std::int32_t widenedFrustumHalfWidth(std::int32_t retailHalfWidth, const CanvasWidths &canvas);

// Replaces the guest's corner builder; at a 4:3 plan it computes exactly what the guest body does.
void installViewFrustumOverride(Game &game);

void rebuildViewFrustumCorners(Core *core);

} // namespace c12
