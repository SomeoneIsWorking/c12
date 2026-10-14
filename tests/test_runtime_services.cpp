#include "booted_game.h"
#include "c12_frame_driver.h"
#include "c12_runtime.h"
#include "c12_session.h"
#include "enhancements.h"
#include "title_facts.h"
#include "view_frustum.h"
#include "world_mesh_pass.h"

#include "core.h"
#include "execution_control.h"
#include "execution_exit.h"
#include "game.h"
#include "guest_code_module.h"
#include "platform_hle.h"
#include "stock_libcd.h"

#include <lucent/log.h>

#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace c12::test {

inline constexpr std::uint32_t kAfterVSync = 0x800A7F90u;
inline constexpr std::uint32_t kParameterAddress = 0x80110000u;
inline constexpr std::uint32_t kQueriedFields = 3u;
inline constexpr std::uint8_t kSentinel = 0xA5u;
inline constexpr std::uint8_t kMode = 0xE0u;
inline constexpr std::array<std::uint8_t, 4> kPosition{0x00u, 0x02u, 0x16u, 0x01u};

void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_c12_binds_its_vsync_as_the_frame_boundary() {
  BootedGame booted;
  Game &game = booted.game();
  require(game.platform_hle.hasNativeFrameLoopContract(), "C-12 has no native frame-loop contract");
  require(game.platform_hle.vsyncAddress() == c12::kVSyncAddress, "C-12 bound the wrong VSync boundary");
  const OverrideFn vsync = game.platform_hle.lookup(c12::kVSyncAddress);
  require(vsync != nullptr, "C-12 VSync boundary was not bound");
  require(game.platform_hle.lookup(c12::kVSyncAddress + 4u) == nullptr, "C-12 bound the instruction after VSync");

  game.core.r[31] = kAfterVSync;
  game.core.r[4] = 0;
  vsync(&game.core);
  const auto result = game.core.executionControl().consume();
  require(result.has_value() && result->reason == psx::cpu::ExecutionExitReason::FrameBoundary,
          "C-12 VSync did not request a frame-boundary exit");
  require(game.core.r[31] == kAfterVSync, "C-12 VSync changed the guest continuation register");
}

void test_c12_vsync_negative_query_answers_guest_counter() {
  BootedGame booted;
  Core &core = booted.core();
  core.mem_w32(c12::kVSyncQueryCounterAddress, kQueriedFields);
  core.r[4] = static_cast<std::uint32_t>(-1);
  booted.game().platform_hle.lookup(c12::kVSyncAddress)(&core);
  require(core.r[2] == kQueriedFields, "C-12 VSync query did not return the measured guest counter");
  require(!core.executionControl().consume().has_value(), "C-12 VSync query requested a frame-boundary exit");
}

void command(Core &core, OverrideFn handler, std::uint8_t code) {
  core.r[4] = code;
  core.r[5] = kParameterAddress;
  core.r[6] = 0;
  handler(&core);
  require(core.r[2] == 0, "stock CD command did not complete successfully");
}

void test_c12_cd_command_publishes_its_work_area() {
  BootedGame booted;
  Game &game = booted.game();
  Core &core = booted.core();
  const OverrideFn handler = game.platform_hle.lookup(c12::kCdCommandAddress);
  require(handler != nullptr, "C-12 did not bind its stock CD_cw");
  require(game.platform_hle.lookup(c12::kCdCommandAddress + 4u) == nullptr, "C-12 bound the instruction after CD_cw");
  for (std::uint32_t index = 0; index < kPosition.size(); ++index) {
    core.mem_w8(kParameterAddress + index, kPosition[index]);
    core.mem_w8(c12::kCdLastPositionAddress + index, kSentinel);
  }
  core.mem_w8(c12::kCdLastPositionAddress - 1u, kSentinel);
  core.mem_w8(c12::kCdLastModeAddress, kSentinel);
  core.mem_w8(c12::kCdLastModeAddress + 1u, kSentinel);
  command(core, handler, 0x02u);
  require(game.cd.setloc_lba == 16, "Setloc did not update the native drive position");
  require(core.mem_r8(c12::kCdLastModeAddress) == kSentinel, "Setloc overwrote the mode byte");
  core.mem_w8(kParameterAddress, kMode);
  command(core, handler, 0x0Eu);
  for (std::uint32_t index = 0; index < kPosition.size(); ++index) {
    require(core.mem_r8(c12::kCdLastPositionAddress + index) == kPosition[index],
            "CD position was not published to the measured work area");
  }
  require(core.mem_r8(c12::kCdLastModeAddress) == kMode, "CD mode was not published to the measured work area");
  require(core.mem_r8(c12::kCdLastPositionAddress - 1u) == kSentinel &&
              core.mem_r8(c12::kCdLastModeAddress + 1u) == kSentinel,
          "CD work area publication overwrote adjacent guest state");
}

