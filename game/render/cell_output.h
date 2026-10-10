// cell_output.h - where the visible-cell collector (guest FUN_800684C0) puts its cells.
//
// The guest keeps 150 twelve-byte cells and a zero-terminated pointer list in fixed buffers; the host list has no
// such limit. Evidence in docs/re-frontier.md (world.cell-collector).

#pragma once

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

class Core;

namespace c12 {

// One visible terrain cell: its polygon run, trimmed to the camera's vertical span.
struct CellGroup {
  std::uint16_t polygonCount = 0;
  std::uint16_t skipsTail = 0;
  std::uint16_t topY = 0;
  std::uint32_t polygonList = 0;
  // Squared horizontal distance from the camera to the cell centre, in guest units; the retail buffers ignore it.
  std::uint64_t distance = 0;
};

class CellOutput {
public:
  virtual ~CellOutput() = default;
  // The slot under construction; called again as the trims refine it.
  virtual void stage(const CellGroup &group) = 0;
  // Keeps the staged cell when it holds polygons; true once no further cell fits.
  virtual bool commit(std::uint32_t record, const CellGroup &group) = 0;
  virtual void finish() = 0;
  virtual std::uint32_t count() const = 0;
};

// The retail buffers: cell slots, the pointer list and the cell record's visible flag.
struct GuestCellWindow {
  std::uint32_t slots = 0;
  std::uint32_t capacity = 0;
};

class GuestCellOutput final : public CellOutput {
public:
  GuestCellOutput(Core &core, const GuestCellWindow &window);
  void stage(const CellGroup &group) override;
  bool commit(std::uint32_t record, const CellGroup &group) override;
  void finish() override;
  std::uint32_t count() const override {
    return count_;
  }

private:
  Core &core_;
  GuestCellWindow window_;
  std::uint32_t count_ = 0;
};

// An unbounded host list; the guest's flags and buffers stay untouched.
class HostCellOutput final : public CellOutput {
public:
  void stage(const CellGroup &group) override;
  bool commit(std::uint32_t record, const CellGroup &group) override;
  void finish() override;
  std::uint32_t count() const override {
    return static_cast<std::uint32_t>(cells_.size());
  }
  std::vector<CellGroup> release() {
    return std::move(cells_);
  }

private:
  std::vector<CellGroup> cells_;
};

// The extended cell list of the frame in flight, published by the collector and consumed by the terrain pass.
class WorldVisibility {
public:
  // Keeps `cells` nearest first, so a full packet pool drops the farthest terrain.
  void publish(std::vector<CellGroup> cells);
  void clear();
  // Records the increase in force; true when it differs from the previous frame's.
  bool noteDistance(long percent) {
    const bool changed = percent != percent_;
    percent_ = percent;
    return changed;
  }
  bool active() const {
    return active_;
  }
  std::span<const CellGroup> cells() const {
    return cells_;
  }

private:
  std::vector<CellGroup> cells_;
  bool active_ = false;
  long percent_ = -1;
};

// The world visibility of this core, created with its game context.
WorldVisibility &worldVisibility(Core &core);

} // namespace c12
