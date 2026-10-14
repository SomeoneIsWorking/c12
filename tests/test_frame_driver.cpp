// C12FrameDriver through the shipping runtime and FrameLoopShell, over synthetic guests written at the
// executable's entry and calling the title's measured VSync address.

#include "c12_frame_driver.h"
#include "c12_runtime.h"
#include "executable_identity.h"
#include "title_facts.h"

#include "core.h"
#include "frame_loop_shell.h"
#include "game.h"

#include <lucent/log.h>

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>

namespace c12::test {
namespace {

constexpr std::uint32_t rZero = 0;
constexpr std::uint32_t rA0 = 4;
constexpr std::uint32_t kEntry = identity::kEntry;

void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

constexpr std::uint32_t jal(std::uint32_t target) {
  return 0x0C000000u | ((target >> 2u) & 0x03FFFFFFu);
}
constexpr std::uint32_t j(std::uint32_t target) {
  return 0x08000000u | ((target >> 2u) & 0x03FFFFFFu);
}
constexpr std::uint32_t addiu(std::uint32_t rt, std::uint32_t rs, std::int32_t imm) {
  return (0x09u << 26u) | (rs << 21u) | (rt << 16u) | (static_cast<std::uint32_t>(imm) & 0xFFFFu);
}
constexpr std::uint32_t breakInstruction() {
  return 0x0000000Du;
}
constexpr std::uint32_t nop() {
  return 0;
}

// VSync(0), then back to the entry: every turn ends at the title's VSync boundary.
constexpr std::array<std::uint32_t, 4> kVSyncLoop{
    jal(kVSyncAddress),
    addiu(rA0, rZero, 0),
    j(kEntry),
    nop(),
};

// A turn that neither reaches VSync nor runs out of budget; the jump ends Lightrec's block scan, which
// does not stop at `break`.
constexpr std::array<std::uint32_t, 3> kBreak{
    breakInstruction(),
    j(kEntry),
    nop(),
};

// One booted C-12 Game with `program` at the executable entry, stepped through the shell.
class DriverRun {
public:
  explicit DriverRun(std::span<const std::uint32_t> program) {
    psxport_install_game(runtime_);
    game_ = std::make_unique<Game>();
    Core &core = game_->core;
    image_ = core.imageCatalog().activate("C-12 resident", runtime_.guestProgramImage()->residentText, 1u);
    for (std::uint32_t index = 0; index < program.size(); ++index) {
      core.mem_w32(kEntry + index * 4u, program[index]);
    }
    core.pc = kEntry;
    runtime_.registerOverrides(*game_);
    shell_.prepareProduct(*game_);
  }

  ~DriverRun() {
    game_->core.imageCatalog().deactivate(image_);
  }

  DriverRun(const DriverRun &) = delete;
  DriverRun &operator=(const DriverRun &) = delete;
  DriverRun(DriverRun &&) = delete;
  DriverRun &operator=(DriverRun &&) = delete;

  // Steps once and requires exactly one presentation fence.
  void step(std::uint32_t frame) {
    const std::uint64_t fence = game_->presentation.fence();
    shell_.step(game_->core, frame);
    require(game_->presentation.fence() == fence + 1u, "a C-12 step did not cross exactly one presentation fence");
  }

  Game &game() {
    return *game_;
  }
  C12FrameDriver &driver() {
    return dynamic_cast<C12FrameDriver &>(*game_->frameDriver);
  }

private:
  C12Runtime runtime_;
  std::unique_ptr<Game> game_;
  psx::frame::FrameLoopShell shell_;
  psx::cpu::ImageIdentity image_{};
};

void test_every_step_is_one_turn_one_boundary_and_one_fence() {
  DriverRun run(kVSyncLoop);
  for (std::uint32_t frame = 1; frame <= 4; ++frame) {
    run.step(frame);
    require(run.game().timing.logicFrame == frame, "the driver did not begin the step's logic frame");
    require(run.driver().fields() == frame, "the driver did not count one field per step");
    require(run.driver().boundaries() == frame, "a VSync turn was not counted as one guest boundary");
    require(run.game().core.pc == kEntry + 8u, "the driver did not resume at the VSync continuation in r31");
    require(!run.game().run.shouldEnd(), "a VSync turn ended the run");
  }
}

void test_an_unresumable_turn_ends_the_run_after_its_fence() {
  DriverRun run(kBreak);
  run.step(1);
  require(run.driver().fields() == 1, "the ending turn was not counted as a field");
  require(run.driver().boundaries() == 0, "a turn with no VSync was counted as a guest boundary");
  require(run.game().run.shouldEnd(), "a turn that cannot resume did not end the run");
}

} // namespace
} // namespace c12::test

int main() {
  try {
    c12::test::test_every_step_is_one_turn_one_boundary_and_one_fence();
    c12::test::test_an_unresumable_turn_ends_the_run_after_its_fence();
    lucent::info("c12.frame", "PASS: C12FrameDriver steps, boundaries, resume and run end");
    return 0;
  } catch (const std::exception &error) {
    lucent::error("c12.frame", "FAIL: {}", error.what());
    return 1;
  }
}
