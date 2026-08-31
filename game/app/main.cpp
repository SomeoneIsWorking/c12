// A bare psxport product: generated code and shared runtime only.
#include "core.h"
#include "frame_loop_shell.h"
#include "game.h"
#include "game_runtime.h"
#include "guest_program_image.h"
#include "hw_bind.h"
#include "overlay_table.h"
#include "recomp_iface.h"
#include "render_capabilities.h"
#include "render_mode.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string_view>

extern "C" {
void mdec_init();
void spu_init();
void watchdog_init();
}

void gte_init();
void load_exe(const char *path, Core *core);
int rec_func_index(std::uint32_t address);
void shard_set_override(std::uint32_t address, RecOverrideFn function);
RecOverrideFn shard_get_override(std::uint32_t address);

namespace {

constexpr char kDiscEnvironment[] = "PSXPORT_DISC";

class BareRuntime final : public GameRuntime {
public:
  BareRuntime() {
    image_.residentText = {.begin = REC_MAIN_LO, .end = REC_MAIN_HI};
  }

  void *createContext(Core &) override { return nullptr; }
  void destroyContext(void *) override {}
  void registerOverrides(Game &) override {}
  void bootInit(Core &) override {}
  RenderCapabilities renderCapabilities() const override {
    return RenderCapabilities::direct();
  }
  bool guestVramIsPicture(const Game &) const override { return true; }
  const GuestProgramImage *guestProgramImage() const override {
    return &image_;
  }
  const GenericWholeProgramProfile *
  genericWholeProgramProfile() const override {
    return &profile_;
  }

private:
  GenericWholeProgramProfile profile_{};
  GuestProgramImage image_{};
};

const RecompRegistry kProgram{
    .main_dispatch = main_dispatch,
    .rec_func_index = rec_func_index,
    .overlays = g_rec_overlays,
    .overlay_count = g_rec_overlay_count,
    .shard_set_override = shard_set_override,
    .ov_a00_set_override = nullptr,
    .ov_game_set_override = nullptr,
    .guestMemset_gen = nullptr,
    .shard_get_override = shard_get_override,
    .substrate_id = g_rec_substrate_id,
    .wholeProgram = &g_rec_whole_program,
};

void printUsage() {
  std::puts(
      "Usage: c12_port EXTRACTED-PS-X-EXE\n"
      "Run the bare generic psxport profile over an extracted executable.");
}

} // namespace

int main(int argc, char **argv) {
  if (argc == 2 && (std::string_view(argv[1]) == "-h" ||
                    std::string_view(argv[1]) == "--help")) {
    printUsage();
    return 0;
  }
  if (argc != 2 || !std::filesystem::is_regular_file(argv[1])) {
    printUsage();
    return 2;
  }

  static BareRuntime runtime;
  psxport_install_game(runtime);
  psxport_install_recomp(&kProgram);
  auto game = std::make_unique<Game>();
  game->disc.env_key = kDiscEnvironment;
  Core *core = &game->core;
  load_exe(argv[1], core);

  gte_init();
  mdec_init();
  spu_init();
  watchdog_init();
  gte_bind(core);
  core->rsub.projprim.bind(core);
  spu_bind(core);
  mdec_bind(core);
  xa_bind(core);
  game->spu_audio.init();
  game->gpu.gpu_native_init();
  game->cd.overridesInit();
  game->pad.overridesInit();
  runtime.registerOverrides(*game);
  render_path_install(core);

  FrameLoopShell shell;
  shell.prepareProduct(*game);
  for (std::uint32_t frame = 0;; ++frame) {
    shell.step(*core, frame);
  }
}
