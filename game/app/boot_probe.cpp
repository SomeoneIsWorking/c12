#include "c12_runtime.h"
#include "game.h"
#include "hw_bind.h"
#include "lightrec_executor.h"
#include "title_identity.h"

#include <charconv>
#include <lucent/log.h>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace {

std::uint64_t parsePositive(std::string_view text) {
  std::uint64_t result = 0;
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
  if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || result == 0) {
    throw std::runtime_error("execution budget must be a positive decimal integer");
  }
  return result;
}

int probe(const char *path, std::uint64_t cycles, std::uint64_t turns) {
  const auto image = c12::readAuthenticatedImage(path, c12::kUsaIdentity);
  c12::C12Runtime runtime(image.header);
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  Core &core = game->core;
  const auto mapped = psx::cpu::loadPsxExeImage(core, image.bytes, c12::kUsaIdentity.name);
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
  for (; completed < turns; ++completed) {
    const auto result = executor.executeUntilExit(core.pc, psx::cpu::ExecutionBudget::fromCycles(cycles));
    lucent::info("c12.boot",
                 "turn {}/{}: {} PC=0x{:08x} cycles={} detail={}",
                 completed + 1,
                 turns,
                 psx::cpu::executionExitName(result.reason),
                 result.guestPc,
                 result.cycles,
                 result.detail);
    if (result.reason != psx::cpu::ExecutionExitReason::BudgetExhausted) {
      faulted = result.reason == psx::cpu::ExecutionExitReason::Fault;
      ++completed;
      break;
    }
  }
  const auto &counts = executor.counters();
  lucent::info("c12.boot",
               "completed {}/{} turns; translated={} executed-blocks={} instructions={} "
               "host-dispatches={} cache-hits={} cache-misses={} invalidations={} faults={}",
               completed,
               turns,
               counts.translatedBlocks,
               counts.executedBlocks,
               counts.executedInstructions,
               counts.hostDispatches,
               counts.cacheHits,
               counts.cacheMisses,
               counts.invalidations,
               counts.faults);
  executor.reportFallbackTelemetry("c12 boot probe");
  lucent::info("c12.boot", "scope: authenticated startup only; no display-field scheduler or gameplay qualification");
  return faulted || counts.executedBlocks == 0 ? 1 : 0;
}

} // namespace

int main(int argc, char **argv) {
  try {
    if (argc < 2 || argc > 4) {
      lucent::error("c12.boot", "usage: c12_boot_probe EXECUTABLE [CYCLES_PER_TURN [TURNS]]");
      return 2;
    }
    return probe(argv[1], argc > 2 ? parsePositive(argv[2]) : 100000, argc > 3 ? parsePositive(argv[3]) : 1);
  } catch (const std::exception &error) {
    lucent::error("c12.boot", "REFUSED: {}", error.what());
    return 2;
  }
}
