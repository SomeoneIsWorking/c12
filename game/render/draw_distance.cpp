#include "render/draw_distance.h"

#include "enhancement_gate.h"
#include "title/enhancements.h"

namespace c12 {

DrawDistance currentDrawDistance() {
  return DrawDistance(psx::config::enh_int(drawDistanceCvar()));
}

} // namespace c12
