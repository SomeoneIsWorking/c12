// player_entry.cpp — the C-12 player executable.
//
// Zero-argument product: the launcher provisions the authenticated executable and names it through
// `PSXPORT_C12_EXECUTABLE` (a developer may pass it as the first argument instead). The entry
// authenticates those bytes, composes one machine, opens the control channel, and hands the run to
// the title's display-field owner — the one thing that crosses the framework's presentation fence.

#include "authenticated_image.h"
#include "guest_field_loop.h"
#include "machine.h"
#include "title_identity.h"

#include "cfg.h"
#include "game.h"
#include "lightrec_executor.h"

#include <lucent/log.h>

#include <string>

namespace c12::app {
namespace {

// The launcher's contract: one argument, else the named executable, else nothing to run.
std::string playerExecutable(int argc, char **argv) {
  if (argc == 2) {
    return argv[1];
  }
  const char *named = cfg_str("PSXPORT_C12_EXECUTABLE");
  return named != nullptr ? named : "";
}

} // namespace

int runPlayer(int argc, char **argv) {
  if (argc > 2) {
    lucent::error("c12.boot", "usage: c12_port [EXECUTABLE]");
    return 2;
  }

  const std::string path = playerExecutable(argc, argv);
  if (path.empty()) {
    lucent::error("c12.boot",
                  "REFUSED: no C-12 executable — the launcher sets PSXPORT_C12_EXECUTABLE, or pass "
                  "the path as the first argument");
    return 2;
  }

  Machine machine(readAuthenticatedImage(path, kUsaIdentity));
  if (const auto refusal = machine.mapExecutable(); refusal) {
    lucent::error("c12.boot", "REFUSED: {}", *refusal);
    return 2;
  }
  Core &core = machine.core();
  machine.game().spu_audio.init();
  machine.runtime().registerOverrides(machine.game());

  // The live control channel, always open on loopback: PSXPORT_DEBUG_SERVER names the port, and
  // `attach` answers the frame cap that a client-driven run must not have.
  const int clientCap = machine.game().dbg_server.attach(&core, cfg_int("PSXPORT_NATIVE_FRAMES", 0));
  GuestFieldLoop fields(machine.game(), core);

  lucent::info(
      "c12.boot", "authenticated {} at PC 0x{:08x} — entering the title field lifecycle", kUsaIdentity.name, core.pc);

  fields.run(clientCap > 0 ? clientCap : 0);

  const auto &counts = core.lightrecExecutor().counters();
  lucent::info("c12.boot",
               "presented {} field(s) across {} guest VSync boundar(ies); translated={} executed-blocks={} "
               "instructions={} faults={}",
               fields.fields(),
               fields.boundaries(),
               counts.translatedBlocks,
               counts.executedBlocks,
               counts.executedInstructions,
               counts.faults);
  core.lightrecExecutor().reportFallbackTelemetry("c12 player");
  return counts.executedBlocks == 0 || counts.faults != 0 ? 1 : 0;
}

} // namespace c12::app

int main(int argc, char **argv) {
  return c12::app::runPlayer(argc, argv);
}