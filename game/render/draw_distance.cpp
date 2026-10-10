#include "draw_distance.h"

#include "config_vars.h"
#include "enhancements.h"

namespace c12 {

DrawDistance currentDrawDistance() {
  return DrawDistance(psx::config::enh_int(drawDistanceCvar()));
}

} // namespace c12
