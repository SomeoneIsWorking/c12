// session.h — C-12's per-Core state, created with each Game.

#pragma once

#include "game_runtime.h"
#include "render/cell_output.h"

class Core;

namespace c12 {

class Session final : public psx::game::TitleSessionState {
public:
  WorldVisibility visibility;
};

// The world visibility of the Game `core` belongs to.
WorldVisibility &worldVisibility(Core &core);

} // namespace c12
