#include "guest_field_loop.h"

#include "vsync_continuation.h"

#include "game.h"
#include "gpu_native_internal.h"
#include "lightrec_executor.h"

#include <lucent/log.h>

#include <cstdlib>

namespace c12 {

GuestFieldLoop::GuestFieldLoop(Game &game, Core &core) : game_(game), core_(core) {
}

void GuestFieldLoop::run(int frameLimit) {
  while (frameLimit <= 0 || static_cast<int>(fields_) < frameLimit) {
    if (!stepField()) {
      return;
    }
    game_.dbg_server.service(&core_);
    game_.dbg_server.honourPause(&core_);
  }
}

bool GuestFieldLoop::stepField() {
  // Input first: the guest reads the pad through the real SIO0 chain at the top of its own field,
  // so the masks for this field have to be latched before it runs.
  game_.pad.serviceFrame();

  auto &executor = core_.lightrecExecutor();
  const auto result = executor.executeUntilExit(core_.pc, psx::cpu::ExecutionBudget::fromCycles(kCyclesPerTurn));
  if (result.reason == psx::cpu::ExecutionExitReason::FrameBoundary) {
    ++boundaries_;
    if (!resumeVsyncContinuation(core_, result.guestPc)) {
      std::abort();
    }
  } else if (result.reason != psx::cpu::ExecutionExitReason::BudgetExhausted) {
    lucent::error("c12.fields",
                  "guest turn ended with {} at PC 0x{:08x} after {} cycles — stopping at field {}",
                  psx::cpu::executionExitName(result.reason),
                  result.guestPc,
                  result.cycles,
                  fields_);
    return false;
  }

  // The ONE presentation fence of this field: `commit` paces the display field against the mode the
  // guest programmed through GP1(0x08), emits what this field's draws queued, and presents it. The
  // guest's own VSync never crosses a fence; the host does, once, per field.
  game_.presentation.commit(&core_, 1);
  game_.spu_audio.frame();
  ++fields_;
  return true;
}

} // namespace c12