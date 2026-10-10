#pragma once

#include "config_var.h"
#include "title_settings.h"

#include <span>

namespace c12 {

// pc_enh knobs; read them through psx::config::enh / enh_int so comparison runs suppress them.
psx::config::BoolVar &widescreenCvar();  // PSXPORT_C12_WIDESCREEN
psx::config::IntVar &drawDistanceCvar(); // PSXPORT_C12_DRAW_DISTANCE: percent increase, 0 is retail

// The integer settings the title offers in the settings menu.
std::span<const TitleIntSetting> titleIntSettings();

} // namespace c12
