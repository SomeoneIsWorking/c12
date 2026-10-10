#include "cell_output.h"

#include "core.h"
#include "title_facts.h"

#include <algorithm>
#include <utility>

namespace c12 {

namespace {

constexpr std::uint32_t kSlotCount = 0;
constexpr std::uint32_t kSlotSkipsTail = 4;
constexpr std::uint32_t kSlotTopY = 6;
constexpr std::uint32_t kSlotPolygonList = 8;
constexpr std::uint32_t kRecordFlags = 6;
constexpr std::uint8_t kRecordVisible = 1;

} // namespace

GuestCellOutput::GuestCellOutput(Core &core, const GuestCellWindow &window) : core_(core), window_(window) {
}

void GuestCellOutput::stage(const CellGroup &group) {
  const std::uint32_t slot = window_.slots + count_ * kCellGroupStride;
  core_.mem_w16(slot + kSlotCount, group.polygonCount);
  core_.mem_w16(slot + kSlotSkipsTail, group.skipsTail);
  core_.mem_w16(slot + kSlotTopY, group.topY);
  core_.mem_w32(slot + kSlotPolygonList, group.polygonList);
}

bool GuestCellOutput::commit(std::uint32_t record, const CellGroup &group) {
  if (group.polygonCount != 0) {
    core_.mem_w32(kVisibleCellListAddress + 4 * count_, record);
    core_.mem_w8(record + kRecordFlags,
                 static_cast<std::uint8_t>(core_.mem_r8(record + kRecordFlags) | kRecordVisible));
    ++count_;
  }
  return count_ >= window_.capacity;
}

void GuestCellOutput::finish() {
  core_.mem_w32(kVisibleCellListAddress + 4 * count_, 0);
}

void HostCellOutput::stage(const CellGroup &) {
}

bool HostCellOutput::commit(std::uint32_t, const CellGroup &group) {
  if (group.polygonCount != 0) {
    cells_.push_back(group);
  }
  return false;
}

void HostCellOutput::finish() {
}

void WorldVisibility::publish(std::vector<CellGroup> cells) {
  std::ranges::stable_sort(cells, {}, &CellGroup::distance);
  cells_ = std::move(cells);
  active_ = true;
}

void WorldVisibility::clear() {
  cells_.clear();
  active_ = false;
}

WorldVisibility &worldVisibility(Core &core) {
  return *static_cast<WorldVisibility *>(core.gameCtx);
}

} // namespace c12
