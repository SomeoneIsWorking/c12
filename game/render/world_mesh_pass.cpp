#include "world_mesh_pass.h"

#include "cell_output.h"
#include "core.h"
#include "draw_distance.h"
#include "fog_table.h"
#include "game.h"
#include "guest_widescreen_projection.h"
#include "native_dispatch.h"
#include "title_facts.h"

#include <lucent/log.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <span>
#include <vector>

namespace c12 {

namespace {

// MIPS o32 registers the guest leaves behind.
constexpr std::uint32_t kV0 = 2;
constexpr std::uint32_t kV1 = 3;
constexpr std::uint32_t kA0 = 4;
constexpr std::uint32_t kFirstSaved = 16;
constexpr std::uint32_t kSavedCount = 8;
constexpr std::uint32_t kFramePointer = 30;

constexpr std::uint32_t kScratch = 0x1F800000u;

// Scratchpad slots the pass keeps its working state in.
constexpr std::uint32_t kSlotDepths = 0x20;
constexpr std::uint32_t kSlotHeader = 0x30;
constexpr std::uint32_t kSlotGroupsLeft = 0x34;
constexpr std::uint32_t kSlotPolysLeft = 0x38;
constexpr std::uint32_t kSlotFlags = 0x3C;
constexpr std::uint32_t kSlotAttributeTable = 0x40;
constexpr std::uint32_t kSlotAttribute = 0x44;
constexpr std::uint32_t kSlotFirstScreen = 0x48;
constexpr std::uint32_t kSlotClut = 0x4C;
constexpr std::uint32_t kSlotFarColor = 0x50;
constexpr std::uint32_t kSlotOtShift = 0x64;
constexpr std::uint32_t kSlotOtLength = 0x66;
constexpr std::uint32_t kSlotOtBase = 0x68;
constexpr std::uint32_t kSlotPacket = 0x6C;
constexpr std::uint32_t kSlotPacketLimit = 0x70;

// Header words (kWorldHeaderAddress).
constexpr std::uint32_t kHeaderModeFlags = 0x04;
constexpr std::uint32_t kHeaderLimits = 0x08;
constexpr std::uint32_t kHeaderFar = 0x54;
constexpr std::uint32_t kHeaderVisited = 0x88;
constexpr std::uint32_t kHeaderGroupCount = 0x90;
constexpr std::uint32_t kHeaderGroups = 0x1B4;
constexpr std::uint32_t kGroupStride = 0xC;

// Polygon record flags and fields.
constexpr std::uint32_t kPolyQuad = 0x1;
constexpr std::uint32_t kPolyVisited = 0x2;
constexpr std::uint32_t kPolySkipMask = 0xC02;
constexpr std::uint32_t kPolyDepthModeMask = 0xC;
constexpr std::uint32_t kPolyDepthFirst = 0x4;
constexpr std::uint32_t kPolyDepthLast = 0x8;
constexpr std::uint32_t kPolySemiTransparent = 0x10;
constexpr std::uint32_t kHeaderPaged = 0x1;

// GTE data registers and operations.
constexpr std::uint32_t kGteVxy0 = 0;
constexpr std::uint32_t kGteVz0 = 1;
constexpr std::uint32_t kGteRgbc = 6;
constexpr std::uint32_t kGteOtz = 7;
constexpr std::uint32_t kGteIr0 = 8;
constexpr std::uint32_t kGteIr1 = 9;
constexpr std::uint32_t kGteIr2 = 10;
constexpr std::uint32_t kGteSxy0 = 12;
constexpr std::uint32_t kGteSxy1 = 13;
constexpr std::uint32_t kGteSxy2 = 14;
constexpr std::uint32_t kGteSz0 = 16;
constexpr std::uint32_t kGteSz1 = 17;
constexpr std::uint32_t kGteSz2 = 18;
constexpr std::uint32_t kGteSz3 = 19;
constexpr std::uint32_t kGteRgb2 = 22;
constexpr std::uint32_t kGteMac0 = 24;
constexpr std::uint32_t kGteIrgb = 28;
constexpr std::uint32_t kGteFarColor = 21;
constexpr std::uint32_t kRtps = 0x4A180001u;
constexpr std::uint32_t kRtpt = 0x4A280030u;
constexpr std::uint32_t kNclip = 0x4B400006u;
constexpr std::uint32_t kAvsz3 = 0x4B58002Du;
constexpr std::uint32_t kAvsz4 = 0x4B68002Eu;
constexpr std::uint32_t kDpcl = 0x4A680029u;
constexpr std::uint32_t kIntpl = 0x4A980011u;

constexpr std::uint32_t kInitialRgbc = 0x00F8F8F8u;
constexpr std::int32_t kMinimumDepth = 4;
constexpr std::int32_t kPacketHeadroomWords = 0x520;
constexpr std::int32_t kOtSlackSlots = 3;
constexpr std::uint32_t kAddressMask = 0x00FFFFFFu;
constexpr std::uint32_t kQuadCode = 0x3C000000u;
constexpr std::uint32_t kTriCode = 0x34000000u;
constexpr std::uint32_t kSemiTransparentBit = 0x02000000u;
constexpr std::uint32_t kFlatQuadLink = 0x0C000000u;
constexpr std::uint32_t kFlatTriLink = 0x09000000u;
constexpr std::uint32_t kFlatQuadBytes = 0x34;
constexpr std::uint32_t kFlatTriBytes = 0x28;

struct PacketShape {
  std::uint32_t link;
  std::uint32_t stride;
};
constexpr PacketShape kFlatQuadShape{kFlatQuadLink, kFlatQuadBytes};
constexpr PacketShape kFlatTriShape{kFlatTriLink, kFlatTriBytes};
constexpr std::uint32_t kRefineAreaLimit = 0x1000;

// Subdivision work vertices.
constexpr std::size_t kWorkVertices = 32;

std::int32_t sx(std::uint32_t xy) {
  return static_cast<std::int16_t>(xy & 0xFFFFu);
}

std::int32_t sy(std::uint32_t xy) {
  return static_cast<std::int32_t>(xy) >> 16;
}

// abs() with the guest's xor/subtract wrap.
std::int32_t wrapAbs(std::int32_t value) {
  const std::uint32_t sign = static_cast<std::uint32_t>(value >> 31);
  return static_cast<std::int32_t>((static_cast<std::uint32_t>(value) ^ sign) - sign);
}

struct WorkVertex {
  std::uint32_t color = 0;
  std::uint32_t screen = 0;
  std::uint32_t model = 0;
  std::uint32_t depthUv = 0;
};

enum class Flow : std::uint8_t { Next, Stop };

// What one stage of a polygon decided.
enum class Step : std::uint8_t { Accept, Reject, Stop };

class WorldMeshPass {
public:
  WorldMeshPass(Core &core, std::int32_t margin, const DrawDistance &distance)
      : core_(core), margin_(margin), distance_(distance) {
  }

