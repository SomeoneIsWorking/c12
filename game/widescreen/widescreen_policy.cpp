#include "widescreen/widescreen_policy.h"

#include "enhancement_gate.h"
#include "title/enhancements.h"

namespace c12 {

PresentationAspect WidescreenPolicy::presentationAspect(const Core &) const {
  return psx::config::enh(widescreenCvar()) ? PresentationAspect::Wide16x9 : PresentationAspect::Standard4x3;
}

} // namespace c12
