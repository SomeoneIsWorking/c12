#include "c12_platform_facts.h"
#include "c12_runtime.h"
#include "cd_control.h"
#include "execution_control.h"
#include "execution_exit.h"
#include "game.h"
#include "platform_hle.h"
#include "psx_exe_image.h"
#include "runtime_service_fixture.h"

#include <cstdint>
#include <lucent/log.h>
#include <memory>
#include <stdexcept>

namespace c12::test {

void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void harmlessHandler(Core *) {
}

void test_c12_declares_recorded_vsync_boundary() {
  psx::cpu::PsxExeImage image{};
  c12::C12Runtime runtime(image);
  require(runtime.platformHlePlan() != nullptr, "C-12 did not publish a PlatformHlePlan");
  require(runtime.platformHlePlan()->vsyncAddress == c12::kVSyncAddress,
          "C-12 VSync address changed from the recorded boundary");
  require(runtime.platformHlePlan()->vsyncQueryCounterAddress == c12::kVSyncQueryCounterAddress,
          "C-12 dropped the measured libetc vs-count from its VSync query contract");
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
  auto result = game->core.executionControl().consume();
  if (!result.has_value()) {
    throw std::runtime_error("C-12 VSync service did not request a typed exit");
  }
  require(result->reason == psx::cpu::ExecutionExitReason::FrameBoundary,
          "C-12 VSync service requested the wrong exit");
  require(game->core.r[31] == kAfterVSync, "C-12 VSync service changed the guest continuation register");
}

void test_c12_vsync_negative_query_answers_guest_counter() {
  psx::cpu::PsxExeImage image{};
  c12::C12Runtime runtime(image);
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  game->platform_hle.initBuiltins();
  auto &core = game->core;
  core.mem_w32(c12::kVSyncQueryCounterAddress, kQueriedFields);
  core.r[4] = static_cast<std::uint32_t>(-1);
  game->platform_hle.lookup(c12::kVSyncAddress)(&core);
  require(core.r[2] == kQueriedFields, "C-12 VSync query did not return the measured guest counter");
  require(!core.executionControl().consume().has_value(), "C-12 VSync query requested a frame-boundary exit");
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

void command(Core &core, OverrideFn handler, std::uint8_t code) {
  core.r[4] = code;
  core.r[5] = kParameterAddress;
  core.r[6] = 0;
  handler(&core);
  require(core.r[2] == 0, "stock CD command did not complete successfully");
}

void test_c12_cd_work_area(GameRuntime &runtime, bool declared) {
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  game->platform_hle.initBuiltins();
  auto &core = game->core;
  require(core.cfg == nullptr, "CD test entered the legacy configuration adapter");
  auto handler = game->platform_hle.lookup(c12::kCdCommandAddress);
  require(handler == cd_command_stock_sync, "C-12 command entry does not select the stock CD owner");
  require(!game->platform_hle.register_(c12::kCdCommandAddress + 4u, harmlessHandler),
          "CD admission includes the adjacent guest instruction");
  for (std::uint32_t index = 0; index < kPosition.size(); ++index) {
    core.mem_w8(kParameterAddress + index, kPosition[index]);
    core.mem_w8(c12::kCdLastPositionAddress + index, kSentinel);
  }
  core.mem_w8(c12::kCdLastPositionAddress - 1u, kSentinel);
  core.mem_w8(c12::kCdLastModeAddress, kSentinel);
  core.mem_w8(c12::kCdLastModeAddress + 1u, kSentinel);
  command(core, handler, 0x02u);
  require(game->cd.setloc_lba == 16, "Setloc did not update the native drive position");
  require(core.mem_r8(c12::kCdLastModeAddress) == kSentinel, "Setloc overwrote the mode byte");
  core.mem_w8(kParameterAddress, kMode);
  command(core, handler, 0x0Eu);
  for (std::uint32_t index = 0; index < kPosition.size(); ++index) {
    require(core.mem_r8(c12::kCdLastPositionAddress + index) == (declared ? kPosition[index] : kSentinel),
            "CD position publication did not respect the declared guest work area");
  }
  require(core.mem_r8(c12::kCdLastModeAddress) == (declared ? kMode : kSentinel),
          "CD mode publication did not respect the declared guest work area");
  require(core.mem_r8(c12::kCdLastPositionAddress - 1u) == kSentinel &&
              core.mem_r8(c12::kCdLastModeAddress + 1u) == kSentinel,
          "CD work area publication overwrote adjacent guest state");
}

} // namespace c12::test

int main() {
  try {
    c12::test::test_c12_declares_recorded_vsync_boundary();
    c12::test::test_c12_installs_typed_frame_boundary_without_guest_override();
    c12::test::test_c12_vsync_negative_query_answers_guest_counter();
    c12::test::test_c12_does_not_admit_adjacent_guest_code();
    psx::cpu::PsxExeImage image{};
    c12::C12Runtime runtime(image);
    c12::test::test_c12_cd_work_area(runtime, true);
    c12::test::UndeclaredCdWorkAreaRuntime undeclared;
    c12::test::test_c12_cd_work_area(undeclared, false);
    lucent::info("c12.runtime", "PASS: 6/6 C-12 VSync/query/admission and stock CD work-area checks");
    return 0;
  } catch (const std::exception &error) {
    lucent::error("c12.runtime", "FAIL: {}", error.what());
    return 1;
  }
}
