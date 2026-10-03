// machine.h — the C-12 machine: one title runtime policy, one `Game`, one boot sequence.

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

// Owns the machine for the life of a run: the title's `GameRuntime` policy, the authenticated guest
// image, and the `Game` that carries 2 MiB of guest RAM and every device. Construction is the one
// composition the player and the startup probe share; it decides nothing about a frame.
class Machine {
public:
  // Installs the title runtime policy and binds the per-Core devices against the given authenticated
  // image. The executable is not mapped yet — see `mapExecutable`.
  explicit Machine(AuthenticatedImage image);

  // Maps the authenticated executable into guest RAM and publishes its entry PC. Empty on success,
  // otherwise the framework's refusal detail.
  std::optional<std::string> mapExecutable();

  Game &game() {
    return *game_;
  }
  Core &core();
  // The installed title policy, for the overrides a run registers after the machine is built.
  C12Runtime &runtime() {
    return runtime_;
  }

private:
  AuthenticatedImage image_;
  // The framework keeps a pointer to the installed runtime, so it is declared before the `Game` built
  // on top of it and outlives it.
  C12Runtime runtime_;
  // On the heap: `Game` carries the whole machine, and the entry stack stays bounded on hosts with
  // the usual 8 MiB thread stack.
  std::unique_ptr<Game> game_;
};

} // namespace c12