#include "enhancements.h"

namespace c12 {
namespace {

psx::config::BoolVar
    cvWidescreen("PSXPORT_C12_WIDESCREEN",
                 true,
                 "pc_enh: 16:9 canvas showing more world, not a stretch (suppressed in comparison runs)",
                 /*persistable=*/true);

} // namespace

psx::config::BoolVar &widescreenCvar() {
  return cvWidescreen;
}

} // namespace c12
