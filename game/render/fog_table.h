// fog_table.h - the terrain pass's fade tables (guest TerrainFog), grown with the draw distance.
//
// The guest builds one shade word and one depth byte per 64 depth units (FUN_800675B8) and sizes them from level
// data. Stretching them by the draw distance moves the whole fade ramp out in proportion. Evidence in
// docs/re-frontier.md (world.distance).

#pragma once

#include "draw_distance.h"

#include <cstdint>
#include <span>
#include <vector>

class Core;

namespace c12 {

class FogTables {
public:
  FogTables() = default;
  // `shade` and `depthByte` are the guest's tables; each entry lands `distance` farther out.
  FogTables(std::span<const std::uint16_t> shade,
            std::span<const std::uint8_t> depthByte,
            const DrawDistance &distance);

  // Reads the guest's current tables; the depth bytes only when the level pages its palettes.
  static FogTables load(Core &core, bool paged, const DrawDistance &distance);

  // Zero past the table.
  std::uint16_t shade(std::uint32_t index) const;
  std::uint8_t depthByte(std::uint32_t index) const;

private:
  std::vector<std::uint16_t> shade_;
  std::vector<std::uint8_t> depthByte_;
};

} // namespace c12
