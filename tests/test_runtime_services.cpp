#include "c12_runtime.h"
#include "cd_control.h"
#include "execution_control.h"
#include "execution_exit.h"
#include "game.h"
#include "guest_cd_stream_callback_layout.h"
#include "guest_code_module.h"
#include "guest_packet_pool_windows.h"
#include "platform_hle.h"
#include "psx_exe_image.h"
#include "runtime_service_fixture.h"
#include "title_facts.h"

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

void test_c12_declares_guest_interrupt_cd_delivery() {
  psx::cpu::PsxExeImage image{};
  c12::C12Runtime runtime(image);
  const auto *layout = runtime.guestCdStreamCallbackLayout();
  require(layout != nullptr, "C-12 published no CD ready-callback layout");
  require(layout->valid(), "C-12 declared no guest ready-callback slot");
  require(layout->readyCallbackPointer == c12::kCdReadyCallbackSlotAddress,
          "C-12 declared a ready-callback slot other than its measured libcd word");
  require(layout->owner == GuestCdStreamCallbackLayout::DeliveryOwner::GuestInterrupt,
          "C-12 handed CD completion to the host pump instead of its own CD interrupt path");
  require(layout->readyStatus == c12::kCdReadyStatus, "C-12 declared a foreign completion code");
}

void test_c12_declares_runtime_code_module_window(GameRuntime &runtime) {
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  Core &core = game->core;
  const GuestAddressRange window = guestCodeModuleWindow(core);
  require(window.valid(), "C-12 declared no window for its runtime-loaded code");
  require(window.containsPhysical(c12::kCdReadyCallbackSlotAddress ^ 0x80000000u) == false,
          "the C-12 module window swallowed a guest global");
  require(!core.currentImageIdentity(0x801211E0u).has_value(),
          "C-12 module RAM was already a residency before any module load");
  publishGuestCodeModuleLanding(core, {0x0011F9BCu, 0x001201BCu});
  require(core.currentImageIdentity(0x801211E0u).has_value(),
          "the measured GT.LVB module is still not executable after its load landed");
  require(core.currentImageIdentity(c12::kCdReadyCallbackSlotAddress).has_value() == false,
          "a CD landing activated guest RAM outside the declared window");
}

void test_c12_declares_measured_packet_pool() {
  psx::cpu::PsxExeImage image{};
  c12::C12Runtime runtime(image);
  const auto *windows = runtime.guestPacketPoolWindows();
  require(windows != nullptr, "C-12 published no 2D packet pool");
  require(windows->valid(), "C-12 declared an empty 2D packet pool");
  require(windows->representation == GuestPacketPoolWindows::Representation::SingleWindow,
          "C-12 declared a pool shape it does not use instead of its measured one window");
  require(windows->base == c12::kPacketPoolLow, "C-12 declared a pool floor other than the measured one");
  require(windows->end == c12::kPacketPoolHigh, "C-12 declared a pool top other than the measured one");
  for (const std::uint32_t head : {0x801AFF98u, 0x801B0F98u}) {
    require(head >= windows->base && head < windows->end,
            "a measured parity ordering table is outside the declared packet pool");
  }
  require(windows->end > c12::kCdReadyCallbackSlotAddress,
          "the declared packet pool stops below the band it was measured over");
}

} // namespace c12::test

int main() {
  try {
    c12::test::test_c12_declares_recorded_vsync_boundary();
    c12::test::test_c12_installs_typed_frame_boundary_without_guest_override();
    c12::test::test_c12_vsync_negative_query_answers_guest_counter();
    c12::test::test_c12_does_not_admit_adjacent_guest_code();
    c12::test::test_c12_declares_guest_interrupt_cd_delivery();
    psx::cpu::PsxExeImage image{};
    c12::C12Runtime runtime(image);
    c12::test::test_c12_cd_work_area(runtime, true);
    c12::test::UndeclaredCdWorkAreaRuntime undeclared;
    c12::test::test_c12_cd_work_area(undeclared, false);
    c12::test::test_c12_declares_runtime_code_module_window(runtime);
    c12::test::test_c12_declares_measured_packet_pool();
    lucent::info("c12.runtime",
                 "PASS: 9/9 C-12 VSync/query/admission, stock CD work-area, CD ready-callback, "
                 "runtime-code-module and 2D packet-pool checks");
    return 0;
  } catch (const std::exception &error) {
    lucent::error("c12.runtime", "FAIL: {}", error.what());
    return 1;
  }
}
