// boot_probe.cpp — the C-12 startup probe: a bounded observation of the authenticated startup.
//
// A maintainer tool, not a second product: it authenticates the executable, boots the same machine
// the player boots, and drives bounded Lightrec turns under the shared pacer without presenting a
// frame or opening an audio stream.

#include "authenticated_image.h"
#include "machine.h"
#include "title_facts.h"
#include "title_identity.h"
#include "vsync_continuation.h"

#include "core.h"
#include "frame_pacer.h"
#include "game.h"
#include "lightrec_executor.h"

#include <charconv>
#include <lucent/log.h>
#include <stdexcept>
#include <string_view>

namespace c12::probe {
namespace {

std::uint64_t parsePositive(std::string_view text) {
  std::uint64_t result = 0;
  auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
  if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || result == 0) {
    throw std::runtime_error("execution budget must be a positive decimal integer");
  }
  return result;
}

} // namespace

int observe(const char *path, std::uint64_t cycles, std::uint64_t turns) {
  Machine machine(readAuthenticatedImage(path, kUsaIdentity));
  if (const auto refusal = machine.mapExecutable(); refusal) {
    throw std::runtime_error(*refusal);
  }
  Core &core = machine.core();

  lucent::info("c12.boot",
               "authenticated {} at PC 0x{:08x}; {} turn(s), {} cycles each",
               kUsaIdentity.name,
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
    // One host display-field step per iteration, paced against the mode the guest programmed through
    // GP1(0x08). The shared pacer raises the VBlank edge; the guest's own libetc chain advances its
    // vs-count and runs its per-field callbacks.
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
      if (!resumeVsyncContinuation(core, result.guestPc)) {
        throw std::runtime_error("refused an unaligned or unauthenticated VSync continuation");
      }
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
               core.mem_r32(kVSyncQueryCounterAddress),
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

} // namespace c12::probe

int main(int argc, char **argv) {
  try {
    if (argc < 2 || argc > 4) {
      lucent::error("c12.boot", "usage: c12_boot_probe EXECUTABLE [CYCLES_PER_TURN [TURNS]]");
      return 2;
    }
    return c12::probe::observe(argv[1],
                               argc > 2 ? c12::probe::parsePositive(argv[2]) : 100000,
                               argc > 3 ? c12::probe::parsePositive(argv[3]) : 1);
  } catch (const std::exception &error) {
    lucent::error("c12.boot", "REFUSED: {}", error.what());
    return 2;
  }
}