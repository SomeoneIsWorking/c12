#include "fog_table.h"

#include "core.h"
#include "title_facts.h"

#include <algorithm>

namespace c12 {

namespace {

// The shade tables hold 16-bit entries copied in blocks of eight, the depth bytes in blocks of sixteen.
constexpr std::uint32_t kShadeEntriesPerBlock = 8;
constexpr std::uint32_t kByteEntriesPerBlock = 16;
// Level data counts the real entries; the guest pads 0x40 more.
constexpr std::uint32_t kPaddingEntries = 0x40;
constexpr std::uint32_t kEntryCountOffset = 0xC;
constexpr std::uint32_t kShadeTableOffset = 4;
constexpr std::uint32_t kByteTableOffset = 8;

// Depth is a 16-bit z, so the tables are indexed up to 0xFFFF >> 6.
constexpr std::size_t kDepthIndexCount = 0x400;

// The source followed by zeros, stretched by the increase, long enough to index any depth.
template <typename T> std::vector<T> stretch(std::span<const T> source, const DrawDistance &distance) {
  if (source.empty()) {
    return {};
  }
  const std::size_t grown = static_cast<std::size_t>(distance.scaleUp(static_cast<std::int64_t>(source.size())));
  std::vector<T> out(std::max(grown, kDepthIndexCount));
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::size_t from = static_cast<std::size_t>(distance.unscale(static_cast<std::int64_t>(i)));
    out[i] = from < source.size() ? source[from] : T{};
  }
  return out;
}

template <typename T> T entry(const std::vector<T> &table, std::uint32_t index) {
  return index < table.size() ? table[index] : T{};
}

} // namespace

FogTables::FogTables(std::span<const std::uint16_t> shade,
                     std::span<const std::uint8_t> depthByte,
                     const DrawDistance &distance)
    : shade_(stretch(shade, distance)), depthByte_(stretch(depthByte, distance)) {
}

FogTables FogTables::load(Core &core, bool paged, const DrawDistance &distance) {
  const std::uint32_t byteTable = core.mem_r32(kWorldShadeTablesAddress + kByteTableOffset);
  const std::uint32_t shadeTable = core.mem_r32(kWorldShadeTablesAddress + kShadeTableOffset);
  const std::uint32_t entries = core.mem_r16(kWorldShadeTablesAddress + kEntryCountOffset) + kPaddingEntries;
  std::vector<std::uint8_t> bytes;
  if (paged) {
    bytes.resize(static_cast<std::size_t>(std::max(entries / kByteEntriesPerBlock, 1u)) * kByteEntriesPerBlock);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
      bytes[i] = core.mem_r8(byteTable + static_cast<std::uint32_t>(i));
    }
  }
  std::vector<std::uint16_t> shades(static_cast<std::size_t>(std::max(entries / kShadeEntriesPerBlock, 1u)) *
                                    kShadeEntriesPerBlock);
  for (std::size_t i = 0; i < shades.size(); ++i) {
    shades[i] = core.mem_r16(shadeTable + 2 * static_cast<std::uint32_t>(i));
  }
  return FogTables(shades, bytes, distance);
}

std::uint16_t FogTables::shade(std::uint32_t index) const {
  return entry(shade_, index);
}

std::uint8_t FogTables::depthByte(std::uint32_t index) const {
  return entry(depthByte_, index);
}

} // namespace c12
