// cd_command_completion.h — C-12's CD command completion.
//
// Runs the guest CdReadySync body (0x800ABD98), then calls the command-status callback slot (0x800EEEB8).

#pragma once

class Core;
class Game;

namespace c12 {

// Installs the title's native overrides; call after the executable is mapped.
void installTitleOverrides(Game &game);

void completeCdReadySyncCommand(Core *core);

} // namespace c12
