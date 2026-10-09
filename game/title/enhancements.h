#pragma once

#include "config_var.h"

namespace c12 {

// pc_enh knobs; read them through psx::config::enh so comparison runs suppress them.
psx::config::BoolVar &widescreenCvar(); // PSXPORT_C12_WIDESCREEN

} // namespace c12
