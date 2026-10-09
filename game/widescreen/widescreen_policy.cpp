#include "widescreen_policy.h"

#include "config_vars.h"
#include "enhancements.h"

namespace c12 {

PresentationAspect WidescreenPolicy::presentationAspect(const Core &) const {
  return psx::config::enh(widescreenCvar()) ? PresentationAspect::Wide16x9 : PresentationAspect::Standard4x3;
}

} // namespace c12
