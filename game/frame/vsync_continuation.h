// vsync_continuation.h — the one rule for resuming a guest VSync continuation.

#pragma once

#include <cstdint>

class Core;

namespace c12 {

// The guest's libetc `VSync` leaves its return address in `r31` and the platform HLE exits the host
// turn there. That address may be resumed only when it is aligned and owned by a loaded image; the
// disc-loaded modules make an unowned address reachable, and the host must not dispatch guest code
// it has not authenticated. Sets `pc` and returns true when the continuation is legal, and reports
// the refusal and returns false otherwise — the caller decides whether that ends the run.
bool resumeVsyncContinuation(Core &core, std::uint32_t boundaryPc);

} // namespace c12