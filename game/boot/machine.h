// machine.h — the C-12 machine: one title runtime policy, one Game, one boot sequence.

#pragma once

#include "authenticated_image.h"
#include "c12_runtime.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

class Core;
class Game;

namespace c12 {

class Machine {
public:
  explicit Machine(AuthenticatedImage image);

  // Maps the executable into guest RAM and sets the entry PC; returns the refusal on failure.
  std::optional<std::string> mapExecutable();

  Game &game() {
    return *game_;
  }
  Core &core();
  C12Runtime &runtime() {
    return runtime_;
  }

private:
  AuthenticatedImage image_;
  C12Runtime runtime_;         // Outlives game_: the framework holds a pointer to it.
  std::unique_ptr<Game> game_; // Heap: Game embeds 2 MiB of guest RAM.
};

} // namespace c12