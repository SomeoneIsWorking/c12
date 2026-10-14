#include "c12_session.h"

#include "core.h"
#include "game.h"

namespace c12 {

WorldVisibility &worldVisibility(Core &core) {
  return static_cast<C12Session &>(*core.game->sessionState).visibility;
}

} // namespace c12
