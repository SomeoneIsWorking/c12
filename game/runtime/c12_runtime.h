#pragma once

#include "game_runtime.h"
#include "psx_exe_image.h"

namespace c12 {

// The initial direct runtime preserves the title's complete guest startup. No native
// CRT0, frame driver, or guessed library override replaces its unrecovered behavior.
class C12Runtime final : public GameRuntime {
public:
  explicit C12Runtime(const psx::cpu::PsxExeImage &header);
  void *createContext(Core &) override;
  void destroyContext(void *) override;
  void registerOverrides(Game &) override;
  void bootInit(Core &) override;
  const GuestProgramImage *guestProgramImage() const override;
  RenderCapabilities renderCapabilities() const override;
  bool guestVramIsPicture(const Game &) const override;

private:
  GuestProgramImage image_;
};

} // namespace c12
