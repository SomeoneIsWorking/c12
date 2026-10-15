#include "frame/frame_driver.h"

#include "frame/vsync_continuation.h"

#include "core.h"
#include "game.h"
#include "lightrec_executor.h"
#include "logic_frame.h"

#include <lucent/log.h>

#include <cstdlib>

namespace c12 {

FrameDriver::FrameDriver(Game &game) : game_(game) {
}

void FrameDriver::stepFrame(Core &core, std::uint32_t frame) {
  psx::frame::LogicFrame::begin(game_, frame);

  auto &executor = core.lightrecExecutor();
  const auto result = executor.executeUntilExit(core.pc, psx::cpu::ExecutionBudget::fromCycles(kCyclesPerTurn));
  if (result.reason == psx::cpu::ExecutionExitReason::FrameBoundary) {
    ++boundaries_;
    if (!resumeVsyncContinuation(core, result.guestPc)) {
      std::abort();
    }
  } else if (result.reason != psx::cpu::ExecutionExitReason::BudgetExhausted) {
    lucent::error("c12.fields",
                  "guest turn ended with {} at PC 0x{:08x} after {} cycles — ending the run at field {}",
                  psx::cpu::executionExitName(result.reason),
                  result.guestPc,
                  result.cycles,
                  frame);
    game_.run.requestEnd();
  }

  game_.presentation.commit(&core, 1);
  game_.spu_audio.frame();
  ++fields_;
}

} // namespace c12
