#include "c12_runtime.h"
#include "cell_collector.h"
#include "cell_output.h"
#include "core.h"
#include "draw_distance.h"
#include "enhancements.h"
#include "fog_table.h"
#include "game.h"
#include "psx_exe_image.h"
#include "title_facts.h"

#include <array>
#include <cstdint>
#include <lucent/log.h>
#include <memory>
#include <stdexcept>
#include <vector>

namespace c12::test {

void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

constexpr std::uint32_t kRecordBase = 0x80180000u;
constexpr std::uint32_t kSlotBase = 0x80190000u;

void test_draw_distance_scales_and_snaps() {
  require(DrawDistance(0).scale(8190) == 8190, "zero increase is not the retail distance");
  require(!DrawDistance(0).increased(), "zero increase reports an increase");
  require(DrawDistance(100).scale(8190) == 16380, "100 percent does not double the distance");
  require(DrawDistance(300).scale(1000) == 4000, "300 percent is not four times the distance");
  require(DrawDistance(1000).scale(1000) == 11000, "1000 percent is not eleven times the distance");
  require(DrawDistance(333).percent() == 325, "an off-grid increase did not snap to the slider step");
  require(DrawDistance(5000).percent() == 1000, "the increase is not clamped to the slider maximum");
  require(DrawDistance(-40).percent() == 0, "a negative increase is not clamped to retail");
  require(DrawDistance(300).unscale(DrawDistance(300).scale(777)) == 777, "unscale does not invert scale");
  require(DrawDistance(25).scaleUp(3) == 4, "scaleUp does not round up");
}

void test_slider_is_exposed_to_the_menu() {
  const auto settings = titleIntSettings();
  require(settings.size() == 1, "the title does not expose exactly one integer setting");
  require(settings[0].range.min == 0 && settings[0].range.max == 1000 && settings[0].range.step == 25,
          "the draw distance slider range is not 0..1000 step 25");
  require(settings[0].var == &drawDistanceCvar(), "the slider is not bound to the draw distance variable");
}

void test_fog_tables_stretch_with_the_distance() {
  std::vector<std::uint16_t> shade(64);
  std::vector<std::uint8_t> depth(64);
  for (std::size_t i = 0; i < shade.size(); ++i) {
    shade[i] = static_cast<std::uint16_t>(i * 64);
    depth[i] = static_cast<std::uint8_t>(i & 0xF);
  }
  const FogTables retail(shade, depth, DrawDistance(0));
  for (std::uint32_t i = 0; i < shade.size(); ++i) {
    require(retail.shade(i) == shade[i] && retail.depthByte(i) == depth[i], "zero increase changed the fade tables");
  }
  require(retail.shade(64) == 0 && retail.shade(0x3FF) == 0, "the retail table does not end where the guest's does");

  const FogTables far(shade, depth, DrawDistance(300));
  for (std::uint32_t i = 0; i < shade.size(); ++i) {
    require(far.shade(i * 4) == shade[i], "the fade did not move out to four times the depth");
    require(far.depthByte(i * 4 + 3) == depth[i], "the depth bytes did not move out with the fade");
  }
  require(far.shade(shade.size() * 4 - 1) == shade.back(), "the last fade value does not hold to the stretched end");
  require(far.shade(0x3FF) == 0, "the stretched table does not end at zero like the retail one");
}

void fillRecords(Core &core, std::uint32_t count) {
  for (std::uint32_t i = 0; i < count; ++i) {
    core.mem_w8(kRecordBase + i * kCellGroupStride + 6, 0);
  }
}

void test_guest_buffers_stay_at_retail_size() {
  psx::cpu::PsxExeImage image{};
  C12Runtime runtime(image);
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  Core &core = game->core;
  fillRecords(core, 200);
  core.mem_w32(kVisibleCellListAddress + 4 * kRetailCellCapacity, 0xDEADBEEFu);
  GuestCellOutput retail(core, {kSlotBase, kRetailCellCapacity});
  std::uint32_t kept = 0;
  bool full = false;
  for (std::uint32_t i = 0; i < 200 && !full; ++i) {
    CellGroup group;
    group.polygonCount = 3;
    group.polygonList = 0x80300000u + i;
    retail.stage(group);
    full = retail.commit(kRecordBase + i * kCellGroupStride, group);
    ++kept;
  }
  retail.finish();
  require(kept == kRetailCellCapacity && retail.count() == kRetailCellCapacity,
          "the retail window did not stop at 150");
  require(core.mem_r32(kVisibleCellListAddress + 4 * (kRetailCellCapacity - 1)) ==
              kRecordBase + (kRetailCellCapacity - 1) * kCellGroupStride,
          "the retail pointer list lost its last cell");
  require(core.mem_r32(kVisibleCellListAddress + 4 * kRetailCellCapacity) == 0, "the retail list is not terminated");
  require((core.mem_r8(kRecordBase + 6) & 1) == 1, "a kept cell was not flagged visible");
  require(core.mem_r32(kSlotBase + 8) == 0x80300000u && core.mem_r16(kSlotBase) == 3, "a cell slot was not written");
}

void test_host_list_holds_more_cells_than_the_guest_window() {
  psx::cpu::PsxExeImage image{};
  C12Runtime runtime(image);
  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  Core &core = game->core;
  fillRecords(core, 600);
  HostCellOutput extended;
  for (std::uint32_t i = 0; i < 600; ++i) {
    CellGroup group;
    group.polygonCount = static_cast<std::uint16_t>(i == 7 ? 0 : 2);
    group.polygonList = 0x80300000u + i;
    extended.stage(group);
    require(!extended.commit(kRecordBase + i * kCellGroupStride, group), "the host list reported itself full");
  }
  extended.finish();
  require(extended.count() == 599, "the host list did not keep every cell with polygons");
  require((core.mem_r8(kRecordBase + 6) & 1) == 0, "the host list flagged a guest cell record");
  std::vector<CellGroup> cells = extended.release();
  require(cells.size() == 599 && cells[7].polygonList == 0x80300008u, "the host list lost a cell or its order");

  WorldVisibility visibility;
  require(!visibility.active(), "a fresh visibility is active");
  visibility.publish(std::move(cells));
  require(visibility.active() && visibility.cells().size() == 599, "a published list was not delivered");
  visibility.clear();
  require(!visibility.active() && visibility.cells().empty(), "a consumed list was not cleared");
}

void test_published_cells_are_nearest_first() {
  std::vector<CellGroup> cells(5);
  const std::array<std::uint64_t, 5> distance{900, 100, 400, 100, 0};
  for (std::size_t i = 0; i < cells.size(); ++i) {
    cells[i].polygonCount = 1;
    cells[i].polygonList = static_cast<std::uint32_t>(i);
    cells[i].distance = distance[i];
  }
  WorldVisibility visibility;
  visibility.publish(std::move(cells));
  const std::array<std::uint32_t, 5> expected{4, 1, 3, 2, 0};
  for (std::size_t i = 0; i < expected.size(); ++i) {
    require(visibility.cells()[i].polygonList == expected[i], "published cells are not nearest first and stable");
  }
}

void test_gte_input_range() {
  require(fitsGteInput(0x7FFF) && fitsGteInput(-0x8000), "the signed 16-bit range was refused");
  require(!fitsGteInput(0x8000) && !fitsGteInput(-0x8001), "a value outside the signed 16-bit range was accepted");
}

} // namespace c12::test

int main() {
  try {
    c12::test::test_draw_distance_scales_and_snaps();
    c12::test::test_slider_is_exposed_to_the_menu();
    c12::test::test_fog_tables_stretch_with_the_distance();
    c12::test::test_guest_buffers_stay_at_retail_size();
    c12::test::test_host_list_holds_more_cells_than_the_guest_window();
    c12::test::test_published_cells_are_nearest_first();
    c12::test::test_gte_input_range();
    lucent::info("c12.draw-distance",
                 "PASS: 7/7 distance scaling, slider, fog stretch, retail buffers, host list, cell order, GTE range");
    return 0;
  } catch (const std::exception &error) {
    lucent::error("c12.draw-distance", "FAIL: {}", error.what());
    return 1;
  }
}