void test_c12_cd_ready_is_delivered_by_its_interrupt() {
  BootedGame booted;
  const psx::cd::StockLibcd *libcd = booted.game().cd.stockLibrary();
  require(libcd != nullptr, "C-12 bound no stock libcd");
  require(libcd->readySlot() == c12::kCdReadyCallbackSlotAddress,
          "C-12 bound a ready-callback slot other than its measured libcd word");
  require(libcd->deliversOnInterrupt(), "C-12 handed CD completion to the host pump instead of its INT1 service");
}

void test_c12_declares_the_record_path_without_producers() {
  c12::C12Runtime runtime;
  const RenderCapabilities capabilities = runtime.renderCapabilities();
  require(capabilities.defaultPath == RenderPath::Record, "C-12 does not default to the record path");
  require(!capabilities.nativeRenderPath && !capabilities.supports(RenderPath::Native),
          "C-12 declared a native render path it has no producers for");
  require(!runtime.interpolates(), "C-12 declared an interpolation product it does not own");
  BootedGame booted;
  require(booted.core().rsub.mode.path() == RenderPath::Record,
          "the C-12 Game did not install its declared record render path");
}

void test_c12_game_owns_its_session_and_driver() {
  BootedGame booted;
  require(dynamic_cast<c12::C12Session *>(booted.game().sessionState.get()) != nullptr, "C-12 created no C12Session");
  require(dynamic_cast<c12::C12FrameDriver *>(booted.game().frameDriver.get()) != nullptr,
          "C-12 created no C12FrameDriver");
  require(!c12::worldVisibility(booted.core()).active(), "a new C-12 session already holds visible cells");
}

// No resident image, so only the module landing can make guest RAM executable.
void test_c12_declares_runtime_code_module_window() {
  c12::C12Runtime runtime;
  auto game = std::make_unique<Game>(runtime, psx::config::Config{});
  Core &core = game->core;
  const GuestAddressRange window = guestCodeModuleWindow(core);
  require(window.valid(), "C-12 declared no window for its runtime-loaded code");
  require(!window.containsPhysical(c12::kCdReadyCallbackSlotAddress),
          "the C-12 module window swallowed a guest global");
  require(!core.currentImageIdentity(0x801211E0u).has_value(),
          "C-12 module RAM was already a residency before any module load");
  publishGuestCodeModuleLanding(core, {0x0011F9BCu, 0x001201BCu});
  require(core.currentImageIdentity(0x801211E0u).has_value(),
          "the measured GT.LVB module is still not executable after its load landed");
  require(!core.currentImageIdentity(c12::kCdReadyCallbackSlotAddress).has_value(),
          "a CD landing activated guest RAM outside the declared window");
}

void test_c12_packet_pool_is_its_measured_window() {
  BootedGame booted;
  const auto answer = booted.runtime().packetPool(booted.core());
  require(answer.has_value(), "C-12 answered no packet pool");
  const psx::gpu::PacketPoolExtent pool = answer.value_or(psx::gpu::PacketPoolExtent{});
  require(pool.count == 1, "C-12 answered a pool shape other than its one measured window");
  require(pool.low[0] == c12::kPacketPoolLow && pool.high[0] == c12::kPacketPoolHigh,
          "C-12 answered a pool other than the measured one");
  for (const std::uint32_t head : {0x801AFF98u, 0x801B0F98u}) {
    require(pool.contains(head), "a measured parity ordering table is outside the packet pool");
  }
  require(!pool.contains(c12::kPacketPoolHigh), "the packet pool claims the word above its top");
}