  void run(std::uint32_t level);

private:
  std::uint32_t scratch(std::uint32_t slot) {
    return core_.mem_r32(kScratch + slot);
  }

  void setScratch(std::uint32_t slot, std::uint32_t value) {
    core_.mem_w32(kScratch + slot, value);
  }

  void saveCalleeRegisters();
  void loadTables(std::uint32_t level);
  Flow runGroups();
  Flow runPolygons(std::uint32_t polyList);
  Flow visitPolygon(std::uint32_t poly);
  Step projectPolygon();
  Step rejectByDepth();
  Step rejectByScreen();
  void prepareAttributes();
  void writeFlatTexture();
  void shadeVertices();
  void writeFlatPositions();
  void emitFlat();
  void emitSubdivided();
  void chooseSubdivision();
  std::uint16_t buildSubdivisionVertex(std::uint32_t index, std::uint32_t &pairPtr, WorkVertex &vertex);
  void writeSubdivisionSlots(std::uint32_t slotPtr, const WorkVertex &vertex, std::uint16_t uv);
  void linkSubdivided();
  void finish();

  Core &core_;
  std::int32_t margin_;
  DrawDistance distance_;

  std::uint32_t header_ = kWorldHeaderAddress;
  std::uint32_t verts_ = 0;
  std::uint32_t polys_ = 0;
  std::uint32_t uvTable_ = 0;
  std::uint32_t visitedList_ = 0;
  std::int32_t polygonLimit_ = 0;
  std::uint32_t farWord_ = 0;
  std::uint32_t limitWord_ = 0;
  std::uint32_t threshold_ = 0;
  std::uint32_t packet_ = 0;
  FogTables fog_;
  std::span<const CellGroup> hostCells_;
  std::vector<std::uint32_t> visited_;

  // The polygon in flight.
  std::uint32_t poly_ = 0;
  bool quad_ = false;
  std::uint32_t flags_ = 0;
  std::int32_t area_ = 0;
  std::int32_t depth_ = 0;
  std::int32_t rawDepth_ = 0;
  std::int32_t otSlot_ = 0;
  std::uint32_t subdivision_ = 0;
  std::uint32_t fine_ = 0;
  std::uint32_t attribute_ = 0;
  std::uint32_t clutWord_ = 0;
  std::uint16_t texturePage_ = 0;
  std::array<std::uint16_t, 4> shadeIndex_{};
  std::array<std::uint32_t, 4> uv_{};
  std::array<std::uint32_t, 4> colors_{};
  std::array<std::uint32_t, 4> screen_{};
  std::array<WorkVertex, kWorkVertices> work_{};

