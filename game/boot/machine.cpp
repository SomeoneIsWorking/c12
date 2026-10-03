#include "machine.h"

#include "title_identity.h"

#include "core.h"
#include "game.h"
#include "hw_bind.h"
#include "psx_exe_image.h"

namespace c12 {

Machine::Machine(AuthenticatedImage image) : image_(std::move(image)), runtime_(image_.header) {
  // Installed before the `Game` exists: a `Game` reads the process's installed runtime as it is
  // constructed, and every per-Core device is bound against it.
  psxport_install_game(runtime_);
  game_ = std::make_unique<Game>();

  // Per-Core bindings, before any guest code can run.
  Core &core = game_->core;
  gte_bind(&core);
  gte_init();
  core.rsub.projprim.bind(&core);
  spu_bind(&core);
  mdec_bind(&core);
  xa_bind(&core);
  game_->gpu.gpu_native_init();
  game_->cd.overridesInit();
  game_->platform_hle.initBuiltins();
  game_->platform_hle.requireNativeFrameLoopContract();
  game_->pad.overridesInit();
}

Core &Machine::core() {
  return game_->core;
}

std::optional<std::string> Machine::mapExecutable() {
  const auto mapped = psx::cpu::loadPsxExeImage(game_->core, image_.bytes, kUsaIdentity.name);
  if (!mapped) {
    return mapped.detail;
  }
  return std::nullopt;
}

} // namespace c12