#include "world_mesh_pass.h"

#include <array>
#include <lucent/log.h>
#include <stdexcept>

namespace c12::test {

void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

// First-mission replay call 92: every vertex sits right of the 512-column retail window.
const std::array<ScreenVertex, 4> kRightOfWindow{{{0x236, 0xDA}, {0x232, 0xBF}, {0x22D, 0xBA}, {0x23A, 0xDA}}};

void test_retail_window_rejects_polygon_right_of_it() {
  require(polygonOutsideWindow(kRightOfWindow, 0), "retail window kept a polygon wholly right of column 512");
}

void test_widened_window_keeps_polygon_in_the_margin() {
  const std::int32_t margin = widenedWindowMargin(512, 684);
  require(margin == 86, "16:9 margin for a 512 column picture is not 86 columns");
  require(!polygonOutsideWindow(kRightOfWindow, margin), "widened window rejected a polygon inside its margin");
  require(polygonOutsideWindow(kRightOfWindow, 0x20), "margin kept a polygon wholly past it");
}

void test_window_edges() {
  const std::array<ScreenVertex, 3> inside{{{0x1FF, 0xFF}, {0x300, 0x300}, {0x300, 0x300}}};
  require(!polygonOutsideWindow(inside, 0), "a vertex on the last column and row was rejected");
  const std::array<ScreenVertex, 3> left{{{-1, 0}, {-1, 5}, {-1, 9}}};
  require(polygonOutsideWindow(left, 0), "a polygon wholly left of column 0 was kept");
  require(!polygonOutsideWindow(left, 1), "a one column margin did not keep a polygon on column -1");
  const std::array<ScreenVertex, 3> above{{{10, -1}, {20, -1}, {30, -3}}};
  require(polygonOutsideWindow(above, 0), "a polygon wholly above row 0 was kept");
  require(widenedWindowMargin(512, 512) == 0, "a picture not wider than native got a margin");
}

} // namespace c12::test

int main() {
  try {
    c12::test::test_retail_window_rejects_polygon_right_of_it();
    c12::test::test_widened_window_keeps_polygon_in_the_margin();
    c12::test::test_window_edges();
    lucent::info("c12.world-mesh", "PASS: 3/3 retail reject, widened margin, window edges");
    return 0;
  } catch (const std::exception &error) {
    lucent::error("c12.world-mesh", "FAIL: {}", error.what());
    return 1;
  }
}