  // Subdivision plan.
  std::uint32_t pairs_ = 0;
  std::uint32_t slots_ = 0;
  std::uint32_t counts_ = 0;
  std::uint32_t codeByte_ = 0;
  std::uint32_t totalVertices_ = 0;
  std::uint32_t storedVertices_ = 0;
};

void WorldMeshPass::saveCalleeRegisters() {
  for (std::uint32_t i = 0; i < kSavedCount; ++i) {
    core_.mem_w32(kWorldPassSaveAreaAddress + 4 * i, core_.r[kFirstSaved + i]);
  }
  core_.mem_w32(kWorldPassSaveAreaAddress + 4 * kSavedCount, core_.r[kFramePointer]);
}

void WorldMeshPass::run(std::uint32_t level) {
  saveCalleeRegisters();
  gte_write_data(kGteRgbc, kInitialRgbc);
  WorldVisibility &visibility = worldVisibility(core_);
  if (visibility.active()) {
    hostCells_ = visibility.cells();
  }
  const std::int32_t groups = visibility.active()
                                  ? static_cast<std::int32_t>(hostCells_.size())
                                  : static_cast<std::int32_t>(core_.mem_r32(header_ + kHeaderGroupCount));
  if (groups > 0) {
    loadTables(level);
    setScratch(kSlotHeader, header_);
    setScratch(kSlotGroupsLeft, static_cast<std::uint32_t>(groups));
    runGroups();
  }
  visibility.clear();
  finish();
}

void WorldMeshPass::loadTables(std::uint32_t level) {
  verts_ = core_.mem_r32(level + 0x14);
  polys_ = core_.mem_r32(level + 0x10);
  uvTable_ = core_.mem_r32(level + 0x18);
  visitedList_ = core_.mem_r32(header_ + kHeaderVisited);
  farWord_ = static_cast<std::uint32_t>(
      std::min<std::int64_t>(distance_.scale(static_cast<std::int32_t>(core_.mem_r32(header_ + kHeaderFar))),
                             std::numeric_limits<std::int32_t>::max()));
  limitWord_ = core_.mem_r32(header_ + kHeaderLimits);
  polygonLimit_ = distance_.increased() ? std::numeric_limits<std::int32_t>::max()
                                        : static_cast<std::int32_t>(limitWord_ & 0xFFFFu);
  setScratch(kSlotAttributeTable, core_.mem_r32(kWorldAttributeTableAddress));
  fog_ = FogTables::load(core_, (core_.mem_r16(header_ + kHeaderModeFlags) & kHeaderPaged) != 0, distance_);
}

Flow WorldMeshPass::runGroups() {
  std::uint32_t group = header_ + kHeaderGroups;
  std::int32_t groupsLeft = 0;
  std::size_t next = 0;
  do {
    std::uint32_t count = core_.mem_r16(group);
    std::uint32_t polyList = core_.mem_r32(group + 8);
    std::uint32_t threshold = core_.mem_r32(group + 4);
    if (!hostCells_.empty()) {
      const CellGroup &cell = hostCells_[next++];
      count = cell.polygonCount;
      polyList = cell.polygonList;
      threshold = cell.skipsTail | (static_cast<std::uint32_t>(cell.topY) << 16);
    }
    setScratch(kSlotPolysLeft, count);
    if (count != 0) {
      threshold_ = threshold;
      if (runPolygons(polyList) == Flow::Stop) {
        return Flow::Stop;
      }
    }
    groupsLeft = static_cast<std::int32_t>(scratch(kSlotGroupsLeft)) - 1;
    group += kGroupStride;
    setScratch(kSlotGroupsLeft, static_cast<std::uint32_t>(groupsLeft));
  } while (groupsLeft > 0);
  return Flow::Next;
}

Flow WorldMeshPass::runPolygons(std::uint32_t polyList) {
  std::int32_t polysLeft = 0;
  do {
    const std::uint32_t index = core_.mem_r16(polyList);
    polyList += 2;
    poly_ = polys_ + (index << 4);
    if (visitPolygon(poly_) == Flow::Stop) {
      return Flow::Stop;
    }
    polysLeft = static_cast<std::int32_t>(scratch(kSlotPolysLeft)) - 1;
    setScratch(kSlotPolysLeft, static_cast<std::uint32_t>(polysLeft));
  } while (polysLeft > 0);
  return Flow::Next;
}

Flow WorldMeshPass::visitPolygon(std::uint32_t poly) {
  flags_ = core_.mem_r32(poly + 0xC);
  if ((flags_ & kPolySkipMask) != 0) {
    return Flow::Next;
  }
  const std::int32_t remaining = static_cast<std::int32_t>(scratch(kSlotPolysLeft));
  setScratch(kSlotFlags, flags_);
  if (remaining < static_cast<std::int32_t>(threshold_ & 0xFFFFu)) {
    const std::uint32_t vertexIndex = core_.mem_r16(poly + (((flags_ & 0xC0u) >> 6) << 1));
    const std::int32_t limitY = static_cast<std::int16_t>(threshold_ >> 16);
    if (limitY < core_.mem_r16s(verts_ + (vertexIndex << 3) + 2)) {
      return Flow::Next;
    }
  }
  if (static_cast<std::int64_t>(visited_.size()) >= polygonLimit_) {
    return Flow::Stop;
  }
  flags_ |= kPolyVisited;
  quad_ = (flags_ & kPolyQuad) != 0;
  if (visited_.size() < static_cast<std::size_t>(limitWord_ & 0xFFFFu)) {
    core_.mem_w32(visitedList_ + 4 * static_cast<std::uint32_t>(visited_.size()), poly);
  }
  visited_.push_back(poly);
  core_.mem_w32(poly + 0xC, flags_);
  return projectPolygon() == Step::Stop ? Flow::Stop : Flow::Next;
}

Step WorldMeshPass::projectPolygon() {
  const std::uint32_t wordA = core_.mem_r32(poly_);
  const std::uint32_t wordB = core_.mem_r32(poly_ + 4);
  const std::array<std::uint32_t, 3> corner = {
      verts_ + ((wordA & 0xFFFFu) << 3), verts_ + ((wordA >> 16) << 3), verts_ + ((wordB & 0xFFFFu) << 3)};
  for (std::uint32_t i = 0; i < 3; ++i) {
    work_[i].model = core_.mem_r32(corner[i]);
    work_[i].depthUv = core_.mem_r32(corner[i] + 4);
    gte_write_data(kGteVz0 + 2 * i, work_[i].depthUv);
    gte_write_data(kGteVxy0 + 2 * i, work_[i].model);
  }
  gte_op(&core_, kRtpt);
  const std::uint32_t attributeIndex = core_.mem_r16(poly_ + 8);
  for (std::uint32_t i = 0; i < 3; ++i) {
    shadeIndex_[i] = static_cast<std::uint16_t>(work_[i].depthUv >> 16);
  }
  attribute_ = core_.mem_r32(scratch(kSlotAttributeTable) + (attributeIndex << 2));
  setScratch(kSlotAttribute, attribute_);

  if (quad_) {
    const std::uint32_t fourth = verts_ + ((wordB >> 16) << 3);
    work_[3].model = core_.mem_r32(fourth);
    work_[3].depthUv = core_.mem_r32(fourth + 4);
    gte_op(&core_, kNclip);
    shadeIndex_[3] = static_cast<std::uint16_t>(work_[3].depthUv >> 16);
    gte_write_data(kGteVxy0, work_[3].model);
    gte_write_data(kGteVz0, work_[3].depthUv);
    const std::int32_t firstArea = static_cast<std::int32_t>(gte_read_data(kGteMac0));
    setScratch(kSlotFirstScreen, gte_read_data(kGteSxy0));
    gte_op(&core_, kRtps);
    gte_op(&core_, kNclip);
    const std::int32_t secondArea = static_cast<std::int32_t>(gte_read_data(kGteMac0));
    if (firstArea <= 0 && secondArea >= 0) {
      return Step::Reject;
    }
    area_ = wrapAbs(firstArea) + wrapAbs(secondArea);
  } else {
    gte_op(&core_, kNclip);
    area_ = static_cast<std::int32_t>(gte_read_data(kGteMac0));
    if (area_ <= 0) {
      return Step::Reject;
    }
  }
  const Step depthStep = rejectByDepth();
  if (depthStep != Step::Accept) {
    return depthStep;
  }
  const Step screenStep = rejectByScreen();
  if (screenStep != Step::Accept) {
    return screenStep;
  }
  prepareAttributes();
  return Step::Accept;
}

Step WorldMeshPass::rejectByDepth() {
  const std::int32_t first = static_cast<std::int32_t>(gte_read_data(kGteSz1));
  const std::int32_t second = static_cast<std::int32_t>(gte_read_data(kGteSz2));
  std::int32_t nearest = first;
  std::int32_t farthest = second;
  if (!(first < second)) {
    nearest = second;
    farthest = first;
  }
  const std::int32_t third = static_cast<std::int32_t>(gte_read_data(kGteSz3));
  if (third < nearest) {
    nearest = third;
  } else if (farthest < third) {
    farthest = third;
  }
  std::int32_t depth = nearest;
  if (quad_) {
    const std::int32_t zeroth = static_cast<std::int32_t>(gte_read_data(kGteSz0));
    if (zeroth < nearest) {
      depth = zeroth;
    } else if (farthest < zeroth) {
      farthest = zeroth;
    }
  }
  if (static_cast<std::int32_t>(farWord_) >> 16 < depth) {
    return Step::Reject;
  }
  const std::uint32_t mode = scratch(kSlotFlags) & kPolyDepthModeMask;
  if (mode != kPolyDepthFirst) {
    depth = farthest;
    if (mode != kPolyDepthLast) {
      gte_op(&core_, quad_ ? kAvsz4 : kAvsz3);
      depth = static_cast<std::int32_t>(gte_read_data(kGteOtz));
    }
  }
  if (depth < kMinimumDepth) {
    return Step::Reject;
  }
  depth_ = depth;
  packet_ = scratch(kSlotPacket);
  const std::int32_t limit = static_cast<std::int32_t>(scratch(kSlotPacketLimit));
  if (!(static_cast<std::int32_t>(packet_) + kPacketHeadroomWords < limit)) {
    return Step::Stop;
  }
  return Step::Accept;
}

Step WorldMeshPass::rejectByScreen() {
  std::array<ScreenVertex, 4> vertices{};
  const std::array<std::uint32_t, 3> projected = {
      gte_read_data(kGteSxy0), gte_read_data(kGteSxy1), gte_read_data(kGteSxy2)};
  std::size_t count = 0;
  for (const std::uint32_t xy : projected) {
    vertices[count++] = {static_cast<std::int16_t>(sx(xy)), static_cast<std::int16_t>(sy(xy))};
  }
  if (quad_) {
    const std::uint32_t xy = scratch(kSlotFirstScreen);
    vertices[count++] = {static_cast<std::int16_t>(sx(xy)), static_cast<std::int16_t>(sy(xy))};
  }
  if (polygonOutsideWindow(std::span<const ScreenVertex>(vertices.data(), count), margin_)) {
    return Step::Reject;
  }
  return Step::Accept;
}

void WorldMeshPass::prepareAttributes() {
  const std::int32_t otLength = static_cast<std::int32_t>(core_.mem_r16(kScratch + kSlotOtLength)) - kOtSlackSlots;
  const std::uint32_t otShift = core_.mem_r16(kScratch + kSlotOtShift);
  rawDepth_ = depth_;
  otSlot_ = depth_ >> (otShift & 31u);
  if (!(otSlot_ < otLength)) {
    otSlot_ = otLength;
  }

  const std::uint32_t coarseShift = (limitWord_ >> 16) & 0xFFu;
  const std::uint32_t flags = scratch(kSlotFlags);
  const std::uint32_t uvIndex = core_.mem_r16(poly_ + 0xA);
  subdivision_ = (flags & kPolySemiTransparent) != 0 ? 0u : static_cast<std::uint32_t>(area_) >> (coarseShift & 31u);

  std::uint32_t clut = core_.mem_r16(attribute_ + 6);
  if ((core_.mem_r16(scratch(kSlotHeader) + kHeaderModeFlags) & kHeaderPaged) != 0) {
    clut += static_cast<std::uint32_t>(fog_.depthByte(static_cast<std::uint32_t>(rawDepth_) >> 6)) << 6;
  }
  clutWord_ = clut << 16;
  setScratch(kSlotClut, clutWord_);

  if ((uvIndex & 0x8000u) != 0) {
    const std::uint32_t entry = uvTable_ + ((uvIndex & 0x7FFFu) << 3);
    const std::uint32_t low = core_.mem_r32(entry);
    const std::uint32_t high = core_.mem_r32(entry + 4);
    uv_ = {low & 0xFFFFu, low >> 16, high & 0xFFFFu, high >> 16};
  } else {
    std::uint32_t packed = uvIndex;
    for (std::uint32_t i = 0; i < (quad_ ? 4u : 3u); ++i) {
      uv_[i] = core_.mem_r16(attribute_ + (packed & 7u) * 2);
      packed >>= 3;
    }
  }
  if (subdivision_ != 0) {
    for (std::uint32_t i = 0; i < (quad_ ? 4u : 3u); ++i) {
      work_[i].depthUv = (work_[i].depthUv & 0xFFFFu) | (uv_[i] << 16);
    }
  }
  texturePage_ = core_.mem_r16(attribute_ + 0xA);
  writeFlatTexture();

  const std::uint32_t first = scratch(kSlotFirstScreen);
  if (quad_) {
    screen_ = {first, gte_read_data(kGteSxy0), gte_read_data(kGteSxy1), gte_read_data(kGteSxy2)};
    for (std::uint32_t i = 0; i < 4; ++i) {
      setScratch(kSlotDepths + 4 * i, gte_read_data(kGteSz0 + i));
    }
  } else {
    screen_ = {gte_read_data(kGteSxy0), gte_read_data(kGteSxy1), gte_read_data(kGteSxy2), 0};
    for (std::uint32_t i = 0; i < 3; ++i) {
      setScratch(kSlotDepths + 4 * i, gte_read_data(kGteSz1 + i));
    }
  }
  for (std::uint32_t i = 0; i < (quad_ ? 4u : 3u); ++i) {
    work_[i].screen = screen_[i];
  }
  writeFlatPositions();
  shadeVertices();
}

void WorldMeshPass::writeFlatTexture() {
  if (subdivision_ != 0) {
    return;
  }
  const std::uint32_t pageWord = static_cast<std::uint32_t>(texturePage_) << 16;
  core_.mem_w32(packet_ + 0xC, uv_[0] | clutWord_);
  core_.mem_w32(packet_ + 0x18, uv_[1] | pageWord);
  core_.mem_w16(packet_ + 0x24, static_cast<std::uint16_t>(uv_[2]));
  if (quad_) {
    core_.mem_w16(packet_ + 0x30, static_cast<std::uint16_t>(uv_[3]));
  }
}

void WorldMeshPass::writeFlatPositions() {
  if (subdivision_ != 0) {
    return;
  }
  core_.mem_w32(packet_ + 8, screen_[0]);
  core_.mem_w32(packet_ + 0x14, screen_[1]);
  core_.mem_w32(packet_ + 0x20, screen_[2]);
  if (quad_) {
    core_.mem_w32(packet_ + 0x2C, screen_[3]);
  }
}

void WorldMeshPass::shadeVertices() {
  const std::uint32_t count = quad_ ? 4u : 3u;
  for (std::uint32_t i = 0; i < count; ++i) {
    const std::uint32_t depth = scratch(kSlotDepths + 4 * i);
    gte_write_data(kGteIrgb, shadeIndex_[i]);
    gte_write_data(kGteIr0, fog_.shade(static_cast<std::uint32_t>(depth) >> 6));
    gte_op(&core_, kDpcl);
    colors_[i] = gte_read_data(kGteRgb2);
    work_[i].color = colors_[i];
  }
  std::uint32_t code = quad_ ? kQuadCode : kTriCode;
  if ((scratch(kSlotFlags) & kPolySemiTransparent) != 0) {
    code |= kSemiTransparentBit;
  }
  codeByte_ = code >> 24;
  for (std::uint32_t i = 0; i < count; ++i) {
    colors_[i] |= code;
  }
  if (subdivision_ == 0) {
    core_.mem_w32(packet_ + 4, colors_[0]);
    core_.mem_w32(packet_ + 0x10, colors_[1]);
    core_.mem_w32(packet_ + 0x1C, colors_[2]);
    if (quad_) {
      core_.mem_w32(packet_ + 0x28, colors_[3]);
    }
    emitFlat();
  } else {
    emitSubdivided();
  }
}

void WorldMeshPass::emitFlat() {
  const std::uint32_t otAddress = (static_cast<std::uint32_t>(otSlot_) << 2) + scratch(kSlotOtBase);
  const std::uint32_t head = core_.mem_r32(otAddress);
  core_.mem_w32(otAddress, packet_ & kAddressMask);
  core_.mem_w32(packet_, head | (quad_ ? kFlatQuadLink : kFlatTriLink));
  packet_ += quad_ ? kFlatQuadBytes : kFlatTriBytes;
  setScratch(kSlotPacket, packet_);
}

void WorldMeshPass::chooseSubdivision() {
  const std::uint32_t coarseShift = (limitWord_ >> 16) & 0xFFu;
  const std::uint32_t fineShift = limitWord_ >> 24;
  fine_ = static_cast<std::uint32_t>(area_) >> (fineShift & 31u);
  const std::uint32_t shift = fine_ != 0 ? fineShift : coarseShift;
  area_ = static_cast<std::int32_t>(static_cast<std::uint32_t>(area_) - (1u << (shift & 31u)));
  if (fine_ != 0) {
    area_ >>= 1;
  }
  if (quad_) {
    pairs_ = kQuadMidpointPairsAddress;
    storedVertices_ = 9;
    totalVertices_ = fine_ != 0 ? 0x19 : 9;
    slots_ = fine_ != 0 ? kQuadFineSlotsAddress : kQuadCoarseSlotsAddress;
    counts_ = fine_ != 0 ? 0xC10 : 0x404;
  } else {
    pairs_ = kTriMidpointPairsAddress;
    storedVertices_ = 6;
    totalVertices_ = fine_ != 0 ? 0xF : 6;
    slots_ = fine_ != 0 ? kTriFineSlotsAddress : kTriCoarseSlotsAddress;
    counts_ = fine_ != 0 ? 0x1900 : 0x700;
  }
}

void WorldMeshPass::emitSubdivided() {
  chooseSubdivision();
  for (std::uint32_t i = 0; i < 3; ++i) {
    setScratch(kSlotFarColor + 4 * i, gte_read_ctrl(kGteFarColor + i));
  }
  std::uint32_t slotPtr = slots_;
  std::uint32_t pairPtr = pairs_;
  for (std::uint32_t index = 0; index < totalVertices_; ++index) {
    WorkVertex vertex;
    std::uint16_t uv = 0;
    if (index < (storedVertices_ >> 1)) {
      vertex = work_[index];
      gte_write_data(kGteSxy2, vertex.screen);
      uv = static_cast<std::uint16_t>(vertex.depthUv >> 16);
    } else {
      uv = buildSubdivisionVertex(index, pairPtr, vertex);
    }
    writeSubdivisionSlots(slotPtr, vertex, uv);
    slotPtr += 0xC;
  }
  for (std::uint32_t i = 0; i < 3; ++i) {
    gte_write_ctrl(kGteFarColor + i, scratch(kSlotFarColor + 4 * i));
  }
  linkSubdivided();
}

std::uint16_t WorldMeshPass::buildSubdivisionVertex(std::uint32_t index, std::uint32_t &pairPtr, WorkVertex &vertex) {
  const std::uint32_t pair = core_.mem_r16(pairPtr);
  pairPtr += 2;
  const std::uint32_t firstIndex = pair & 0xFFu;
  const std::uint32_t secondIndex = (pair >> 8) & 0xFFu;
  if (firstIndex >= kWorkVertices || secondIndex >= kWorkVertices) {
    lucent::error("c12-wide", "subdivision pair 0x{:04X} names a vertex outside the work set", pair);
    std::abort();
  }
  const WorkVertex &a = work_[firstIndex];
  const WorkVertex &b = work_[secondIndex];

  const std::int32_t x = (static_cast<std::int16_t>(a.model) + static_cast<std::int16_t>(b.model)) >> 1;
  const std::int32_t y = (static_cast<std::int16_t>(a.model >> 16) + static_cast<std::int16_t>(b.model >> 16)) >> 1;
  const std::int32_t z = (static_cast<std::int16_t>(a.depthUv) + static_cast<std::int16_t>(b.depthUv)) >> 1;
  gte_write_data(kGteVxy0, (static_cast<std::uint32_t>(x) & 0xFFFFu) | (static_cast<std::uint32_t>(y) << 16));
  gte_write_data(kGteVz0, static_cast<std::uint32_t>(z));
  gte_op(&core_, kRtps);

  std::uint32_t color = 0;
  for (std::uint32_t byte = 0; byte < 3; ++byte) {
    const std::uint32_t sum = ((a.color >> (8 * byte)) & 0xFFu) + ((b.color >> (8 * byte)) & 0xFFu);
    color |= (sum >> 1) << (8 * byte);
  }
  const std::uint32_t uSum = ((a.depthUv >> 16) & 0xFFu) + ((b.depthUv >> 16) & 0xFFu);
  const std::uint32_t vSum = ((a.depthUv >> 24) & 0xFFu) + ((b.depthUv >> 24) & 0xFFu);
  const std::uint16_t uv = static_cast<std::uint16_t>((uSum >> 1) | ((vSum >> 1) << 8));

  const bool refine = area_ < static_cast<std::int32_t>(kRefineAreaLimit) && (fine_ == 0 || index >= storedVertices_);
  if (refine) {
    const std::int32_t sx0 = (static_cast<std::int16_t>(a.screen) + static_cast<std::int16_t>(b.screen)) >> 1;
    const std::int32_t sy0 =
        (static_cast<std::int16_t>(a.screen >> 16) + static_cast<std::int16_t>(b.screen >> 16)) >> 1;
    const std::uint32_t projected = gte_read_data(kGteSxy2);
    gte_write_data(kGteIr0, static_cast<std::uint32_t>(area_));
    gte_write_data(kGteIr1, static_cast<std::uint32_t>(sx0));
    gte_write_data(kGteIr2, static_cast<std::uint32_t>(sy0));
    gte_write_ctrl(kGteFarColor, static_cast<std::uint32_t>(sx(projected)));
    gte_write_ctrl(kGteFarColor + 1, static_cast<std::uint32_t>(sy(projected)));
    gte_op(&core_, kIntpl);
    const std::uint32_t blendedY = gte_read_data(kGteIr2);
    const std::uint32_t blendedX = gte_read_data(kGteIr1);
    gte_write_data(kGteSxy2, (blendedX & 0xFFFFu) | (blendedY << 16));
  }
  if (fine_ != 0 && index < storedVertices_) {
    WorkVertex &stored = work_[index];
    stored.model = gte_read_data(kGteVxy0);
    stored.depthUv = (gte_read_data(kGteVz0) & 0xFFFFu) | (static_cast<std::uint32_t>(uv) << 16);
    stored.screen = gte_read_data(kGteSxy2);
    stored.color = color;
  }
  vertex.color = color;
  return uv;
}

void WorldMeshPass::writeSubdivisionSlots(std::uint32_t slotPtr, const WorkVertex &vertex, std::uint16_t uv) {
  const std::uint32_t color = vertex.color;
  const auto write = [this, color, uv](std::uint32_t offset) {
    const std::uint32_t at = packet_ + offset;
    core_.mem_w32(at, color);
    core_.mem_w32(at + 4, gte_read_data(kGteSxy2));
    core_.mem_w16(at + 8, uv);
  };
  const std::uint32_t first = core_.mem_r32(slotPtr);
  const std::uint32_t second = core_.mem_r32(slotPtr + 4);
  write(first & 0xFFFFu);
  write(first >> 16);
  write(second & 0xFFFFu);
  const std::uint32_t fourth = second >> 16;
  if (fourth == 0) {
    return;
  }
  const std::uint32_t third = core_.mem_r32(slotPtr + 8);
  write(fourth);
  if ((third & 0xFFFFu) == 0) {
    return;
  }
  write(third & 0xFFFFu);
  if ((third >> 16) != 0) {
    write(third >> 16);
  }
}

void WorldMeshPass::linkSubdivided() {
  const std::uint32_t otAddress = (static_cast<std::uint32_t>(otSlot_) << 2) + scratch(kSlotOtBase);
  const std::uint32_t head = core_.mem_r32(otAddress);
  std::uint32_t previous = head & kAddressMask;
  const std::uint32_t headTag = head ^ previous;
  const std::uint16_t clut = static_cast<std::uint16_t>(core_.mem_r16(kScratch + kSlotClut + 2));
  std::uint32_t code = codeByte_ | 8u;
  const auto chain = [&](std::uint32_t pieces, const PacketShape &shape) {
    for (; pieces != 0; --pieces) {
      core_.mem_w8(packet_ + 7, static_cast<std::uint8_t>(code));
      core_.mem_w16(packet_ + 0x1A, texturePage_);
      core_.mem_w16(packet_ + 0xE, clut);
      core_.mem_w32(packet_, previous | shape.link);
      previous = packet_ & kAddressMask;
      packet_ += shape.stride;
    }
  };
  chain(counts_ & 0xFFu, kFlatQuadShape);
  code ^= 8u;
  chain((counts_ >> 8) & 0xFFu, kFlatTriShape);
  core_.mem_w32(otAddress, previous | headTag);
  setScratch(kSlotPacket, packet_);
}

void WorldMeshPass::finish() {
  for (auto poly = visited_.rbegin(); poly != visited_.rend(); ++poly) {
    core_.mem_w32(*poly + 0xC, core_.mem_r32(*poly + 0xC) & ~kPolyVisited);
  }
  visited_.clear();
  for (std::uint32_t i = 0; i < kSavedCount; ++i) {
    core_.r[kFirstSaved + i] = core_.mem_r32(kWorldPassSaveAreaAddress + 4 * i);
  }
  core_.r[kFramePointer] = core_.mem_r32(kWorldPassSaveAreaAddress + 4 * kSavedCount);
  core_.r[kV0] = kScratch;
  core_.r[kV1] = 0;
}

} // namespace

std::int32_t widenedWindowMargin(std::int32_t nativeWidth, std::int32_t presentationWidth) {
  if (nativeWidth <= 0 || presentationWidth <= nativeWidth) {
    return 0;
  }
  const std::int64_t extra =
      static_cast<std::int64_t>(kRetailWindowWidth) * static_cast<std::int64_t>(presentationWidth - nativeWidth);
  return static_cast<std::int32_t>((extra + 2 * static_cast<std::int64_t>(nativeWidth) - 1) /
                                   (2 * static_cast<std::int64_t>(nativeWidth)));
}

bool polygonOutsideWindow(std::span<const ScreenVertex> vertices, std::int32_t margin) {
  bool allLeft = true;
  bool allAbove = true;
  bool anyInsideRight = false;
  bool anyInsideBottom = false;
  for (const ScreenVertex &vertex : vertices) {
    allLeft = allLeft && vertex.x < -margin;
    allAbove = allAbove && vertex.y < 0;
    anyInsideRight = anyInsideRight || vertex.x < kRetailWindowWidth + margin;
    anyInsideBottom = anyInsideBottom || vertex.y < kRetailWindowHeight;
  }
  return allLeft || allAbove || !anyInsideRight || !anyInsideBottom;
}

void installWorldMeshPassOverride(Game &game) {
  psx::cpu::installNativeOverride(game.core, kWorldMeshPassAddress, "c12::drawWorldMeshPass", drawWorldMeshPass);
  lucent::info("c12-wide", "installed world mesh pass owner at 0x{:08X}", kWorldMeshPassAddress);
}

void drawWorldMeshPass(Core *host) {
  Core &core = *host;
  const GuestProjectionPlan &plan = core.game->guestDisplay.plan();
  WorldMeshPass pass(
      core, widenedWindowMargin(plan.nativeExtent.width, plan.presentationExtent.width), currentDrawDistance());
  pass.run(core.r[kA0]);
}

} // namespace c12