void test_c12_widens_frustum_half_width_with_the_canvas() {
  require(c12::widenedFrustumHalfWidth(0xA0, {512, 512}) == 0xA0, "an equal canvas changed the retail frustum");
  require(c12::widenedFrustumHalfWidth(0xA0, {512, 684}) == 0xD6, "a 684-column canvas did not scale the frustum");
  require(c12::widenedFrustumHalfWidth(0xA0, {512, 400}) == 0xA0, "a narrower canvas shrank the frustum");
}

void test_c12_widens_the_polygon_window_margin() {
  require(c12::widenedWindowMargin(512, 512) == 0, "an equal canvas added a polygon margin");
  require(c12::widenedWindowMargin(512, 684) == 86, "a 684-column canvas did not add 86 columns per side");
  require(c12::widenedWindowMargin(512, 400) == 0, "a narrower canvas added a polygon margin");
}

void test_c12_polygon_window_reject() {
  const c12::ScreenVertex rightOfRetail[] = {{520, 10}, {540, 20}, {530, 30}};
  const c12::ScreenVertex leftOfRetail[] = {{-20, 10}, {-5, 20}, {-8, 30}};
  require(c12::polygonOutsideWindow(rightOfRetail, 0), "a polygon right of the retail window was kept");
  require(!c12::polygonOutsideWindow(rightOfRetail, 86), "a polygon inside the widened margin was rejected");
  require(c12::polygonOutsideWindow(leftOfRetail, 0), "a polygon left of the retail window was kept");
  require(!c12::polygonOutsideWindow(leftOfRetail, 86), "a polygon inside the left margin was rejected");
}

void test_c12_widescreen_policy_follows_the_enhancement() {
  BootedGame booted;
  Game *game = &booted.game();
  const GuestWidescreenProjection *policy = booted.runtime().guestWidescreenProjection();
  require(policy != nullptr, "C-12 did not publish a widescreen policy");
  c12::widescreenCvar().set_text(psx::config::Layer::Runtime, "1");
  require(policy->presentationAspect(game->core) == PresentationAspect::Wide16x9,
          "the widescreen enhancement did not ask for 16:9");
  c12::widescreenCvar().set_text(psx::config::Layer::Runtime, "0");
  require(policy->presentationAspect(game->core) == PresentationAspect::Standard4x3,
          "the disabled enhancement did not stay 4:3");
}

} // namespace c12::test

int main() {
  try {
    c12::test::test_c12_binds_its_vsync_as_the_frame_boundary();
    c12::test::test_c12_vsync_negative_query_answers_guest_counter();
    c12::test::test_c12_cd_command_publishes_its_work_area();
    c12::test::test_c12_cd_ready_is_delivered_by_its_interrupt();
    c12::test::test_c12_declares_the_record_path_without_producers();
    c12::test::test_c12_game_owns_its_session_and_driver();
    c12::test::test_c12_declares_runtime_code_module_window();
    c12::test::test_c12_packet_pool_is_its_measured_window();
    c12::test::test_c12_widens_frustum_half_width_with_the_canvas();
    c12::test::test_c12_widens_the_polygon_window_margin();
    c12::test::test_c12_polygon_window_reject();
    c12::test::test_c12_widescreen_policy_follows_the_enhancement();
    lucent::info("c12.runtime",
                 "PASS: C-12 VSync and query, stock CD work area and interrupt delivery, record path, session, "
                 "runtime-code-module, packet pool and widescreen checks");
    return 0;
  } catch (const std::exception &error) {
    lucent::error("c12.runtime", "FAIL: {}", error.what());
    return 1;
  }
}
