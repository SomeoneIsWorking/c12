#include "authenticated_image.h"
#include "title_identity.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <lucent/log.h>
#include <stdexcept>

namespace {

void require(bool condition, const char *detail) {
  if (!condition) {
    throw std::runtime_error(detail);
  }
}

template <typename Function> void requireRefusal(Function function, std::string_view message) {
  try {
    function();
  } catch (const std::runtime_error &error) {
    require(std::string_view(error.what()).find(message) != std::string_view::npos,
            "refusal did not identify the failed boundary");
    return;
  }
  throw std::runtime_error("invalid image was accepted");
}

std::vector<std::uint8_t> fixture() {
  std::vector<std::uint8_t> bytes(0x820);
  constexpr std::array<std::uint8_t, 8> magic{'P', 'S', '-', 'X', ' ', 'E', 'X', 'E'};
  std::copy(magic.begin(), magic.end(), bytes.begin());
  const auto word = [&bytes](std::size_t offset, std::uint32_t value) {
    for (unsigned byte = 0; byte < 4; ++byte) {
      bytes[offset + byte] = static_cast<std::uint8_t>(value >> (byte * 8));
    }
  };
  word(0x10, 0x80010000);
  word(0x18, 0x80010000);
  word(0x1c, 0x20);
  word(0x30, 0x801ffff0);
  word(0x800, 0x03e00008);
  return bytes;
}

void writeFixture(const std::filesystem::path &path, std::span<const std::uint8_t> bytes) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  output.close();
  require(static_cast<bool>(output), "cannot write the synthetic fixture");
}

void checkFileAdmission(const std::filesystem::path &directory,
                        const c12::ExecutableIdentity &identity,
                        std::vector<std::uint8_t> bytes) {
  std::filesystem::create_directories(directory);
  const auto path = directory / "synthetic.exe";
  writeFixture(path, bytes);
  const auto admitted = c12::readAuthenticatedImage(path, identity);
  require(admitted.bytes == bytes && admitted.header.entry == 0x80010000,
          "file admission did not retain the exact authenticated buffer");
  bytes.push_back(0);
  writeFixture(path, bytes);
  requireRefusal(
      [&] {
        c12::readAuthenticatedImage(path, identity);
      },
      "length");
  bytes.pop_back();
  bytes.back() = 1;
  writeFixture(path, bytes);
  requireRefusal(
      [&] {
        c12::readAuthenticatedImage(path, identity);
      },
      "SHA-256 mismatch");
  require(std::filesystem::remove(path), "cannot remove the synthetic fixture");
  requireRefusal(
      [&] {
        c12::readAuthenticatedImage(path, identity);
      },
      "cannot open");
}

} // namespace

int main(int argc, char **argv) {
  try {
    require(argc == 2, "provide the repository-scoped synthetic fixture directory");
    auto bytes = fixture();
    const c12::ExecutableIdentity fixtureIdentity{
        "synthetic", bytes.size(), "b68e2cd77fc22289d575428378196cfc6b66c2a8aaf1d0d49c84ba25dfefbaa9"};
    const auto parsed = c12::authenticateImage(bytes, fixtureIdentity);
    require(parsed.entry == 0x80010000 && parsed.physicalText.begin == 0x10000,
            "authenticated fixture lost its entry/load range");
    checkFileAdmission(argv[1], fixtureIdentity, bytes);
    requireRefusal(
        [&] {
          c12::authenticateImage(bytes, c12::kUsaIdentity);
        },
        "length");
    bytes[0x804] = 1;
    requireRefusal(
        [&] {
          c12::authenticateImage(bytes, fixtureIdentity);
        },
        "SHA-256 mismatch");
    bytes.pop_back();
    requireRefusal(
        [&] {
          c12::authenticateImage(bytes, fixtureIdentity);
        },
        "length");
    lucent::info("c12.image", "PASS: 8/8 span/file admission checks including changed, oversized and missing files");
    return 0;
  } catch (const std::exception &error) {
    lucent::error("c12.image", "FAIL: {}", error.what());
    return 1;
  }
}
