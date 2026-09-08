#include "authenticated_image.h"

#include <fstream>
#include <lucent/content.h>
#include <stdexcept>
#include <string>

namespace c12 {

psx::cpu::PsxExeImage authenticateImage(std::span<const std::uint8_t> bytes, const ExecutableIdentity &identity) {
  if (bytes.size() != identity.size) {
    throw std::runtime_error(std::string(identity.name) + ": unexpected executable length");
  }
  const auto digest = lucent::content::sha256_hex(lucent::content::sha256(std::as_bytes(bytes)));
  if (digest != identity.sha256) {
    throw std::runtime_error(std::string(identity.name) + ": executable SHA-256 mismatch: " + digest);
  }
  const auto parsed = psx::cpu::parsePsxExeImage(bytes);
  if (!parsed) {
    throw std::runtime_error(std::string(identity.name) + ": " + parsed.detail);
  }
  return *parsed.image;
}

AuthenticatedImage readAuthenticatedImage(const std::filesystem::path &path, const ExecutableIdentity &identity) {
  if (identity.size > psx::cpu::kPsxExeMaxBytes) {
    throw std::runtime_error("title executable size exceeds the shared PS-X EXE bound");
  }
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("cannot open executable: " + path.string());
  }
  std::vector<std::uint8_t> bytes(identity.size + 1);
  input.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (input.bad()) {
    throw std::runtime_error("cannot read executable: " + path.string());
  }
  bytes.resize(static_cast<std::size_t>(input.gcount()));
  const auto header = authenticateImage(bytes, identity);
  return {std::move(bytes), header};
}

} // namespace c12
