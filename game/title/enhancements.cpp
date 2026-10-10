#include "enhancements.h"

#include "draw_distance.h"

#include <array>

namespace c12 {
namespace {

psx::config::BoolVar
    cvWidescreen("PSXPORT_C12_WIDESCREEN",
                 true,
                 "pc_enh: 16:9 canvas showing more world, not a stretch (suppressed in comparison runs)",
                 /*persistable=*/true);

psx::config::IntVar cvDrawDistance("PSXPORT_C12_DRAW_DISTANCE",
                                   kDrawDistanceDefaultPercent,
                                   "pc_enh: percent increase of the world draw distance and its fog, 0..1000 in steps "
                                   "of 25, 0 is retail (suppressed in comparison runs)",
                                   /*persistable=*/true);

const std::array<TitleIntSetting, 1> kTitleIntSettings{{
    {"draw_distance", "Draw Distance", "%", &cvDrawDistance, kDrawDistanceRange},
}};

} // namespace

psx::config::BoolVar &widescreenCvar() {
  return cvWidescreen;
}

psx::config::IntVar &drawDistanceCvar() {
  return cvDrawDistance;
}

std::span<const TitleIntSetting> titleIntSettings() {
  return kTitleIntSettings;
}

} // namespace c12
