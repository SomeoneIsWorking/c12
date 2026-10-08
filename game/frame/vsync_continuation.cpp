#include "vsync_continuation.h"

#include "core.h"
#include "image_identity.h"

#include <lucent/log.h>

namespace c12 {

bool resumeVsyncContinuation(Core &core, std::uint32_t boundaryPc) {
  const std::uint32_t continuation = core.r[31];
  if ((continuation & 3u) != 0 || !core.currentImageIdentity(continuation)) {
    lucent::error("c12.fields",
                  "VSync left an unaligned or unauthenticated continuation 0x{:08x} (boundary PC 0x{:08x})",
                  continuation,
                  boundaryPc);
    return false;
  }
  core.pc = continuation;
  return true;
}

} // namespace c12