// booted_game.h — a C-12 Game admitted through the shipping path with the title's overrides registered.

#pragma once

#include "c12_runtime.h"
#include "executable_identity.h"

#include "core.h"
#include "game.h"
#include "psx_exe_image.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace c12::test {

// A PS-X EXE with C-12's header fields and zeroed text; tests write their guest code over it.
inline std::vector<std::uint8_t> syntheticExecutable() {
  std::vector<std::uint8_t> bytes(psx::cpu::kPsxExeHeaderBytes + identity::kTextBytes);
  const auto word = [&bytes](std::size_t offset, std::uint32_t value) {
    for (std::size_t index = 0; index < 4; ++index) {
      bytes[offset + index] = static_cast<std::uint8_t>(value >> (index * 8u));
    }
  };
  const char magic[] = "PS-X EXE";
  for (std::size_t index = 0; index < 8; ++index) {
    bytes[index] = static_cast<std::uint8_t>(magic[index]);
  }
  word(0x10, identity::kEntry);
  word(0x14, identity::kGlobalPointer);
  word(0x18, identity::kTextAddress);
  word(0x1C, identity::kTextBytes);
  word(0x30, identity::kStackAddress);
  word(0x34, identity::kStackOffset);
  return bytes;
}

class BootedGame {
public:
  BootedGame() : game_(std::make_unique<Game>(runtime_, psx::config::Config{})) {
    if (!runtime_.admitExecutable(game_->core, syntheticExecutable())) {
      throw std::runtime_error("C-12 refused the synthetic executable");
    }
    runtime_.registerOverrides(*game_);
  }
  ~BootedGame() = default;
  BootedGame(const BootedGame &) = delete;
  BootedGame &operator=(const BootedGame &) = delete;
  BootedGame(BootedGame &&) = delete;
  BootedGame &operator=(BootedGame &&) = delete;

  C12Runtime &runtime() {
    return runtime_;
  }
  Game &game() {
    return *game_;
  }
  Core &core() {
    return game_->core;
  }

private:
  C12Runtime runtime_;
  std::unique_ptr<Game> game_;
};

} // namespace c12::test
