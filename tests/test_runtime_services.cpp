#include "c12_platform_facts.h"
#include "c12_runtime.h"
#include "execution_control.h"
#include "execution_exit.h"
#include "game.h"
#include "platform_hle.h"
#include "psx_exe_image.h"

#include <cstdint>
#include <lucent/log.h>
#include <memory>
#include <stdexcept>

namespace {

constexpr std::uint32_t kAfterVSync = 0x800A7F90u;

void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void harmlessHandler(Core *) {}

void test_c12_declares_recorded_vsync_boundary() {
  psx::cpu::PsxExeImage image{};
  c12::C12Runtime runtime(image);
  require(runtime.platformHlePlan() != nullptr, "C-12 did not publish a PlatformHlePlan");
  require(runtime.platformHlePlan()->vsyncAddress == c12::kVSyncAddress,
          "C-12 VSync address changed from the recorded boundary");
  require(runtime.platformHlePlan()->windowLo[0] == c12::kVSyncAddress,
          "C-12 VSync window does not begin at its entry");
  require(runtime.platformHlePlan()->windowHi[0] == c12::kVSyncAddress + 4u,
          "C-12 VSync window admits more than its measured entry");
}

void test_c12_installs_typed_frame_boundary_without_guest_override() {
  psx::cpu::PsxExeImage image{};
  c12::C12Runtime runtime(image);
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();

  game->platform_hle.initBuiltins();
  require(game->platform_hle.hasNativeFrameLoopContract(), "C-12 has no native frame-loop contract");
  require(game->platform_hle.vsyncAddress() == c12::kVSyncAddress, "C-12 installed the wrong VSync boundary");
  require(game->platform_hle.lookup(c12::kVSyncAddress) != nullptr, "C-12 VSync boundary was not installed");

  game->core.r[31] = kAfterVSync;
  game->platform_hle.lookup(c12::kVSyncAddress)(&game->core);
  const auto result = game->core.executionControl().consume();
  require(result.has_value(), "C-12 VSync service did not request a typed exit");
  require(result->reason == psx::cpu::ExecutionExitReason::FrameBoundary,
          "C-12 VSync service requested the wrong exit");
  require(game->core.r[31] == kAfterVSync, "C-12 VSync service changed the guest continuation register");
}

void test_c12_does_not_admit_adjacent_guest_code() {
  psx::cpu::PsxExeImage image{};
  c12::C12Runtime runtime(image);
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();

  require(!game->platform_hle.register_(c12::kVSyncAddress + 4u, harmlessHandler),
          "C-12 admitted adjacent guest code as a hardware service");
  require(game->platform_hle.lookup(c12::kVSyncAddress + 4u) == nullptr,
          "C-12 exposed an adjacent guest address through the HLE table");
}

} // namespace

int main() {
  try {
    test_c12_declares_recorded_vsync_boundary();
    test_c12_installs_typed_frame_boundary_without_guest_override();
    test_c12_does_not_admit_adjacent_guest_code();
    lucent::info("c12.runtime", "PASS: C-12 direct-runtime VSync plan and admission boundary");
    return 0;
  } catch (const std::exception &error) {
    lucent::error("c12.runtime", "FAIL: {}", error.what());
    return 1;
  }
}
