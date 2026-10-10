#include "cell_collector.h"

#include "core.h"
#include "game.h"
#include "gte_control.h"
#include "gte_registers.h"
#include "guest_call.h"
#include "native_dispatch.h"
#include "title_facts.h"

#include <lucent/log.h>

#include <algorithm>
#include <limits>
#include <string_view>

namespace c12 {

namespace {

constexpr std::string_view kOwnerName = "c12 cell collector";

constexpr std::uint32_t kV0 = 2;
constexpr std::uint32_t kV1 = 3;
constexpr std::uint32_t kA0 = 4;
constexpr std::uint32_t kA1 = 5;
constexpr std::uint32_t kA2 = 6;
constexpr std::uint32_t kStackPointer = 29;
constexpr std::uint32_t kScratchFrameBytes = 0x40;
// The guest prologue homes a1 in the caller frame.
constexpr std::uint32_t kArgumentHomeOffset = 4;
constexpr std::uint32_t kSavedRotationOffset = 0x10;

// Camera object words: the matrix pointer, and the screen offsets the render leaves for the next frame.
constexpr std::uint32_t kCameraMatrixPointer = 0x80;
constexpr std::uint32_t kCameraOffsetX = 0x68;
constexpr std::uint32_t kCameraOffsetY = 0x6A;

// Camera matrix: rotation words 0..4 (the yaw/pitch terms read below are halfwords), position words 5..7.
constexpr std::uint32_t kMatrixRotationWords = 5;
constexpr std::uint32_t kMatrixPositionX = 0x14;
constexpr std::uint32_t kMatrixPositionY = 0x18;
constexpr std::uint32_t kMatrixPositionZ = 0x1C;
constexpr std::uint32_t kMatrixForwardX = 0x04;
constexpr std::uint32_t kMatrixForwardZ = 0x10;
constexpr std::uint32_t kMatrixPitchTerm = 0x0A;

// World level: the cell tables at +0x10 (grid at +0xC, records at +0x10) and the polygon store at +0x18.
constexpr std::uint32_t kLevelCells = 0x10;
constexpr std::uint32_t kLevelStore = 0x18;
constexpr std::uint32_t kCellsGrid = 0xC;
constexpr std::uint32_t kCellsRecords = 0x10;
constexpr std::uint32_t kStorePolygons = 0x10;
constexpr std::uint32_t kStoreVertices = 0x14;

// Cell record (12 bytes): polygon count, vertical span, flags, mask byte and the polygon index run.
constexpr std::uint32_t kRecordCount = 0;
constexpr std::uint32_t kRecordTop = 2;
constexpr std::uint32_t kRecordBottom = 4;
constexpr std::uint32_t kRecordMask = 7;
constexpr std::uint32_t kRecordPolygons = 8;
constexpr std::uint32_t kPolygonStride = 16;
constexpr std::uint32_t kPolygonFlags = 0xC;
constexpr std::uint32_t kPolygonVertexSelect = 0x300;
constexpr std::uint32_t kVertexStride = 8;

constexpr std::uint32_t kHeaderTrimHead = 2;
constexpr std::uint32_t kHeaderTrimTail = 4;

// Cell size and the grid's origin offset, in guest units.
constexpr std::int32_t kCellShift = 10;
constexpr std::int32_t kCellSize = 1 << kCellShift;
constexpr std::int32_t kGridOrigin = 0x8000;
constexpr std::int32_t kCameraGridOrigin = 0x20;
constexpr std::int32_t kFootprintRows = 64;
constexpr std::int32_t kLastRow = 63;
constexpr std::uint32_t kGridRowStride = 64;
constexpr std::int32_t kPitchBand = 0x400;
constexpr std::int32_t kGroundShift = 8;
constexpr std::int32_t kGroundReach = 0x7FFF;
constexpr std::uint16_t kEmptyLast = 0x8000;
constexpr std::uint16_t kEmptyFirst = 0x7FFF;

// Scratchpad matrix the cell test projects with, and the constants that scale it.
constexpr std::uint32_t kProjectionMatrix = 0x1F800034;
constexpr std::uint32_t kProjectionTranslation = 0x1F800048;
constexpr std::uint16_t kUnit = 0x1000;
constexpr std::uint32_t kScaleX = 0x1999;
constexpr std::uint32_t kScaleY = 0x1111;
constexpr std::uint32_t kScaleZ = 0x1000;
constexpr std::uint32_t kScaleVectorStride = 8;
constexpr std::uint32_t kCellProjectionOffsetX = 0x1000000;
constexpr std::uint32_t kCellProjectionOffsetY = 0x800000;

// A box is off screen when every projected corner is past the same edge of the 512x256 window.
constexpr std::uint32_t kOutsideWindow = 0xFF00FE00u;
constexpr std::uint32_t kSignBits = 0x80008000u;

std::uint16_t halfword(std::int32_t value) {
  return static_cast<std::uint16_t>(value);
}

std::uint32_t pack(std::uint16_t x, std::uint16_t y) {
  return (static_cast<std::uint32_t>(y) << 16) | x;
}

std::int32_t wrapAdd(std::int32_t a, std::int32_t b) {
  return static_cast<std::int32_t>(static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(b));
}

std::int32_t wrapSub(std::int32_t a, std::int32_t b) {
  return static_cast<std::int32_t>(static_cast<std::uint32_t>(a) - static_cast<std::uint32_t>(b));
}

std::int32_t wrapMul(std::int32_t a, std::int32_t b) {
  return static_cast<std::int32_t>(static_cast<std::uint32_t>(a) * static_cast<std::uint32_t>(b));
}

// The twice-signed area of (b - a) x (c - a), exact.
std::int64_t
cross(std::int64_t ax, std::int64_t az, std::int64_t bx, std::int64_t bz, std::int64_t cx, std::int64_t cz) {
  return (bx - ax) * (cz - az) - (cx - ax) * (bz - az);
}

} // namespace

bool fitsGteInput(std::int32_t relative) {
  return relative >= std::numeric_limits<std::int16_t>::min() && relative <= std::numeric_limits<std::int16_t>::max();
}

CellCollector::CellCollector(Core &core, const DrawDistance &distance, CellOutput &output, std::uint32_t capacity)
    : core_(core), distance_(distance), output_(output), capacity_(capacity) {
}

void CellCollector::run(std::uint32_t camera) {
  matrix_ = core_.mem_r32(camera + kCameraMatrixPointer);
  camX_ = static_cast<std::int32_t>(core_.mem_r32(matrix_ + kMatrixPositionX));
  camY_ = static_cast<std::int32_t>(core_.mem_r32(matrix_ + kMatrixPositionY));
  camZ_ = static_cast<std::int32_t>(core_.mem_r32(matrix_ + kMatrixPositionZ));
  loadWorld();

  const std::uint32_t savedSp = core_.r[kStackPointer];
  frame_ = savedSp - kScratchFrameBytes;
  core_.r[kStackPointer] = frame_;
  measurePitch();
  projectCorners();
  clipCornersToGround();
  anchorToGrid();
  rasterizeFootprint();
  prepareProjection();
  scanRows();
  core_.r[kStackPointer] = savedSp;
  finish();
}

void CellCollector::loadWorld() {
  const std::uint32_t level = core_.mem_r32(kWorldLevelPointerAddress);
  const std::uint32_t cells = core_.mem_r32(level + kLevelCells);
  const std::uint32_t store = core_.mem_r32(level + kLevelStore);
  grid_ = core_.mem_r32(cells + kCellsGrid);
  records_ = core_.mem_r32(cells + kCellsRecords);
  polygons_ = core_.mem_r32(store + kStorePolygons);
  vertices_ = core_.mem_r32(store + kStoreVertices);
}

void CellCollector::measurePitch() {
  psx::cpu::callGuestNow(core_, kOwnerName, kReadRotationMatrixAddress, matrix_, frame_ + kSavedRotationOffset);
  const std::int32_t forwardX = core_.mem_r16s(matrix_ + kMatrixForwardX);
  const std::int32_t forwardZ = core_.mem_r16s(matrix_ + kMatrixForwardZ);
  psx::cpu::callGuestNow(
      core_, kOwnerName, kSquareRootAddress, wrapAdd(wrapMul(forwardX, forwardX), wrapMul(forwardZ, forwardZ)));
  const std::uint32_t horizontal = core_.r[kV0];
  psx::cpu::callGuestNow(core_,
                         kOwnerName,
                         kPitchAngleAddress,
                         static_cast<std::uint32_t>(core_.mem_r16s(matrix_ + kMatrixPitchTerm)),
                         horizontal);
  pitch_ = -static_cast<std::int32_t>(core_.r[kV0]);
  const std::int32_t band = core_.mem_r16s(kGroundBandAddress);
  const bool high = wrapSub(pitch_, band) >= 1;
  plane_ = static_cast<std::int32_t>(core_.mem_r32(high ? kGroundPlaneLowAddress : kGroundPlaneHighAddress));
}

void CellCollector::projectCorners() {
  for (std::uint32_t word = 0; word < kMatrixRotationWords; ++word) {
    gte_write_ctrl(psx::gte::kRotation + word, core_.mem_r32(matrix_ + 4 * word));
  }
  std::int32_t highestY = std::numeric_limits<std::int32_t>::min();
  std::int32_t lowestY = std::numeric_limits<std::int32_t>::max();
  highest_ = 0;
  lowest_ = 0;
  for (std::uint32_t k = 0; k < kFrustumCornerCount; ++k) {
    const std::uint32_t entry = kViewFrustumCornerTableAddress + k * kFrustumCornerStride;
    gte_write_data(psx::gte::kVxy0, core_.mem_r32(entry));
    gte_write_data(psx::gte::kVz0, core_.mem_r32(entry + 4));
    gte_op(&core_, psx::gte::kMvmvaRtV0);
    Corner &corner = corners_[k];
    corner.x = wrapAdd(
        static_cast<std::int32_t>(distance_.scale(static_cast<std::int32_t>(gte_read_data(psx::gte::kMac1)))), camX_);
    corner.y = wrapAdd(
        static_cast<std::int32_t>(distance_.scale(static_cast<std::int32_t>(gte_read_data(psx::gte::kMac2)))), camY_);
    corner.z = wrapAdd(
        static_cast<std::int32_t>(distance_.scale(static_cast<std::int32_t>(gte_read_data(psx::gte::kMac3)))), camZ_);
    if (highestY < corner.y) {
      highestY = corner.y;
      highest_ = k;
    }
    if (corner.y < lowestY) {
      lowestY = corner.y;
      lowest_ = k;
    }
  }
  for (std::uint32_t word = 0; word < kMatrixRotationWords; ++word) {
    gte_write_ctrl(psx::gte::kRotation + word, core_.mem_r32(frame_ + kSavedRotationOffset + 4 * word));
  }
}

void CellCollector::clipCornersToGround() {
  const std::int64_t limit =
      std::min<std::int64_t>(distance_.scale(static_cast<std::int32_t>(core_.mem_r32(kGroundDistanceLimitAddress))),
                             std::numeric_limits<std::int32_t>::max());
  const std::int32_t height = wrapSub(plane_, camY_);
  for (Corner &corner : corners_) {
    const std::int32_t dx = wrapSub(corner.x, camX_);
    const std::int32_t dy = wrapSub(corner.y, camY_);
    const std::int32_t dz = wrapSub(corner.z, camZ_);
    if (dy == 0) {
      continue;
    }
    const std::int64_t scaledHeight = static_cast<std::int32_t>(static_cast<std::uint32_t>(height) << kGroundShift);
    const std::int32_t toGround = static_cast<std::int32_t>(scaledHeight / dy);
    const std::int32_t x = wrapMul(toGround, dx) >> kGroundShift;
    const std::int32_t z = wrapMul(toGround, dz) >> kGroundShift;
    psx::cpu::callGuestNow(
        core_, kOwnerName, kSquareRootAddress, wrapAdd(wrapAdd(wrapMul(x, x), wrapMul(height, height)), wrapMul(z, z)));
    const std::int32_t distance = static_cast<std::int32_t>(core_.r[kV0]);
    if (distance >= limit || x > kGroundReach || x < -kGroundReach || z > kGroundReach || z < -kGroundReach) {
      continue;
    }
    gte_write_data(psx::gte::kVxy0, pack(halfword(x), halfword(height)));
    gte_write_data(psx::gte::kVz0, halfword(z));
    gte_op(&core_, psx::gte::kMvmvaRtV0);
    if (static_cast<std::int32_t>(gte_read_data(psx::gte::kMac3)) >= 0) {
      corner.x = wrapAdd(x, camX_);
      corner.y = plane_;
      corner.z = wrapAdd(z, camZ_);
    }
  }
  spanHigh_ = std::max(camY_, corners_[highest_].y);
  spanLow_ = std::min(camY_, corners_[lowest_].y);
}

void CellCollector::anchorToGrid() {
  for (Corner &corner : corners_) {
    corner.y = 0;
    corner.x = wrapAdd(corner.x, kGridOrigin);
    corner.z = wrapAdd(corner.z, kGridOrigin);
  }
  const std::int32_t cameraCellX = (camX_ >> kCellShift) + kCameraGridOrigin;
  const std::int32_t cameraCellZ = (camZ_ >> kCellShift) + kCameraGridOrigin;
  const std::int32_t cameraCell = core_.mem_r16s(grid_ + 2 * (cameraCellZ * kGridRowStride + cameraCellX));
  cameraMask_ = cameraCell < 0 ? 0 : core_.mem_r8(records_ + cameraCell * kCellGroupStride + kRecordMask);
}

void CellCollector::rasterizeFootprint() {
  const std::int64_t px = static_cast<std::int64_t>(camX_) + kGridOrigin;
  const std::int64_t pz = static_cast<std::int64_t>(camZ_) + kGridOrigin;
  for (std::uint32_t k = 0; k < kFrustumCornerCount; ++k) {
    const Corner &from = corners_[k];
    const Corner &to = corners_[(k + 1) % kFrustumCornerCount];
    psx::cpu::callGuestNow(core_, kOwnerName, kFootprintEdgeAddress, from.x, from.z, to.x, to.z);
  }
  std::int64_t lowZ = corners_[0].z;
  std::int64_t highZ = corners_[0].z;
  for (const Corner &corner : corners_) {
    lowZ = std::min<std::int64_t>(lowZ, corner.z);
    highZ = std::max<std::int64_t>(highZ, corner.z);
  }
  bool outside = false;
  for (std::uint32_t k = 0; k < kFrustumCornerCount && !outside; ++k) {
    const Corner &a = corners_[k];
    const Corner &b = corners_[(k + 1) % kFrustumCornerCount];
    outside = cross(a.x, a.z, b.x, b.z, px, pz) > 0;
  }
  if (outside) {
    std::array<std::int64_t, kFrustumCornerCount> side{};
    for (std::uint32_t k = 0; k < kFrustumCornerCount; ++k) {
      const Corner &a = corners_[k];
      const Corner &b = corners_[(k + 1) % kFrustumCornerCount];
      side[k] = (a.x - px) * (b.z - pz) - (a.z - pz) * (b.x - px);
    }
    for (std::uint32_t k = 0; k < kFrustumCornerCount; ++k) {
      const std::int64_t here = side[k];
      const std::int64_t next = side[(k + 1) % kFrustumCornerCount];
      if ((here < 1 || next < 1) && (here > -1 || next > -1)) {
        const Corner &to = corners_[(k + 1) % kFrustumCornerCount];
        psx::cpu::callGuestNow(core_, kOwnerName, kFootprintEdgeAddress, to.x, to.z, px, pz);
      }
    }
    lowZ = std::min(lowZ, pz);
    highZ = std::max(highZ, pz);
  }
  gte_write_ctrl(psx::gte::kH, static_cast<std::uint32_t>(core_.mem_r16s(kZoneProjectionPlaneAddress)));
  firstRow_ = static_cast<std::int32_t>(std::max<std::int64_t>(lowZ >> kCellShift, 0));
  lastRow_ = static_cast<std::int32_t>(std::min<std::int64_t>(highZ >> kCellShift, kLastRow));
}

void CellCollector::prepareProjection() {
  core_.mem_w32(kProjectionMatrix, kUnit);
  core_.mem_w32(kProjectionMatrix + 4, 0);
  core_.mem_w32(kProjectionMatrix + 8, kUnit);
  core_.mem_w32(kProjectionMatrix + 12, 0);
  core_.mem_w16(kProjectionMatrix + 16, kUnit);
  core_.mem_w32(kCellTestScaleMatrixAddress, kScaleX);
  core_.mem_w32(kCellTestScaleMatrixAddress + kScaleVectorStride, kScaleY);
  core_.mem_w32(kCellTestScaleMatrixAddress + 2 * kScaleVectorStride, kScaleZ);
  psx::cpu::callGuestNow(core_, kOwnerName, kScaleMatrixAddress, kCellTestScaleMatrixAddress, kProjectionMatrix);
  psx::cpu::callGuestNow(core_, kOwnerName, kMultiplyMatrixAddress, kProjectionMatrix, frame_ + kSavedRotationOffset);
  for (std::uint32_t word = 0; word < 3; ++word) {
    core_.mem_w32(kProjectionTranslation + 4 * word, 0);
  }
  for (std::uint32_t word = 0; word < kMatrixRotationWords; ++word) {
    gte_write_ctrl(psx::gte::kRotation + word, core_.mem_r32(kProjectionMatrix + 4 * word));
  }
  for (std::uint32_t word = 0; word < 3; ++word) {
    gte_write_ctrl(psx::gte::kTranslationX + word, 0);
  }
  gte_write_ctrl(psx::gte::kOfx, kCellProjectionOffsetX);
  gte_write_ctrl(psx::gte::kOfy, kCellProjectionOffsetY);
}

void CellCollector::scanRows() {
  Box box;
  for (std::int32_t row = firstRow_; row <= lastRow_; ++row) {
    const std::int32_t first = core_.mem_r16s(kFootprintFirstColumnAddress + 2 * row);
    const std::int32_t last = core_.mem_r16s(kFootprintLastColumnAddress + 2 * row);
    const std::int32_t from = first >= 0 ? first : 0;
    const std::int32_t to = last < static_cast<std::int32_t>(kGridDimension) ? last : kLastRow;
    box.z0 = row * kCellSize - kGridOrigin - camZ_;
    box.z1 = box.z0 + kCellSize;
    if (from <= to && scanColumns(box, row, from, to) == Outcome::Full) {
      resetRows(row);
      return;
    }
    resetRow(row);
  }
}

void CellCollector::resetRows(std::int32_t from) {
  for (std::int32_t row = from; row <= lastRow_; ++row) {
    resetRow(row);
  }
}

void CellCollector::resetRow(std::int32_t row) {
  core_.mem_w16(kFootprintLastColumnAddress + 2 * row, kEmptyLast);
  core_.mem_w16(kFootprintFirstColumnAddress + 2 * row, kEmptyFirst);
}

CellCollector::Outcome CellCollector::scanColumns(Box &box, std::int32_t row, std::int32_t first, std::int32_t last) {
  for (std::int32_t column = first; column <= last; ++column) {
    box.x0 = column * kCellSize - kGridOrigin - camX_;
    box.x1 = box.x0 + kCellSize;
    const std::int32_t index = core_.mem_r16s(grid_ + 2 * (row * kGridRowStride + column));
    if (index < 0) {
      continue;
    }
    const std::uint32_t record = records_ + index * kCellGroupStride;
    if (cameraMask_ != 0 && (core_.mem_r8(record + kRecordMask) & cameraMask_) == 0) {
      continue;
    }
    if (!boxVisible(record, box)) {
      continue;
    }
    const Outcome outcome = collectCell(record, box);
    if (outcome != Outcome::Next) {
      return outcome;
    }
  }
  return Outcome::Next;
}

bool CellCollector::testable(const Box &box, std::int32_t y0, std::int32_t y1) const {
  return !distance_.increased() || (fitsGteInput(box.x0) && fitsGteInput(box.x1) && fitsGteInput(box.z0) &&
                                    fitsGteInput(box.z1) && fitsGteInput(y0) && fitsGteInput(y1));
}

void CellCollector::projectBox(std::uint16_t y, const Box &box) {
  gte_write_data(psx::gte::kVxy0, pack(halfword(box.x0), y));
  gte_write_data(psx::gte::kVz0, halfword(box.z0));
  gte_write_data(psx::gte::kVxy1, pack(halfword(box.x0), y));
  gte_write_data(psx::gte::kVz1, halfword(box.z1));
  gte_write_data(psx::gte::kVxy2, pack(halfword(box.x1), y));
  gte_write_data(psx::gte::kVz2, halfword(box.z1));
  gte_op(&core_, psx::gte::kRtpt);
}

std::uint32_t CellCollector::projectFourth(std::uint16_t y, const Box &box) {
  gte_write_data(psx::gte::kVxy0, pack(halfword(box.x1), y));
  gte_write_data(psx::gte::kVz0, halfword(box.z0));
  gte_op(&core_, psx::gte::kRtps);
  return gte_read_data(psx::gte::kSxy2);
}

bool CellCollector::boxVisible(std::uint32_t record, const Box &box) {
  const std::int32_t band = core_.mem_r16s(kGroundBandAddress);
  const bool pitched = pitch_ - band < -kPitchBand || kPitchBand < pitch_ + band;
  return pitched ? pitchedBoxVisible(record, box) : levelBoxVisible(record, box);
}

bool CellCollector::pitchedBoxVisible(std::uint32_t record, const Box &box) {
  const std::int32_t top = core_.mem_r16s(record + kRecordTop);
  const std::int32_t bottom = core_.mem_r16s(record + kRecordBottom);
  std::int32_t y = bottom;
  bool onScreen = false;
  if (pitch_ < 1) {
    if (top > spanHigh_) {
      return false;
    }
    onScreen = spanLow_ < bottom;
  } else {
    if (spanLow_ > bottom) {
      return false;
    }
    y = top;
    onScreen = top < spanHigh_;
  }
  if (onScreen || !testable(box, y - camY_, y - camY_)) {
    return true;
  }
  const std::uint16_t relative = halfword(y - camY_);
  projectBox(relative, box);
  const std::uint32_t outside =
      gte_read_data(psx::gte::kSxy0) & gte_read_data(psx::gte::kSxy1) & gte_read_data(psx::gte::kSxy2) & kOutsideWindow;
  if (outside == 0) {
    return true;
  }
  return (outside & projectFourth(relative, box)) == 0;
}

bool CellCollector::levelBoxVisible(std::uint32_t record, const Box &box) {
  const std::int32_t top = core_.mem_r16s(record + kRecordTop) - camY_;
  const std::int32_t bottom = core_.mem_r16s(record + kRecordBottom) - camY_;
  if (!testable(box, top, bottom)) {
    return true;
  }
  projectBox(halfword(top), box);
  const std::uint32_t topA = gte_read_data(psx::gte::kSxy0);
  const std::uint32_t topB = gte_read_data(psx::gte::kSxy1);
  const std::uint32_t topC = gte_read_data(psx::gte::kSxy2);
  const std::uint32_t topD = projectFourth(halfword(top), box);
  const std::uint32_t topAnd = topA & topB & topC & topD;
  if ((topAnd & kOutsideWindow) == 0) {
    return true;
  }
  projectBox(halfword(bottom), box);
  const std::uint32_t bottomA = gte_read_data(psx::gte::kSxy0);
  const std::uint32_t bottomB = gte_read_data(psx::gte::kSxy1);
  const std::uint32_t bottomC = gte_read_data(psx::gte::kSxy2);
  const std::uint32_t bottomD = projectFourth(halfword(bottom), box);
  const std::uint32_t shared = topAnd & bottomA & bottomB & bottomC & bottomD;
  const std::uint32_t every = topA | topB | topC | topD | bottomA | bottomB | bottomC | bottomD;
  const bool rejected = (shared & kOutsideWindow) != 0 && ((shared & kSignBits) != 0 || (every & kSignBits) == 0);
  return !rejected;
}

std::int32_t CellCollector::polygonY(std::uint32_t polygonIndex) const {
  const std::uint32_t polygon = polygons_ + polygonIndex * kPolygonStride;
  const std::uint32_t select = (core_.mem_r32(polygon + kPolygonFlags) & kPolygonVertexSelect) >> 7;
  const std::uint32_t vertex = core_.mem_r16(polygon + select);
  return core_.mem_r16s(vertices_ + vertex * kVertexStride + 2);
}

void CellCollector::trimHead(CellGroup &group) const {
  std::uint16_t remaining = group.polygonCount;
  while (remaining != 0 && polygonY(core_.mem_r16(group.polygonList)) < spanLow_) {
    --remaining;
    group.polygonList += 2;
  }
  group.polygonCount = remaining;
}

bool CellCollector::tailReachesBelow(std::uint32_t record) const {
  const std::uint32_t count = core_.mem_r16(record + kRecordCount);
  const std::uint32_t list = core_.mem_r32(record + kRecordPolygons);
  return spanHigh_ < polygonY(core_.mem_r16(list + 2 * count - 2));
}

CellCollector::Outcome CellCollector::collectCell(std::uint32_t record, const Box &box) {
  CellGroup group;
  group.polygonCount = static_cast<std::uint16_t>(core_.mem_r16(record + kRecordCount));
  group.topY = halfword(spanHigh_);
  group.polygonList = core_.mem_r32(record + kRecordPolygons);
  const std::int64_t centreX = static_cast<std::int64_t>(box.x0) + kCellSize / 2;
  const std::int64_t centreZ = static_cast<std::int64_t>(box.z0) + kCellSize / 2;
  group.distance = static_cast<std::uint64_t>(centreX * centreX + centreZ * centreZ);
  output_.stage(group);
  const std::uint32_t flags = core_.mem_r16(kWorldHeaderFlagsAddress);
  if ((flags & kHeaderTrimHead) != 0 && core_.mem_r16s(record + kRecordTop) < spanLow_) {
    trimHead(group);
    output_.stage(group);
  }
  if ((flags & kHeaderTrimTail) != 0 && spanHigh_ < core_.mem_r16s(record + kRecordBottom)) {
    if (!tailReachesBelow(record)) {
      return Outcome::EndRow;
    }
    group.skipsTail = 1;
    output_.stage(group);
  }
  return output_.commit(record, group) ? Outcome::Full : Outcome::Next;
}

void CellCollector::finish() {
  output_.finish();
  const std::uint32_t camera = core_.mem_r32(kCameraPointerAddress);
  const std::uint32_t offsetX = core_.mem_r16(camera + kCameraOffsetX);
  gte_write_ctrl(psx::gte::kOfx, offsetX << 16);
  gte_write_ctrl(psx::gte::kOfy, core_.mem_r16(camera + kCameraOffsetY) << 16);
  core_.r[kV0] = output_.count();
  core_.r[kV1] = offsetX;
}

void installCellCollectorOverride(Game &game) {
  psx::cpu::installNativeOverride(game.core, kCellCollectorAddress, "c12::collectVisibleCells", collectVisibleCells);
  lucent::info("c12-draw", "installed visible cell collector at 0x{:08X}", kCellCollectorAddress);
}

void collectVisibleCells(Core *host) {
  Core &core = *host;
  const DrawDistance distance = currentDrawDistance();
  const std::uint32_t camera = core.r[kA0];
  const std::uint32_t capacity = core.r[kA1];
  const std::uint32_t slots = core.r[kA2];
  core.mem_w32(core.r[kStackPointer] + kArgumentHomeOffset, capacity);
  WorldVisibility &visibility = worldVisibility(core);
  visibility.clear();
  if (visibility.noteDistance(distance.percent())) {
    lucent::info("c12-draw", "draw distance increase {}%", distance.percent());
  }
  if (distance.increased()) {
    HostCellOutput extended;
    {
      psx::present::GteGuard gte;
      CellCollector(core, distance, extended, std::numeric_limits<std::uint32_t>::max()).run(camera);
    }
    visibility.publish(extended.release());
  }
  GuestCellOutput retail(core, {slots, capacity});
  CellCollector(core, DrawDistance(0), retail, capacity).run(camera);
}

} // namespace c12
