// c12_session.h — C-12's per-Core state, created with each Game.

#pragma once

#include "cell_output.h"
#include "game_runtime.h"

class Core;

namespace c12 {

class C12Session final : public psx::game::TitleSessionState {
public:
  WorldVisibility visibility;
};

// The world visibility of the Game `core` belongs to.
WorldVisibility &worldVisibility(Core &core);

} // namespace c12
