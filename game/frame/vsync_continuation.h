// vsync_continuation.h — rule for resuming a guest VSync continuation.

#pragma once

#include <cstdint>

class Core;

namespace c12 {

// Resumes at the continuation in r31 if aligned and inside a loaded image; false otherwise.
bool resumeVsyncContinuation(Core &core, std::uint32_t boundaryPc);

} // namespace c12