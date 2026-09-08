#pragma once

#include "psx_exe_image.h"

#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

namespace c12 {

struct ExecutableIdentity {
  std::string_view name;
  std::size_t size;
  std::string_view sha256;
};

struct AuthenticatedImage {
  std::vector<std::uint8_t> bytes;
  psx::cpu::PsxExeImage header;
};

// Both the probe and admission tests use this revision policy before any Core exists.
psx::cpu::PsxExeImage authenticateImage(std::span<const std::uint8_t> bytes, const ExecutableIdentity &identity);
AuthenticatedImage readAuthenticatedImage(const std::filesystem::path &path, const ExecutableIdentity &identity);

} // namespace c12
