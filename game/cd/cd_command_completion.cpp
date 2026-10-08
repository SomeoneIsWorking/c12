#include "cd_command_completion.h"

#include "core.h"
#include "execution_control.h"
#include "execution_exit.h"
#include "game.h"
#include "guest_call.h"
#include "native_dispatch.h"
#include "r3000.h"
#include "title_facts.h"

#include <lucent/log.h>

#include <cstdint>
#include <string_view>

namespace c12 {

namespace {

// MIPS o32 registers.
enum : std::uint8_t { V0 = 2, A0 = 4, A1 = 5, A2 = 6 };

// FUN_800abd98 returns 1 on acceptance, 0 after 3 retries.
constexpr std::uint32_t kGuestAcceptedCommand = 1u;

// Completion code FUN_80057f7c branches on.
constexpr std::uint32_t kLibCdStatusComplete = 2u;

constexpr std::string_view kOwnerName = "c12 cd command completion";

void deliverCommandStatus(Core &core, std::uint32_t response) {
  // The title swaps this callback when it opens the queue, so read it per call.
  const std::uint32_t callback = core.mem_r32(kCdCommandStatusCallbackSlotAddress);
  if (!callback) {
    lucent::debug("cdqueue",
                  "CdReadySync completed with no command-status callback at 0x{:08X}: queue holds no state",
                  kCdCommandStatusCallbackSlotAddress);
    return;
  }
  // The callback runs like an interrupt handler: all registers are restored.
  const R3000 saved = *static_cast<R3000 *>(&core);
  core.r[A0] = kLibCdStatusComplete;
  core.r[A1] = response;
  const auto result = psx::cpu::dispatchGuest0(core, callback, psx::cpu::ExecutionBudget::currentTurn(core));
  *static_cast<R3000 *>(&core) = saved;
  lucent::debug("cdqueue",
                "CdReadySync delivered completion to 0x{:08X} (status {}, response 0x{:08X})",
                callback,
                kLibCdStatusComplete,
                response);
  psx::cpu::completeOrPropagate(core, result);
}

} // namespace

void installTitleOverrides(Game &game) {
  psx::cpu::installNativeOverride(
      game.core, kCdReadySyncAddress, "c12::completeCdReadySyncCommand", completeCdReadySyncCommand);
  lucent::info("cdqueue", "installed CD command-completion owner at 0x{:08X}", kCdReadySyncAddress);
}

void completeCdReadySyncCommand(Core *host) {
  Core &core = *host;
  // callOriginal clobbers the argument registers.
  const std::uint32_t response = core.r[A2];
  const std::uint32_t command = core.r[A0] & 0xFFu;
  psx::cpu::callOriginalToReturn(core, kCdReadySyncAddress, psx::cpu::ExecutionBudget::currentTurn(core), kOwnerName);
  if (core.r[V0] != kGuestAcceptedCommand) {
    lucent::debug(
        "cdqueue", "CdReadySync command 0x{:02X} not accepted by guest retry loop (v0={})", command, core.r[V0]);
    return;
  }
  deliverCommandStatus(core, response);
}

} // namespace c12
