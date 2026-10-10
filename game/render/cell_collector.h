// cell_collector.h - the visible-cell collector (guest FUN_800684C0), ported with a scalable draw distance.
//
// Walks the view frustum's footprint on the ground plane and keeps the terrain cells whose box can reach the screen.
// At zero increase it produces the guest's exact buffers and GTE state; above zero a first pass collects the grown
// footprint into a host list, then the retail pass runs so every other guest reader sees the retail result.
// Evidence in docs/re-frontier.md (world.cell-collector, world.distance).

#pragma once

#include "cell_output.h"
#include "draw_distance.h"

#include <array>
#include <cstdint>

class Core;
class Game;

namespace c12 {

// True when a cell coordinate relative to the camera survives the GTE's signed 16-bit vertex input unchanged.
bool fitsGteInput(std::int32_t relative);

class CellCollector {
public:
  CellCollector(Core &core, const DrawDistance &distance, CellOutput &output, std::uint32_t capacity);

  // Collects for the camera object at `camera`; the guest's own return pair is left in v0 and v1.
  void run(std::uint32_t camera);

private:
  struct Corner {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t z = 0;
  };

  // A cell's footprint relative to the camera, in guest units.
  struct Box {
    std::int32_t x0 = 0;
    std::int32_t x1 = 0;
    std::int32_t z0 = 0;
    std::int32_t z1 = 0;
  };

  enum class Outcome : std::uint8_t { Next, EndRow, Full };

  void loadWorld();
  void measurePitch();
  void projectCorners();
  void clipCornersToGround();
  void anchorToGrid();
  void rasterizeFootprint();
  void prepareProjection();
  void scanRows();
  void resetRows(std::int32_t from);
  void resetRow(std::int32_t row);
  Outcome scanColumns(Box &box, std::int32_t row, std::int32_t first, std::int32_t last);
  bool boxVisible(std::uint32_t record, const Box &box);
  bool pitchedBoxVisible(std::uint32_t record, const Box &box);
  bool levelBoxVisible(std::uint32_t record, const Box &box);
  bool testable(const Box &box, std::int32_t y0, std::int32_t y1) const;
  Outcome collectCell(std::uint32_t record, const Box &box);
  bool tailReachesBelow(std::uint32_t record) const;
  void trimHead(CellGroup &group) const;
  std::int32_t polygonY(std::uint32_t polygonIndex) const;
  void projectBox(std::uint16_t y, const Box &box);
  std::uint32_t projectFourth(std::uint16_t y, const Box &box);
  void finish();

  Core &core_;
  DrawDistance distance_;
  CellOutput &output_;
  std::uint32_t capacity_;

  std::uint32_t matrix_ = 0;
  std::int32_t camX_ = 0;
  std::int32_t camY_ = 0;
  std::int32_t camZ_ = 0;
  std::uint32_t grid_ = 0;
  std::uint32_t records_ = 0;
  std::uint32_t polygons_ = 0;
  std::uint32_t vertices_ = 0;
  std::uint32_t frame_ = 0;

  std::int32_t pitch_ = 0;
  std::int32_t plane_ = 0;
  std::array<Corner, 4> corners_{};
  std::uint32_t highest_ = 0;
  std::uint32_t lowest_ = 0;
  std::int32_t spanHigh_ = 0;
  std::int32_t spanLow_ = 0;
  std::int32_t firstRow_ = 0;
  std::int32_t lastRow_ = 0;
  std::uint32_t cameraMask_ = 0;
};

// Replaces guest FUN_800684C0, the collector of the terrain pass.
void installCellCollectorOverride(Game &game);

void collectVisibleCells(Core *core);

} // namespace c12
