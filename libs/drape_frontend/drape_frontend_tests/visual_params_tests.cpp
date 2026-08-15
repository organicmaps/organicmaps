#include "testing/testing.hpp"

#include "drape_frontend/visual_params.hpp"

#include "geometry/mercator.hpp"

namespace visual_params_tests
{
UNIT_TEST(DrawTileScale_DependsOnVisualScale)
{
  uint32_t constexpr kTileSize = 1024;
  int constexpr kBaseScale = 10;
  double constexpr kRectSize = mercator::Bounds::kRangeX / (1 << kBaseScale);
  m2::RectD const rect(0.0, 0.0, kRectSize, kRectSize);

  TEST_EQUAL(df::GetDrawTileScale(rect, kTileSize, 3.0), kBaseScale, ());
  TEST_EQUAL(df::GetDrawTileScale(rect, kTileSize, 2.0), kBaseScale + 1, ());
}
}  // namespace visual_params_tests
