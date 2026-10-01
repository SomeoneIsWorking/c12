// player_entry.cpp — the C-12 player executable.
//
// Zero-argument path: the launcher provisions the authenticated executable and names it through
// `PSXPORT_C12_EXECUTABLE` (a developer may pass it as the first argument instead). The entry
// authenticates those bytes before any guest code exists, builds one fully wired `Game`, and hands
// the machine to the title's display-field owner, which is what crosses the framework's presentation
// fence. That composition — authentication, image mapping, per-Core device binding, the title
// runtime — is the diagnostic probe's; the field loop is the difference that makes it a product.

#include "authenticated_image.h"
#include "c12_platform_facts.h"
#include "c12_runtime.h"
#include "guest_field_loop.h"
#include "lightrec_executor.h"
#include "platform_hle.h"
#include "title_identity.h"

#include "cfg.h"
#include "game.h"
#include "hw_bind.h"

#include <lucent/log.h>

#include <string>

int main(int argc, char **argv) {
  if (argc > 2) {
    lucent::error("c12.boot", "usage: c12_port [EXECUTABLE]");
    return 2;
  }

  // Nothing about the executable is trusted until `authenticateImage` has hashed it.
  std::string path =
      argc == 2 ? argv[1] : std::string(cfg_str("PSXPORT_C12_EXECUTABLE") ? cfg_str("PSXPORT_C12_EXECUTABLE") : "");
  if (path.empty()) {
    lucent::error("c12.boot",
                  "REFUSED: no C-12 executable — the launcher sets PSXPORT_C12_EXECUTABLE, or pass "
                  "the path as the first argument");
    return 2;
  }
  const c12::AuthenticatedImage image = c12::readAuthenticatedImage(path, c12::kUsaIdentity);

  static c12::C12Runtime runtime(image.header);
  psxport_install_game(runtime);

  // Game owns the whole machine, including 2 MiB of guest RAM and the renderer state; on the heap so
  // the entry stack stays bounded on hosts with the usual 8 MiB thread stack.
  auto game = std::make_unique<Game>();
  Core &core = game->core;
  const auto mapped = psx::cpu::loadPsxExeImage(core, image.bytes, c12::kUsaIdentity.name);
  if (!mapped) {
    lucent::error("c12.boot", "REFUSED: {}", mapped.detail);
    return 2;
  }

  // Per-Core bindings, before any guest code can run.
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
  game->spu_audio.init();
  runtime.registerOverrides(*game);

  // The live control channel, always open on loopback: PSXPORT_DEBUG_SERVER names the port, and
  // `attach` answers the frame cap that a client-driven run must not have.
  const int clientCap = game->dbg_server.attach(&core, cfg_int("PSXPORT_NATIVE_FRAMES", 0));
  const int limit = clientCap > 0 ? clientCap : 0;

  lucent::info("c12.boot",
               "authenticated {} at PC 0x{:08x} — entering the title field lifecycle",
               c12::kUsaIdentity.name,
               core.pc);

  c12::GuestFieldLoop loop(*game, core);
  while (limit <= 0 || static_cast<int>(loop.fields()) < limit) {
    if (!loop.stepField()) {
      break;
    }
    game->dbg_server.service(&core);
    game->dbg_server.honourPause(&core);
  }

  const auto &counts = core.lightrecExecutor().counters();
  lucent::info("c12.boot",
               "presented {} field(s) across {} guest VSync boundar(ies); translated={} executed-blocks={} "
               "instructions={} faults={}",
               loop.fields(),
               loop.boundaries(),
               counts.translatedBlocks,
               counts.executedBlocks,
               counts.executedInstructions,
               counts.faults);
  core.lightrecExecutor().reportFallbackTelemetry("c12 player");
  return counts.executedBlocks == 0 || counts.faults != 0 ? 1 : 0;
}