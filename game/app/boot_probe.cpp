#include "c12_platform_facts.h"
#include "c12_runtime.h"
#include "frame_pacer.h"
#include "game.h"
#include "hw_bind.h"
#include "lightrec_executor.h"
#include "platform_hle.h"
#include "title_identity.h"

#include <charconv>
#include <lucent/log.h>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace c12::app {

std::uint64_t parsePositive(std::string_view text) {
  std::uint64_t result = 0;
  auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
  if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || result == 0) {
    throw std::runtime_error("execution budget must be a positive decimal integer");
  }
  return result;
}

int probe(const char *path, std::uint64_t cycles, std::uint64_t turns) {
  auto image = c12::readAuthenticatedImage(path, c12::kUsaIdentity);
  c12::C12Runtime runtime(image.header);
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  Core &core = game->core;
  auto mapped = psx::cpu::loadPsxExeImage(core, image.bytes, c12::kUsaIdentity.name);
  if (!mapped) {
    throw std::runtime_error(mapped.detail);
  }
  gte_bind(&core);
  gte_init();
  core.rsub.projprim.bind(&core);
  spu_bind(&core);
  mdec_bind(&core);
  xa_bind(&core);
  game->gpu.gpu_native_init();
  game->cd.overridesInit();
  game->platform_hle.initBuiltins();
  game->platform_hle.requireNativeFrameLoopContract();
  game->pad.overridesInit();

  lucent::info("c12.boot",
               "authenticated {} at PC 0x{:08x}; {} turn(s), {} cycles each",
               c12::kUsaIdentity.name,
               core.pc,
               turns,
               cycles);
  auto &executor = core.lightrecExecutor();
  bool faulted = false;
  std::uint64_t completed = 0;
  std::uint64_t frameBoundaries = 0;
  std::uint64_t resumedTurns = 0;
  std::uint64_t fieldSteps = 0;
  bool resumePending = false;
  for (; completed < turns; ++completed) {
    // One host display-field step per loop iteration, paced against the mode the guest programmed
    // through GP1(0x08). The shared pacer raises the VBlank edge; the guest's own libetc chain
    // advances its vs-count and runs its per-field callbacks.
    gpu_pace_frame(&core);
    ++fieldSteps;
    auto blocksBefore = executor.counters().executedBlocks;
    auto result = executor.executeUntilExit(core.pc, psx::cpu::ExecutionBudget::fromCycles(cycles));
    lucent::info("c12.boot",
                 "turn {}/{}: {} PC=0x{:08x} cycles={} detail={}",
                 completed + 1,
                 turns,
                 psx::cpu::executionExitName(result.reason),
                 result.guestPc,
                 result.cycles,
                 result.detail);
    if (resumePending && executor.counters().executedBlocks > blocksBefore) {
      ++resumedTurns;
    }
    resumePending = false;
    if (result.reason == psx::cpu::ExecutionExitReason::FrameBoundary) {
      ++frameBoundaries;
      auto continuation = core.r[31];
      if ((continuation & 3u) != 0 || !core.currentImageIdentity(continuation)) {
        throw std::runtime_error("VSync returned an unaligned or unauthenticated continuation");
      }
      lucent::info("c12.boot", "VSync boundary=0x{:08x} continuation=0x{:08x}", result.guestPc, continuation);
      core.pc = continuation;
      resumePending = true;
      continue;
    }
    if (result.reason != psx::cpu::ExecutionExitReason::BudgetExhausted) {
      faulted = result.reason == psx::cpu::ExecutionExitReason::Fault;
      ++completed;
      break;
    }
  }
  auto &counts = executor.counters();
  lucent::info("c12.boot",
               "completed {}/{} turns; frame-boundaries={} resumed-turns={} field-steps={} guest-vs-count={} "
               "translated={} executed-blocks={} instructions={} "
               "host-dispatches={} cache-hits={} cache-misses={} invalidations={} faults={}",
               completed,
               turns,
               frameBoundaries,
               resumedTurns,
               fieldSteps,
               core.mem_r32(c12::kVSyncQueryCounterAddress),
               counts.translatedBlocks,
               counts.executedBlocks,
               counts.executedInstructions,
               counts.hostDispatches,
               counts.cacheHits,
               counts.cacheMisses,
               counts.invalidations,
               counts.faults);
  executor.reportFallbackTelemetry("c12 boot probe");
  lucent::info("c12.boot",
               "scope: authenticated startup field lifecycle under the shared pacer; no native presentation or "
               "gameplay qualification");
  return faulted || counts.executedBlocks == 0 ? 1 : 0;
}

} // namespace c12::app

int main(int argc, char **argv) {
  try {
    if (argc < 2 || argc > 4) {
      lucent::error("c12.boot", "usage: c12_boot_probe EXECUTABLE [CYCLES_PER_TURN [TURNS]]");
      return 2;
    }
    return c12::app::probe(
        argv[1], argc > 2 ? c12::app::parsePositive(argv[2]) : 100000, argc > 3 ? c12::app::parsePositive(argv[3]) : 1);
  } catch (const std::exception &error) {
    lucent::error("c12.boot", "REFUSED: {}", error.what());
    return 2;
  }
}
